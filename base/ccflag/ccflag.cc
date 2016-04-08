#include "ccflag.h"

#include "../cclog/cclog.h"
#include "../string_util.h"
#include "../thread_util.h"
#include "../file_util.h"

#include <stdlib.h>
#include <string.h>
#include <memory>
#include <map>

DEF_string(config, "", "path of config file");

namespace ccflag {
namespace xx {
struct FlagInfo {
    FlagInfo(const char* _type_str, const char* _name, const char* _value,
             const char* _help, const char* _file, void* _addr, int _type)
        : type_str(_type_str), name(_name), value(_value), help(_help),
          file(_file), addr(_addr), type(_type) {
    }

    std::string to_string() const {
        return std::string("--") + name + ": " + help +
            "\n\t type" + ": " + type_str + "\t     default" + ": " + value +
            "\n\t from" + ": " + file;
    }

    const char* const type_str;
    const char* const name;
    const char* const value;    // default value
    const char* const help;
    const char* const file;     // file where the flag is defined
    void* const addr;           // pointer to the flag variable
    int type;
};

class Flagger {
  public:
    static Flagger* instance() {
        static Flagger kFlagger;
        return &kFlagger;
    }

    ~Flagger() {
        if (_conf_thread != NULL) _conf_thread->join();
    }

    void add_flag(const char* type_str, const char* name, const char* value,
                  const char* help, const char* file, void* addr, int type);

    bool set_flag_value(const std::string& flg, const std::string& val,
                        std::string& err);

    bool set_bool_flags(const std::string& flg, std::string& err);

    std::string flags_info_to_string() const;

    std::vector<std::string>
    parse_flags_from_command_line(int argc, char** argv);

    void parse_flags_from_config(const std::string& config);

    void start_conf_thread() {
        _conf_thread.reset(
            new ::StoppableThread(std::bind(&Flagger::thread_fun, this), 3000));
        _conf_thread->start();
    }

  private:
    std::map<std::string, FlagInfo> _map;
    std::unique_ptr<StoppableThread> _conf_thread;
    sys::ifile _config;

    Flagger() = default;
    void thread_fun();
};

void Flagger::add_flag(const char* type_str, const char* name,
                       const char* value, const char* help, const char* file,
                       void* addr, int type) {
    auto ret = _map.insert(
        std::make_pair(name, FlagInfo(type_str, name, value, help, file, addr,
                                      type)));

    FLOG_IF(!ret.second) << "flags defined with the same name" << ": " << name
        << ", from " << ret.first->second.file << ", and " << file;
}

bool Flagger::set_flag_value(const std::string& flg, const std::string& v,
                             std::string& err) {
    auto it = _map.find(flg);
    if (it == _map.end()) {
        err = std::string("flag not defined") + ": " + flg;
        return false;
    }

    auto& fi = it->second;
    switch (fi.type) {
      case TYPE_string:
        *static_cast<std::string*>(fi.addr) = v;
        return true;

      case TYPE_bool:
        return util::to_bool(v, static_cast<bool*>(fi.addr), err);

      case TYPE_int32:
        return util::to_int32(v, static_cast<int32*>(fi.addr), err);

      case TYPE_uint32:
        return util::to_uint32(v, static_cast<uint32*>(fi.addr), err);

      case TYPE_int64:
        return util::to_int64(v, static_cast<int64*>(fi.addr), err);

      case TYPE_uint64:
        return util::to_uint64(v, static_cast<uint64*>(fi.addr), err);

      case TYPE_double:
        return util::to_double(v, static_cast<double*>(fi.addr), err);
    }

    return false; // fake return
}

/*
 * set_bool_flags("abc", err);
 *
 *   --abc ==> true
 *   --a, --b, --c ==> true
 */
bool Flagger::set_bool_flags(const std::string& flg, std::string& err) {
    auto it = _map.find(flg);
    if (it != _map.end()) {
        if (it->second.type != TYPE_bool) {
            err = std::string("value not set for non-bool flag") + ": " + flg;
            return false;
        }

        *static_cast<bool*>(it->second.addr) = true;
        return true;
    }

    for (::size_t i = 0; i < flg.size(); ++i) {
        it = _map.find(flg.substr(i, 1));
        if (it == _map.end()) {
            err = std::string("invalid combination of bool flags") + ": " + flg;
            return false;
        }

        *static_cast<bool*>(it->second.addr) = true;
    }

    return true;
}

std::string Flagger::flags_info_to_string() const {
    std::string s;
    for (auto it = _map.begin(); it != _map.end(); ++it) {
        if (it->second.help[0] != '\0') {
            s += it->second.to_string() + "\n";
        }
    }
    return s;
}

FlagSaver::FlagSaver(const char* type_str, const char* name, const char* value,
                     const char* help, const char* file, void* addr, int type) {
    Flagger::instance()->add_flag(type_str, name, value, help, file, addr, type);
}

static std::string trim_string(const std::string& s) {
    std::string x = util::replace_string(s, '\t', ' ');
    x = util::replace_string(x, '\"', ' ');
    x = util::replace_string(x, "　", ' ');  // replace Chinese space
    return util::trim_string(x);
}

static std::string getline(sys::ifile& ifs) {
    std::string line;

    while (true) {
        std::string s = xx::trim_string(ifs.getline());
        if (!ifs.valid()) return line;

        if (s.empty() || *s.rbegin() != '\\') {
            line += s;
            return line;
        }

        s.resize(s.size() - 1);
        line += xx::trim_string(s);
    }
}

std::vector<std::string>
Flagger::parse_flags_from_command_line(int argc, char** argv) {
    if (argc <= 1) return std::vector<std::string>();

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    /*
     * ./exe --  or  ./exe --help     print flags info and exit
     */
    if (args.size() == 1 && (args[0].find_first_not_of('-') == std::string::npos
        || args[0] == "--help")) {
        printf("%s", this->flags_info_to_string().c_str());
        exit(0);
    }

    std::string flg, val;
    std::vector<std::string> v;

    for (::size_t i = 0; i < args.size(); ++i) {
        auto& arg = args[i];

        if (arg[0] != '-') {
            v.push_back(arg);  // non-flag element
            continue;
        }

        ::size_t bp = arg.find_first_not_of('-');
        ::size_t ep = arg.find('=');
        FLOG_IF(ep <= bp) << "invalid parameter" << ": " << arg;

        std::string err;

        if (ep == std::string::npos) {
            flg = arg.substr(bp);
            if (!this->set_bool_flags(flg, err)) FLOG << err;

        } else {
            flg = arg.substr(bp, ep - bp);
            val = arg.substr(ep + 1);
            if (!this->set_flag_value(flg, val, err)) FLOG << err;
        }
    }

    return v;
}

void Flagger::parse_flags_from_config(const std::string& config) {
    if (!_config.open(config)) {
        FLOG << "failed to open config file" << ": " << config;
    }

    while (_config.valid()) {
        std::string line = xx::trim_string(xx::getline(_config));
        if (line.empty() || line[0] == '#' || line.substr(0, 2) == "//") {
            continue;
        }

        ::size_t pos = std::min(line.find('#'), line.find("//"));
        if (pos != std::string::npos) line.resize(pos);

        pos = line.find('=');
        if (pos + 1 <= 1) {
            FLOG << "invalid config: " << line << ", at "
                 << _config.path() << ':' << _config.line();
        }

        /*
         * ignore '!' at the beginning of line
         */
        int x = (line[0] == '!');
        std::string flg = xx::trim_string(line.substr(x, pos - x));
        std::string val = xx::trim_string(line.substr(pos + 1));

        std::string err;
        if (!xx::Flagger::instance()->set_flag_value(flg, val, err)) {
            FLOG << err << ", at " << _config.path() << ':' << _config.line();
        }
    }
}

void Flagger::thread_fun() {
    if (!_config.exist() || !_config.modified()) return;

    if (!_config.reopen()) {
        ELOG << "failed to open config file: " << _config.path();
        return;
    }

    while (_config.valid()) {
        std::string line = xx::trim_string(xx::getline(_config));

        // ignore lines not beginning with '!'
        if (line.empty() || line[0] != '!') continue;

        ::size_t pos = std::min(line.find('#'), line.find("//"));
        if (pos != std::string::npos) line.resize(pos);

        pos = line.find('=');
        if (pos == std::string::npos) {
            ELOG << "invalid config: " << line << ", at "
                 << _config.path() << ':' << _config.line();
            continue;
        }

        std::string flg = xx::trim_string(line.substr(1, pos - 1));
        std::string val = xx::trim_string(line.substr(pos + 1));

        std::string err;
        if (!xx::Flagger::instance()->set_flag_value(flg, val, err)) {
            ELOG << err << ", at " << _config.path() << ':' << _config.line();
        }
    }
}
}  // namespace xx

std::vector<std::string> init_ccflag(int argc, char** argv) {
    auto ins = xx::Flagger::instance();
    auto v = ins->parse_flags_from_command_line(argc, argv);

    if (!::FLG_config.empty()) {
        ins->parse_flags_from_config(::FLG_config);
        ins->start_conf_thread();
    }

    return v;
}

std::string set_flag_value(const std::string& name, const std::string& value) {
    std::string err;
    xx::Flagger::instance()->set_flag_value(name, value, err);
    return err;
}
}  // namespace ccflag
