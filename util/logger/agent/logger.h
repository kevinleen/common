#pragma once

#include "base/base.h"

namespace util {

class MSPolicy;
class LoggerImpl;

class Logger {
  public:
    virtual ~Logger() = default;

    enum {
      SUCCESS = 0, FAIL, TIMEDOUT,
    };

    virtual bool readPolicy(MSPolicy* policy) = 0;
    virtual void record(const std::string& method, uint8 status) = 0;

    static Logger* create(std::shared_ptr<LoggerImpl> impl);

  protected:
    const std::string _serv;

    explicit Logger(const std::string& serv)
        : _serv(serv) {
      CHECK(!serv.empty());
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(Logger);
};

}
