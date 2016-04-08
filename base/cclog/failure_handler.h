#pragma once

#include <stdio.h>
#include <functional>

namespace cclog {
namespace xx {
class FailureHandler {
  public:
    FailureHandler() = default;
    virtual ~FailureHandler() = default;

    virtual void set_fd(FILE* file) = 0;
    virtual void set_handler(std::function<void()> cb) = 0;

  private:
    FailureHandler(const FailureHandler&) = delete;
    void operator=(const FailureHandler&) = delete;
};

FailureHandler* NewFailureHandler();
}  // namespace xx
}  // namespace cclog
