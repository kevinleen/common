#pragma once

#include "proxy_action.h"
#include "process_watcher.h"
#include "util/zkclient/zk_client.h"

DEF_uint32(watch_pid, 0, "pid of watching");

DEF_string(zk_path, "", "the full path in zookeeper");
DEF_string(proxy_server, "172.0.0.1:80;127.0.0.1:81",
           "server's address, split by char ';'");

namespace util {

class ProxyWatcher {
  public:
    ProxyWatcher() = default;
    ~ProxyWatcher() = default;

    void watch() {
      CHECK(!FLG_zk_path.empty());
      _zk_action.reset(new util::ZkProxy(FLG_zk_path, serializeServerList()));
      _zk_client.reset(new util::ZkClient(_zk_action));

      CHECK(_zk_client->init());
      CHECK(_zk_action->init(_zk_client.get()));

      _watcher.reset(
          new util::ProcessWatcher(
              FLG_watch_pid, std::bind(&ProxyWatcher::exitHandler, this)));
      _watcher->watch();
    }

  private:
    std::shared_ptr<util::ZkProxy> _zk_action;
    std::unique_ptr<util::ZkClient> _zk_client;

    void exitHandler() {
      if (_zk_client != NULL) {
        _zk_client->remove(FLG_zk_path);
      }
      _watcher->stop();
      exit(0);
    }

    std::unique_ptr<util::ProcessWatcher> _watcher;
    const std::string serializeServerList() const {
      CHECK(!FLG_proxy_server.empty());
      return FLG_proxy_server;
    }

    DISALLOW_COPY_AND_ASSIGN(ProxyWatcher);
};

}

