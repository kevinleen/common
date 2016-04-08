#pragma once

#include "base/base.h"

namespace util {

class FLogger {
  public:
    explicit FLogger(const std::string& fpath) {
      _file = ::fopen(fpath.data(), "a+");
    }
    ~FLogger() {
      ::fclose(_file);
    }

    void log(const std::string& log) {
      ::fwrite(log.data(), sizeof(char), log.size(), _file);
    }

  private:
    FILE* _file;

    DISALLOW_COPY_AND_ASSIGN(FLogger);
};

}

