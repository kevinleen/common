#include "cclog.h"
#include "failure_handler.h"

#include "../time_util.h"
#include "../file_util.h"
#include "../string_util.h"
#include "../thread_util.h"
#include "../signal_util.h"

#include <map>
#include <vector>
#include <memory>

DEF_bool(log2stderr, false, "log to stderr only");
DEF_bool(alsolog2stderr, false, "log to stderr and file");
DEF_bool(dlog_on, false, "if true, turn on DLOG");
DEF_bool(klog_off, false, "if true, turn off KLOG");

DEF_string(log_dir, "/tmp", "log dir");
DEF_string(log_prefix, "", "prefix of log file name");
DEF_string(ip, "", "ip for klog");
DEF_int64(max_log_file_size, 1 << 30, "max log file size, default: 1G");

namespace cclog {
namespace xx {
static std::string kProgName = sys::get_prog_name();

class Logger {
  public:
    explicit Logger(uint32 ms);
    virtual ~Logger() = default;

    virtual void stop();

    static void set_log_dir(const std::string& log_dir) {
        _log_dir = log_dir;
        if (!_log_dir.empty() && *_log_dir.rbegin() != sys::path_separ) {
            _log_dir += sys::path_separ;
        }
    }

    static void set_log_prefix(const std::string& log_prefix) {
        _log_prefix = log_prefix;
        if (!_log_prefix.empty() && _log_prefix.back() != '.') {
            _log_prefix.push_back('.');
        }
    }

  protected:
    Mutex _log_mtx;
    std::unique_ptr<StoppableThread> _log_thread;
    uint32 _ms; // run thread_fun() every n ms

    std::vector<void*> _logs;
    std::vector<void*> _temp;

    uint32 _last_day;
    uint32 _last_hour;

    static std::string _log_dir;
    static std::string _log_prefix;

  private:
    void thread_fun();

    virtual void flush_log_files() = 0;
    virtual void write_logs(std::vector<void*>& logs) = 0;

    DISALLOW_COPY_AND_ASSIGN(Logger);
};

std::string Logger::_log_dir;
std::string Logger::_log_prefix;

Logger::Logger(uint32 ms)
    : _ms(ms) {
    _log_thread.reset(
        new StoppableThread(std::bind(&Logger::thread_fun, this), _ms));
    _log_thread->start();

    int64 sec = sys::local_time.sec();
    _last_day = sec / 86400;
    _last_hour = sec / 3600;
}

void Logger::thread_fun() {
    {
        MutexGuard g(_log_mtx);
        if (!_logs.empty()) _logs.swap(_temp);
    }

    this->write_logs(_temp);
    _temp.clear();

    if (_temp.capacity() > 1024) {
        std::vector<void*>().swap(_temp);
        _temp.reserve(1024);
    }

    this->flush_log_files();
}

void Logger::stop() {
    _log_thread->join(); // wait for logging thread

    MutexGuard g(_log_mtx);
    if (_logs.empty()) return;

    this->write_logs(_logs);
    this->flush_log_files();
    _logs.clear();
}

class TaggedLogger : public Logger {
  public:
    TaggedLogger() : Logger(500) {
    }

    virtual ~TaggedLogger() {
        Logger::stop();
    }

    void push(TaggedLog* log) {
        ::MutexGuard g(_log_mtx);
        _logs.push_back(log);
    }

    enum strategy {
        by_day = 0,
        by_hour = 1,
    };

    void log_by_day(const char* tag) {
        _strategy[tag] = by_day;
    }

    void log_by_hour(const char* tag) {
        _strategy[tag] = by_hour;
    }

    bool open_log_file(const char* tag);

  private:
    std::map<const char*, sys::wfile> _files;
    std::map<const char*, int> _strategy;

    void log_to_file(const std::string& time, TaggedLog* log);

    virtual void flush_log_files();
    virtual void write_logs(std::vector<void*>& logs);
};

bool TaggedLogger::open_log_file(const char* tag) {
    if (::strlen(tag) > 5 && std::string(tag, 5) == "klog_") {
        this->log_by_hour(tag);
    }

    std::string time =
        sys::local_time.to_string(_strategy[tag] == 0 ? "%Y%m%d" : "%Y%m%d%H");

    std::string name = _log_prefix + kProgName + "_" + tag + "_" + time + ".log";
    std::string path = _log_dir + name;

    if (!_files[tag].open(path)) return false;

    std::string link_path = _log_dir + kProgName + "." + tag;
    sys::symlink(name, link_path);

    return true;
}

inline void TaggedLogger::log_to_file(const std::string& time, TaggedLog* log) {
    auto& file = _files[log->type()];
    int strategy = _strategy[log->type()];
    if (!file.valid() && !this->open_log_file(log->type())) return;

    file.write(time);
    file.write(log->data(), log->size());
}

void TaggedLogger::write_logs(std::vector<void*>& logs) {
    std::string time(sys::local_time.to_string()); // yyyy-mm-dd hh:mm:ss

    for (::size_t i = 0; i < logs.size(); ++i) {
        TaggedLog* log = (TaggedLog*) logs[i];
        this->log_to_file(time, log);
        delete log;
    }
}

void TaggedLogger::flush_log_files() {
    uint64 ms = sys::local_time.ms() + _ms;
    uint32 day = ms / (86400 * 1000);
    uint32 hour = ms / (3600 * 1000);

    if (day != _last_day) std::swap(day, _last_day);
    if (hour != _last_hour) std::swap(hour, _last_hour);

    for (auto it = _files.begin(); it != _files.end(); ++it) {
        auto& file = it->second;

        if (file.exist()) file.flush();
        if (file.exist() && day == _last_day &&
            (hour == _last_hour || _strategy[it->first] != by_hour)) {
            continue;
        }

        file.close();
    }
}

class LevelLogger : public Logger {
  public:
    LevelLogger();
    virtual ~LevelLogger();

    void push_non_fatal_log(LevelLog* log) {
        MutexGuard g(_log_mtx);
        _logs.push_back(log);
    }

    void push_fatal_log(LevelLog* log);

    bool open_log_file(int level, const char* tag);

    void install_failure_handler();

    void uninstall_failure_handler() {
        _failure_handler.reset();
    }

    void install_signal_handler();
    void uninstall_signal_handler();

  private:
    std::vector<sys::wfile> _files;
    std::vector<int> _index;
    std::unique_ptr<FailureHandler> _failure_handler;

    void log_to_file(const std::string& time, LevelLog* log);
    void log_to_stderr(const std::string& time, LevelLog* log);

    void on_failure();        // for CHECK failed, SIGSEGV, SIGFPE
    void on_signal(int sig);  // for SIGTERM, SIGINT, SIGQUIT

    virtual void flush_log_files();
    virtual void write_logs(std::vector<void*>& logs);
};

LevelLogger::LevelLogger() : Logger(1000) {
    _files.resize(FATAL + 1);
    _index.resize(FATAL + 1);
    this->install_signal_handler();
}

LevelLogger::~LevelLogger() {
    Logger::stop();
    this->uninstall_signal_handler();
}

void LevelLogger::install_failure_handler() {
    if (_failure_handler != NULL) return;
    _failure_handler.reset(NewFailureHandler());
    _failure_handler->set_handler(std::bind(&LevelLogger::on_failure, this));
}

void LevelLogger::install_signal_handler() {
    auto f = std::bind(&LevelLogger::on_signal, this, std::placeholders::_1);

    sys::signal.add_handler(SIGTERM, std::bind(f, SIGTERM));
    sys::signal.add_handler(SIGQUIT, std::bind(f, SIGQUIT));
    sys::signal.add_handler(SIGINT, std::bind(f, SIGINT));
    sys::signal.ignore(SIGPIPE);
}

void LevelLogger::uninstall_signal_handler() {
    sys::signal.del_handler(SIGTERM);
    sys::signal.del_handler(SIGQUIT);
    sys::signal.del_handler(SIGINT);
}

void LevelLogger::on_failure() {
    ::cclog::close_cclog();

    if (!_files[FATAL].valid()) this->open_log_file(FATAL, "FATAL");
    _failure_handler->set_fd(_files[FATAL].fd());
}

void LevelLogger::on_signal(int sig) {
    ::cclog::close_cclog();
    if (sig != SIGTERM) return;

    std::string msg("terminated: probably killed by someone!\n");
    ::fwrite(msg.data(), 1, msg.size(), stderr);

    if (_files[FATAL].valid() || this->open_log_file(FATAL, "FATAL")) {
        _files[FATAL].write(msg);
        _files[FATAL].flush();
    }
}

bool LevelLogger::open_log_file(int level, const char* tag) {
    auto& file = _files[level];

    std::string time = sys::local_time.to_string("%Y%m%d");  // yyyymmdd
    std::string name = _log_prefix + kProgName + "." + time; // + "." + tag;

    if (file.exist()) {
        name += std::string(1, '_') + util::to_string(++_index[level]);
    }

    name += std::string(1, '.') + tag;
    std::string path = _log_dir + name;

    if (!file.open(path)) return false;

    std::string link_path = _log_dir + kProgName + "." + tag;
    sys::symlink(name, link_path);

    return true;
}

inline void LevelLogger::log_to_stderr(const std::string& time, LevelLog* log) {
    ::fwrite("IWEF" + log->type(), 1, 1, stderr);
    ::fwrite(time.data(), 1, time.size(), stderr);
    ::fwrite(log->data(), 1, log->size(), stderr);
}

void LevelLogger::write_logs(std::vector<void*>& logs) {
    std::string time(sys::local_time.to_string("%m%d %H:%M:%S"));

    if (!FLG_log2stderr && !FLG_alsolog2stderr) { /* default: log to file */
        for (::size_t i = 0; i < logs.size(); ++i) {
            LevelLog* log = (LevelLog*) logs[i];
            this->log_to_file(time, log);
            delete log;
        }

    } else if (FLG_alsolog2stderr) { /* log to stderr and file */
        for (::size_t i = 0; i < logs.size(); ++i) {
            LevelLog* log = (LevelLog*) logs[i];
            this->log_to_stderr(time, log);
            this->log_to_file(time, log);
            delete log;
        }

    } else { /* log to stderr */
        for (::size_t i = 0; i < logs.size(); ++i) {
            LevelLog* log = (LevelLog*) logs[i];
            this->log_to_stderr(time, log);
            delete log;
        }
    }
}

#define WRITE_LOGS(time, log, i) \
    do { \
        auto& file = _files[i]; \
        if (!file.valid() && !this->open_log_file(i, #i)) return; \
        file.write("IWEF"[log->type()]); \
        file.write(time); \
        file.write(log->data(), log->size()); \
    } while (0)

void LevelLogger::log_to_file(const std::string& time, LevelLog* log) {
    WRITE_LOGS(time, log, INFO);
    if (log->type() < WARNING) return;

    WRITE_LOGS(time, log, WARNING);
    if (log->type() < ERROR) return;

    WRITE_LOGS(time, log, ERROR);
}

void LevelLogger::push_fatal_log(LevelLog* log) {
    ::cclog::close_cclog();

    std::string time(sys::local_time.to_string("%m%d %H:%M:%S"));
    WRITE_LOGS(time, log, FATAL);
    _files[FATAL].flush();

    this->log_to_stderr(time, log);
    delete log;

    if (_failure_handler == NULL) exit(0);

    _failure_handler->set_handler(NULL);
    _failure_handler->set_fd(_files[FATAL].fd());
    ::abort();
}

#undef WRITE_LOGS

void LevelLogger::flush_log_files() {
    uint32 day = sys::local_time.sec() / 86400;

    // reset index on new day
    if (day != _last_day) {
        std::swap(day, _last_day);
        _index.clear();
        _index.resize(FATAL + 1);
    }

    for (uint32 i = 0; i < _files.size(); ++i) {
        auto& file = _files[i];

        if (file.exist()) file.flush();
        if (file.exist() && file.size() < ::FLG_max_log_file_size) continue;

        file.close();
    }
}

class KLogger : public Logger {
  public:
    KLogger() : Logger(500) {
    }

    virtual ~KLogger() {
        stop();
    }

    virtual void stop() {
        if (_failure_cb) _failure_cb();
        Logger::stop();
    }

    void push(KLog* log) {
        ::MutexGuard g(_log_mtx);
        _logs.push_back(log);
    }

    void set_log_callback(
        std::function<void(const char*, const char*, uint32)> cb) {
        _log_cb = cb;
    }

    void set_flush_callback(std::function<void()> cb) {
        _flush_cb = cb;
    }

    void set_failure_callback(std::function<void()> cb) {
        _failure_cb = cb;
    }

  private:
    void log_to_file(const std::string& time, KLog* log);

    virtual void flush_log_files();
    virtual void write_logs(std::vector<void*>& logs);

  private:
    std::function<void(const char*, const char*, uint32)> _log_cb;
    std::function<void()> _flush_cb;
    std::function<void()> _failure_cb;
};

void KLogger::log_to_file(const std::string& time, KLog* log) {
    std::string msg = FLG_ip + "&" + time + "&" + log->sb().to_string();
    _log_cb(log->topic(), msg.data(), msg.size());
}

void KLogger::flush_log_files() {
    if (_flush_cb) _flush_cb();
}

void KLogger::write_logs(std::vector<void*>& logs) {
    std::string time = sys::local_time.to_string();

    for (auto i = 0; i < logs.size(); ++i) {
        KLog* log = (KLog*) logs[i];
        this->log_to_file(time, log);
        delete log;
    }
}

static TaggedLogger kTaggedLogger;
static LevelLogger kLevelLogger;
static KLogger kKLogger;

TaggedLogSaver::~TaggedLogSaver() {
    (*_log) << '\n';
    kTaggedLogger.push(_log);
}

KLogSaver::~KLogSaver() {
    _log->sb() << '\n';
    kKLogger.push(_log);
}

void klog::set_log_callback(
    std::function<void(const char*, const char*, uint32)> cb) {
    kKLogger.set_log_callback(cb);
}

void klog::set_flush_callback(std::function<void()> cb) {
    kKLogger.set_flush_callback(cb);
}

void klog::set_failure_callback(std::function<void()> cb) {
    kKLogger.set_failure_callback(cb);
}

void NonFatalLogSaver::push() {
    kLevelLogger.push_non_fatal_log(_log);
}

void FatalLogSaver::push() {
    kLevelLogger.push_fatal_log(_log);
}
}  // namespace xx

void init_cclog(const std::string& argv0) {
    auto it = sys::split_path(argv0);
    if (!it.second.empty()) xx::kProgName = std::move(it.second);

    if (!FLG_log_dir.empty()) {
        std::string cmd = "mkdir -p " + FLG_log_dir;
        system(cmd.c_str());
    }

    xx::Logger::set_log_dir(::FLG_log_dir);
    xx::Logger::set_log_prefix(::FLG_log_prefix);
    xx::kLevelLogger.install_failure_handler();
}

void close_cclog() {
    xx::kTaggedLogger.stop();
    xx::kLevelLogger.stop();
    xx::kKLogger.stop();
}

void log_by_day(const char* tag) {
    xx::kTaggedLogger.log_by_day(tag);
}

void log_by_hour(const char* tag) {
    xx::kTaggedLogger.log_by_hour(tag);
}
}  // namespace cclog
