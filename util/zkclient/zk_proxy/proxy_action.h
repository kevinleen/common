#pragma once

#include "util/zkclient/zk_action.h"

namespace util {

class ZkProxy : public ZkAction {
  public:
    ZkProxy(const std::string& path, const std::string& value);
    virtual ~ZkProxy() = default;

    virtual bool init(ZkClient* zk_client);

    virtual void doCheck();

    virtual void handleAbort();
    virtual void handleEstablished();

    virtual void handleRemove(const std::string& path);
    virtual void handleCreate(const std::string& path) {
    }

    virtual void handleChanged(const std::string& path);
    virtual void handleChild(const std::string& path) {
    }

  private:
    const std::string _path;
    const std::string _value;

    sys::timer _timer;
    bool initDirectory();
    bool createIfNotExist(bool force = false);

    DISALLOW_COPY_AND_ASSIGN(ZkProxy);
};

}
