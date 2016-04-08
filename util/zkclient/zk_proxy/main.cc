#include "proxy_watcher.h"

int main(int argc, char* argv[]) {
  ccflag::init_ccflag(argc, argv);
  cclog::init_cclog(*argv);

  util::ProxyWatcher watcher;
  watcher.watch();

  return 0;
}

