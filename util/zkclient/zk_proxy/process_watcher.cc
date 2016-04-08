#include "process_watcher.h"

DEF_uint32(watch_interval, 3, "watch process interval, seconds");

namespace util {

void ProcessWatcher::watch(bool in_another_thread) {
  _stop = false;

  if (!in_another_thread) {
    while (!_stop) {
      watchInternal();
      sys::sleep(FLG_watch_interval);
    }
    return;
  }

  _thread.reset(
      new StoppableThread(std::bind(&ProcessWatcher::watchInternal, this),
                          FLG_watch_interval * 1000));
  _thread->start();
}

void ProcessWatcher::watchInternal() {
  int ret = ::kill(_pid, 0);
  if (ret != 0) {
    _cb();
    WLOG<< "process have already exited, pid: " << _pid;
  }
}

}
