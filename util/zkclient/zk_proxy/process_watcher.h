#pragma once

#include "base/base.h"

namespace util {

class ProcessWatcher {
  public:
    typedef std::function<void()> closure;
    explicit ProcessWatcher(pid_t pid, closure cb)
        : _pid(pid), _stop(true) {
      CHECK_NOTNULL(cb);
      _cb = cb;
    }
    ~ProcessWatcher() {
      stop();
    }

    void watch(bool in_another_thread = false);
    void stop() {
      _stop = true;
      if (_thread != NULL) {
        _thread->join();
        _thread.reset();
      }
    }

  private:
    pid_t _pid;
    closure _cb;

    bool _stop;
    void watchInternal();
    std::unique_ptr<StoppableThread> _thread;

    DISALLOW_COPY_AND_ASSIGN(ProcessWatcher);
};

}
