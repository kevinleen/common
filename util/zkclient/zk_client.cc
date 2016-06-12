#include "zk_client.h"
#include "zk_action.h"

DEF_string(zk_server, "127.0.0.1:3000,127.0.0.1:3001", "zk's address");
DEF_uint32(zk_timedout, 1, "zk's timedout, second");

namespace {

void watchHandler(zhandle_t *zh, int type, int state, const char *path,
                  void *watcherCtx) {
  util::ZkClient* cli = static_cast<util::ZkClient*>(watcherCtx);
  cli->handleEvent(type, state, std::string(path));
}
}

namespace util {

ZkClient::ZkClient(std::shared_ptr<ZkAction> action)
    : _action(action), _event(false) {
  assert(action != NULL);
  _timeout = 3;

  _connected = false;
  _zh_handle = NULL;
}

ZkClient::~ZkClient() {
  MutexGuard l(_mutex);
  if (_thread != NULL) {
    _thread->join();
    _thread.reset();
  }

  if (_zh_handle != NULL) {
    zookeeper_close(_zh_handle);
    _zh_handle = NULL;
  }
}

bool ZkClient::init(const std::string& zk_server) {
  cclog::log_by_day("zk_debug");
  cclog::log_by_day("zk_error");

  auto zk_ip = zk_server;
  if (zk_server.empty()) {
    zk_ip = FLG_zk_server;
  }

  assert(!zk_ip.empty() && FLG_zk_timedout >= 1);
  _action->init(this);
  {
    MutexGuard l(_mutex);
    _server = zk_ip;
    _timeout = FLG_zk_timedout;

    clientid_t cid;
    ::memset(&cid, 0x00, sizeof(cid));
    _zh_handle = ::zookeeper_init(zk_ip.data(), watchHandler,
                                  FLG_zk_timedout * 1000, &cid, this, 0);
    if (_zh_handle == NULL) {
      TLOG("error")<< "can't connect zkServer: " << zk_ip;
      return false;
    }
  }

  if (!_event.timed_wait(FLG_zk_timedout * 1000)) {
    TLOG("zk_error")<< "zk timedout" << zk_ip;
  }

  MutexGuard l(_mutex);
  _thread.reset(
      new StoppableThread(std::bind(&ZkClient::doConnect, this), 1 * 1000));
  _thread->start();

  return _connected;
}

/*
 * type:
 *   ZOO_CREATED_EVENT:           1
 *   ZOO_DELETED_EVENT:           2
 *   ZOO_CHANGED_EVENT:           3
 *   ZOO_CHILD_EVENT:             4
 *   ZOO_SESSION_EVENT:           -1
 *   NOTWATCHING_EVENT_DEF        -2
 *
 *
 * state:
 *   ZOO_CONNECTED_STATE:           1
 *   ZOO_EXPIRED_SESSION_STATE:     -112
 *   ZOO_CONNECTED_STATE            3
 */
void ZkClient::handleEvent(int type, int state, const std::string& path) {
  TLOG("zk_debug")<< "zk event: " << path << " type:" << type << " state: " << state;
  if (type == ZOO_SESSION_EVENT) {
    if (state == ZOO_CONNECTED_STATE) handleConnected();
    else if (state == ZOO_EXPIRED_SESSION_STATE) {
      handleExpired();
    }
  }
  if (type == ZOO_CREATED_EVENT) {
    _action->handleCreate(path);
  }
  if (type == ZOO_DELETED_EVENT) {
    _action->handleRemove(path);
  }
  if (type == ZOO_CHANGED_EVENT) {
    _action->handleChanged(path);
  }
  if (type == ZOO_CHILD_EVENT) {
    _action->handleChild(path);
  }
}

bool ZkClient::create(const std::string& path, const std::string& value,
                      bool permanent) {
  MutexGuard l(_mutex);
  if (!_connected) {
    TLOG("zk_error")<< "zk not connected";
    return false;
  }

  uint32 flag = permanent ? 0 : ZOO_EPHEMERAL;
  ACL_vector acl = { 0 };
  int ret = ::zoo_create(_zh_handle, path.data(), value.data(), value.size(),
                         &ZOO_OPEN_ACL_UNSAFE, flag,
                         NULL,
                         0);
  switch (ret) {
    case ZOK:
      return true;
    case ZNODEEXISTS:
      return false;
    case ZNONODE:
    case ZNOAUTH:
    case ZNOCHILDRENFOREPHEMERALS:
    case ZBADARGUMENTS:
      break;
    default:
      handleExpiredInternal();
      break;
  }

  TLOG("zk_error")<< "zoo_create error, path: " << path << " ret: " << zerror(ret);
  return false;
}

bool ZkClient::remove(const std::string& path) {
  MutexGuard l(_mutex);
  if (!_connected) {
    TLOG("zk_error")<< "zk not connected";
    return false;
  }

  int ret = ::zoo_delete(_zh_handle, path.data(), 0);
  switch (ret) {
    case ZOK:
    case ZNONODE:
      return true;
    default:
      handleExpiredInternal();
      break;
  }

  TLOG("zk_error")<< "zoo_delete error, path: " << path << " ret: " << zerror(ret);
  return false;
}

bool ZkClient::exist(const std::string& path) {
  MutexGuard l(_mutex);
  if (!_connected) {
    TLOG("zk_error")<< "zk not connected";
    return false;
  }

  int ret = ::zoo_exists(_zh_handle, path.data(), 1, NULL);
  switch (ret) {
    case ZOK:
      return true;
    case ZNONODE:
      break;
    default:
      _connected = false;
      handleExpiredInternal();
      break;
  }

  return false;
}

bool ZkClient::watchDir(const std::string& path) {
  MutexGuard l(_mutex);
  if (!_connected) {
    TLOG("zk_error")<< "zk not connected";
    return false;
  }

  int ret = ::zoo_exists(_zh_handle, path.data(), 1, NULL);
  if (ret != ZOK) {
    TLOG("zk_error")<< "watch error: " << ::zerror(ret);
    return false;
  }

  return true;
}

bool ZkClient::read(const std::string& path, std::string* value) {
  value->clear();
  MutexGuard l(_mutex);
  if (!_connected) {
    TLOG("zk_error")<< "zk not connected";
    return false;
  }

  std::vector<char> buf(81920);
  int size = buf.size();
  struct Stat st;
  int ret = ::zoo_get(_zh_handle, path.data(), 1, buf.data(), &size, &st);
  switch (ret) {
    case ZOK:
      *value = std::string(buf.data(), size);
      return true;
    case ZNONODE:
      break;
    default:
      _connected = false;
      handleExpiredInternal();
      break;
  }

  TLOG("zk_error")<< "zoo_get error, path: " << path << " ret: " << ret;

  return false;
}

bool ZkClient::write(const std::string& path, const std::string& value) {
  MutexGuard l(_mutex);
  if (!_connected) {
    TLOG("zk_error")<< "zk not connected";
    return false;
  }

  int ret = ::zoo_set(_zh_handle, path.data(), value.data(), value.size(), 0);
  switch (ret) {
    case ZOK:
      return true;
    case ZNONODE:
      break;
    default:
      _connected = false;
      handleExpiredInternal();
      break;
  }

  TLOG("zk_error")<< "zoo_set error, path: " << path << " ret: " <<ret;
  return false;
}

bool ZkClient::listDir(const std::string& path,
                       std::vector<std::string>* children) {
  children->clear();
  MutexGuard l(_mutex);
  if (!_connected) {
    TLOG("zk_error")<< "zk not connected";
    return false;
  }

  struct String_vector strvec;
  int ret = ::zoo_get_children(_zh_handle, path.data(), 1, &strvec);
  switch (ret) {
    case ZOK:
      for (int i = 0; i < strvec.count; ++i) {
        std::string child(strvec.data[i]);
        children->push_back(child);
      }
      ::free(strvec.data);
      strvec.data = NULL;
      return true;
    case ZNONODE:
      break;
    default:
      _connected = false;
      handleExpiredInternal();
      break;
  }

  TLOG("zk_error")<< "zoo_set error, path: " << path << " ret: " << ret;

  return false;
}

void ZkClient::handleExpired() {
  TLOG("zk_debug")<< "zk expired";
  {
    MutexGuard l(_mutex);
    _connected = false;
  }

  handleExpiredInternal();
}

bool ZkClient::reconnect() {
  zhandle_t* old_handler = NULL;
  {
    MutexGuard l(_mutex);
    if (_connected) return true;
    old_handler = _zh_handle;
    _zh_handle = NULL;
  }

  if (old_handler != NULL) zookeeper_close(old_handler);

  {
    MutexGuard l(_mutex);
    _zh_handle = ::zookeeper_init(_server.data(), watchHandler, _timeout * 1000,
                                  0, this, 0);
    TLOG("zk_debug")<< "zk timeout: " << _timeout;
    if (_zh_handle == NULL) {
      TLOG("zk_error")<< "can't connect zkServer: " << _server;
      return false;
    }
  }

  if (!_event.timed_wait(_timeout * 1200)) {
    TLOG("zk_error")<< "zk timedout";
    return false;
  }

  MutexGuard l(_mutex);
  return _connected;
}

void ZkClient::handleExpiredInternal() {
  _action->handleAbort();
  _event.signal();
}

void ZkClient::handleConnected() {
  TLOG("zk_debug")<< "zk connected successfully";
  {
    MutexGuard l(_mutex);
//    assert(!_connected);
    _connected = true;
  }

  _action->handleEstablished();
  _event.signal();
}

void ZkClient::doConnect() {
  _event.timed_wait(1 * 1000);

  if (isConnected() || reconnect()) {
    _action->doCheck();
  }
}

}
