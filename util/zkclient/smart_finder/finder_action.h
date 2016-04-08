#pragma once

#include "util/zkclient/zk_action.h"

namespace util {

class SmartFinderImpl;
class FinderAction : public ZkAction {
  public:
    explicit FinderAction(SmartFinderImpl* impl)
        : _impl(impl) {
      CHECK_NOTNULL(impl);
    }
    virtual ~FinderAction() = default;

    virtual bool init(ZkClient* zk_client);

  private:
    SmartFinderImpl* _impl;

    sys::timer _timer;
    virtual void doCheck();

    virtual void handleAbort();
    virtual void handleEstablished();

    virtual void handleRemove(const std::string& path);
    virtual void handleCreate(const std::string& path);

    virtual void handleChanged(const std::string& path);
    virtual void handleChild(const std::string& path);

    DISALLOW_COPY_AND_ASSIGN (FinderAction);
};

}

