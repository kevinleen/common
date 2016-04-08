#include "finder_action.h"
#include "smart_finder_impl.h"

namespace util {

bool FinderAction::init(ZkClient* zk_client) {
  CHECK_NOTNULL(zk_client);
  _zk_client = zk_client;
  return true;
}

void FinderAction::doCheck() {
  if (_timer.sec() < 3) return;

  _impl->onCheck();
}

void FinderAction::handleAbort() {
  _impl->onAbort();
}

void FinderAction::handleEstablished() {
  _impl->onEstablished();
}

void FinderAction::handleRemove(const std::string& path) {
  _impl->onAbort();
}

void FinderAction::handleCreate(const std::string& path) {
  _impl->onEstablished();
}

void FinderAction::handleChanged(const std::string& path) {
  _impl->onChanged();
}

void FinderAction::handleChild(const std::string& path) {
  _impl->onChild();
}

}
