#pragma once

#include "base/base.h"

namespace util {
class LogReader;

class LogInputStream {
  public:
    explicit LogInputStream(bool enable_crc32_check)
        : _crc32_check(enable_crc32_check) {
    }
    virtual ~LogInputStream();

    bool Init(const std::string& log_dir);

    // delete by yourself.
    std::string* next();

  private:
    bool _crc32_check;

    bool loadCache();
    std::deque<std::string*> _log_queue;

    class LogDir;
    std::unique_ptr<LogDir> _dir;

    bool createReader();
    std::unique_ptr<LogReader> _reader;

    const static uint32 kLoadNumber = 64;

    DISALLOW_COPY_AND_ASSIGN(LogInputStream);
};
}

