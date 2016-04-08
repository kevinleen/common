#pragma once

#include "base/base.h"

namespace util {

class ZkClient;
class ZkAction {
  public:
    virtual ~ZkAction() = default;

    virtual bool init(ZkClient* zk_client) = 0;

    virtual void doCheck() = 0;

    virtual void handleAbort() = 0;
    virtual void handleEstablished() = 0;

    virtual void handleCreate(const std::string& path) = 0;
    virtual void handleRemove(const std::string& path) = 0;

    virtual void handleChanged(const std::string& path) = 0;
    virtual void handleChild(const std::string& path) = 0;

  protected:
    ZkAction()
        : _zk_client(NULL) {
    }

    ZkClient* _zk_client;

  private:
    DISALLOW_COPY_AND_ASSIGN(ZkAction);
};

}
