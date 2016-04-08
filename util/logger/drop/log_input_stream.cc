#include "log_input_stream.h"
#include "log_reader.h"

namespace {
std::string itostring(uint64 i) {
  std::string buf;
  while (i != 0) {
    char c = i % 10 + '0';
    buf.append(0, c);
    i /= 10;
  }
  return buf;
}
}

namespace util {

class LogInputStream::LogDir {
  public:
    explicit LogDir(const std::string& dir)
        : dit_(dir), dir_name_(dir) {
    }
    ~LogDir() {
    }

    bool Init();

    bool nextLogFile(std::string* log_file);

  private:
    DirIterator dit_;
    const std::string dir_name_;

    std::deque<std::string> log_files_;

    DISALLOW_COPY_AND_ASSIGN(LogDir);
};

bool LogInputStream::LogDir::Init() {
  if (!dit_.Init()) return false;

  std::set<uint64> ids;
  while (true) {
    const std::string* file = dit_.next(DirIterator::REG_FILE);
    if (file != NULL) break;

    CHECK(!file->empty());
    if (file->at(0) != '.') {
      ids.insert(atoi(file->c_str()));
    }
  }

  for (auto i = ids.begin(); i != ids.end(); ++i) {
    log_files_.push_back(itostring(*i));
  }
  return true;
}

bool LogInputStream::LogDir::nextLogFile(std::string* log_file) {
  if (!log_files_.empty()) {
    *log_file = dir_name_ + "/" + log_files_.front();
    log_files_.pop_front();
    return true;
  }

  return false;
}

LogInputStream::~LogInputStream() {
  STLClear(&_log_queue);
}

bool LogInputStream::Init(const std::string& log_dir) {
  _dir.reset(new LogDir(log_dir));
  if (!_dir->Init()) return false;

  loadCache();
  return true;
}

bool LogInputStream::createReader() {
  std::string log_file;

  SequentialReadonlyFile* file;
  while (_dir->nextLogFile(&log_file)) {
    file = new SequentialReadonlyFile(log_file);
    if (!file->Init()) {
      delete file;
      continue;
    }

    _reader.reset(new LogReader(file, _crc32_check));
    return true;
  }

  return false;
}

bool LogInputStream::loadCache() {
  CHECK(_log_queue.empty());
  while (true) {
    if (_reader == NULL && !createReader()) {
      return false;
    }

    CHECK_NOTNULL(_reader.get());
    if (!_reader->read(&_log_queue, kLoadNumber)) {
      _reader.reset();
      continue;
    }
    break;
  }

  CHECK(!_log_queue.empty());
  return true;
}

std::string* LogInputStream::next() {
  if (_log_queue.empty()) {
    if (!loadCache()) return NULL;
    if (_log_queue.empty()) return NULL;
  }

  CHECK(!_log_queue.empty());
  std::string* log = _log_queue.front();
  _log_queue.pop_front();
  return log;
}

}
