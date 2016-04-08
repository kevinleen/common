#include "watch_dog.h"
#include <sys/wait.h>

DEF_bool(daemon, true, "run as daemon");
DEF_bool(restart_if_killed, false,
         "if true, will restart server if it was killed by signal");
DEF_bool(restart_if_coredump, false,
         "if true, will restart server if it was stopped as coredump");

namespace inv {

void WatchDog::removeStopFile() const {
  RemoveFile(_stop_file);
}

bool WatchDog::stopFileExist() const {
  return FileExist(_stop_file);
}

void WatchDog::watch(int argc, char* argv[]) {
  ccflag::init_ccflag(argc, argv);
  cclog::init_cclog(*argv);

  if (FLG_daemon) ::daemon(1, 0);

  removeStopFile();
  while (!stopFileExist()) {
    auto pid = ::fork();
    if (pid == -1) {
      _server->loop();
      return;
    }

    // parent.
    int status;
    auto kill_pid = waitpid(pid, &status, WUNTRACED);
    CHECK_EQ(kill_pid, pid);
    if (WIFEXITED(status)) {
      ELOG<< "server exit normaly";
      return;
    }
    if (WIFSIGNALED(status)) {
      if (WCOREDUMP(status)) {
        ELOG<<"server stopped because coredump";
        if (!FLG_restart_if_coredump) {
          return;
        }
      }

      ELOG<< "server killed by signal: " << WTERMSIG(status);
      if (!FLG_restart_if_killed) {
        WLOG<<"not restart because killed...";
        return;
      }
    }
  }
}

}

