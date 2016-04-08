#include "proxy_action.h"
#include "util/zkclient/zk_client.h"

namespace util {

ZkProxy::ZkProxy(const std::string& path, const std::string& value)
    : _path(path), _value(value) {
  CHECK(!path.empty() && !value.empty());

  WLOG<< "zk path: " << path;
}

bool ZkProxy::init(ZkClient* zk_client) {
  CHECK_NOTNULL(zk_client);
  _zk_client = zk_client;

  return true;
}

void ZkProxy::handleAbort() {
}

void ZkProxy::handleEstablished() {
  createIfNotExist(true);
}

void ZkProxy::handleChanged(const std::string& path) {
  createIfNotExist(true);
}

void ZkProxy::handleRemove(const std::string& path) {
  createIfNotExist(true);
}

void ZkProxy::doCheck() {
  createIfNotExist(false);
}

bool ZkProxy::initDirectory() {
  auto vec = split_string(_path, '/');

  std::string full_path;
  for (int i = 0; i < vec.size() - 1; ++i) {
    full_path += "/" + vec[i];

    if (_zk_client->exist(full_path)) {
      continue;
    }

    bool ret = _zk_client->create(full_path, "");
    if (!ret) {
      ELOG<< "create node error, path: " << full_path;
      return false;
    }
  }

  return true;
}

bool ZkProxy::createIfNotExist(bool force) {
  if (!force) {
    if (_timer.sec() < 3) {
      return true;
    }
  }

  if (_zk_client->exist(_path)) return true;

  if (!initDirectory()) return false;
  bool ret = _zk_client->create(_path, _value);
  if (!ret) {
    ELOG<< "create node error, path: " << _path;
    return false;
  }

  _timer.restart();
  return true;
}

}
