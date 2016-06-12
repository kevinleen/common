#include "redis.h"
#include "db_impl.h"

DEF_uint32(moved_retry_cnt, 3, "times to retry after moved error");
DEF_uint32(slot_reflesh_interval, 3, "slot reflesh interval");

namespace {

bool parseServer(const std::string &server, std::string* ip, uint16* port) {
  auto ipport = util::split_string(server, ':');
  if (ipport.size() != 2) {
    ELOG<<"server in wrong format: " << server;
    return false;
  }

  *ip = ipport[0];
  *port = util::to_uint32(ipport[1]);
  return true;
}

bool parseSlot(const std::string& item, std::string* ip, uint16* port,
               uint32* slot_begin, uint32* slot_end) {
  auto vec = util::split_string(item, ' ');
  if (vec.size() != 2) {
    WLOG<< "wrong slot item: " << item;
    return false;
  }

  auto slots_vec = util::split_string(vec[0], '~');
  if (slots_vec.size() != 2) {
    WLOG<< "wrong slot range: " << vec[0];
    return false;
  }

  *slot_begin = util::to_int32(slots_vec[0]);
  *slot_end = util::to_int32(slots_vec[1]);

  auto serv_vec = util::split_string(vec[1], ',');
  if (serv_vec.empty()) {
    WLOG<< "wrong server: " << vec[1];
    return false;
  }

  return parseServer(serv_vec[0], ip, port);
}

uint32_t HASH_SLOT(const std::string &strKey) {
  const char *pszKey = strKey.data();
  size_t nKeyLen = strKey.size();
  size_t nStart, nEnd; /* start-end indexes of { and  } */

  /* Search the first occurrence of '{'. */
  for (nStart = 0; nStart < nKeyLen; nStart++)
    if (pszKey[nStart] == '{') break;

  /* No '{' ? Hash the whole key. This is the base case. */
  if (nStart == nKeyLen) return crc16(pszKey, nKeyLen) & 16383;

  /* '{' found? Check if we have the corresponding '}'. */
  for (nEnd = nStart + 1; nEnd < nKeyLen; nEnd++)
    if (pszKey[nEnd] == '}') break;

  /* No '}' or nothing between {} ? Hash the whole key. */
  if (nEnd == nKeyLen || nEnd == nStart + 1)
    return crc16(pszKey, nKeyLen) & 16383;

  /* If we are here there is both a { and a  } on its right. Hash
   * what is in the middle between { and  }. */
  return crc16(pszKey + nStart + 1, nEnd - nStart - 1) & 16383;
}

}

namespace util {

std::shared_ptr<DB> createDB(const std::string &server_list, uint32 timedout) {
  std::shared_ptr<DBImpl> db(new DBImpl(timedout));
  if (!db->init(server_list)) {
    db.reset();
    ELOG<< "init redis cluster error";
  }

  return db;
}

DBImpl::~DBImpl() = default;
DBImpl::DBImpl(uint32 timedout)
    : _timedout(timedout) {
  CHECK_NE(timedout, 0);
  WLOG<< "redis cluster timedout: " << timedout;
}

// "|0~100 127.0.0.1:999,127.0.0.1:888|101~300 127.0.0.1:777,127.0.0.1:666|"
bool DBImpl::initCluster(const std::string& ip, uint16 port) {
  NodeMap nodes;
  auto client = CreateRedisClient(ip, port, _timedout);
  auto slots = util::split_string((*client)->cluster_slots(), '|');
  if (slots.empty()) {
    ELOG<< "query cluser is empty";
    return false;
  }

  for (auto it = slots.begin(); it != slots.end(); ++it) {
    uint32 slot_begin, slot_end;
    std::string ip;
    uint16 port;
    if (!parseSlot(*it, &ip, &port, &slot_begin, &slot_end)) {
      ELOG<< "parse slot error: " << *it;
      continue;
    }

    auto redis = CreateRedisClient(ip, port, _timedout);
    nodes[toServer(ip, port)] = redis;

    MutexGuard l(_slot_mutex);
    for (uint32 i = slot_begin; i <= slot_end; ++i) {
//      WLOG<< "slot: " << i << " --> " << ip << ":" << port;
      _slot_map[i] = redis;
    }
  }

  updateNodes(nodes);
  return true;
}

std::shared_ptr<RedisClient> DBImpl::getRedis(const std::string& server) {
  std::shared_ptr<RedisClient> client;
  MutexGuard l(_node_mutex);
  auto it = _nodes.find(server);
  if (it != _nodes.end()) {
    client = it->second;
  }
  return client;
}

void DBImpl::updateNodes(const NodeMap& nodes) {
  CHECK(!nodes.empty());
  MutexGuard l(_node_mutex);
  _nodes = nodes;
}

std::shared_ptr<RedisClient> DBImpl::randomRedis() {
  std::shared_ptr<RedisClient> client;
  MutexGuard l(_node_mutex);
  if (!_nodes.empty()) {
    auto it = _nodes.begin();
    uint32 index = sys::local_time.us() % _nodes.size();
    for (int i = 0; i < index && it != _nodes.end(); ++i) {
      ++it;
    }
    WLOG<< "random redis: " << it->first;
    client = it->second;
  }
  return client;
}

bool DBImpl::init(const std::string &server_list) {
  CHECK(!server_list.empty());
  WLOG<< "redis server list: " << server_list;

  _slot_map.resize(16384);
  auto ip_ports = util::split_string(server_list, ';');
  for (auto it = ip_ports.begin(); it != ip_ports.end(); ++it) {
    std::string ip;
    uint16 port;
    if (!parseServer(*it, &ip, &port)) {
      continue;
    }
    if (!initCluster(ip, port)) {
      ELOG<< "redis impl init cluster error";
      continue;
    }

    return true;
  }

  ELOG<< "init cluster error: no actived nodes";
  return false;
}

void DBImpl::eraseNode(const std::string& ip, uint16 port) {
  MutexGuard l(_node_mutex);
  _nodes.erase(toServer(ip, port));
}

bool DBImpl::get(const std::string &key, std::string *value) {
  std::string err;
  int retry = 0;
  do {
    auto client = getRedisClient(key);
    if (client == NULL) {
      WLOG<< "get empty redis client";
      continue;
    }

//    WLOG << "1111111111: " << HASH_SLOT(key) << " " << client->ip() << " " << client->port();
    if ((*client)->get(key, value, &err)) return true;
//    WLOG << "22222  err: " << err;
    if (err.empty()) return false;
    if (!handleErrorMsg(err)) {
      eraseSlot(HASH_SLOT(key));
      if (err == "connect error") {
        eraseNode(client->ip(), client->port());
      }
    }

  } while (retry++ <= FLG_moved_retry_cnt);
  return false;
}

bool DBImpl::set(const std::string &key, const std::string &value,
                 int32 expired) {
  std::string err;
  int retry = 0;
  do {
    if (setInternal(key, value, &err, expired)) return true;

  } while (retry++ <= FLG_moved_retry_cnt);
  return false;
}

bool DBImpl::setInternal(const std::string &key, const std::string &value,
                         std::string *err, int32 expired) {
  bool ret;
  auto client = getRedisClient(key);
  if (client == NULL) {
    WLOG<< "get empty redis client";
    return false;
  }

  if (expired == -1) {
    ret = (*client)->set(key, value, err);
  } else {
    ret = (*client)->set(key, value, expired, err);
  }

  if (ret) return true;
  if (!err->empty() && !handleErrorMsg(*err)) {
    eraseSlot(HASH_SLOT(key));
    if (*err == "connect error") {
      eraseNode(client->ip(), client->port());
    }
  }

  return err->empty() ? true : false;
}

bool DBImpl::del(const std::string &key) {
  std::string err;
  int retry = 0;
  do {
    auto client = getRedisClient(key);
    if (client == NULL) {
      WLOG<< "get empty redis client";
      return false;
    }
    if ((*client)->del(key, &err)) return true;

    if (!err.empty() && !handleErrorMsg(err)) {
      eraseSlot(HASH_SLOT(key));
      if (err == "connect error") {
        eraseNode(client->ip(), client->port());
      }
    }

  } while (retry++ <= FLG_moved_retry_cnt);
  return false;
}

std::shared_ptr<RedisClient> DBImpl::getRedisClient(const std::string &key) {
  uint32 slot = HASH_SLOT(key);
  std::shared_ptr<RedisClient> client;
  {
    MutexGuard l(_slot_mutex);
    client = _slot_map[slot];
  }

  if (client == NULL) {
    client = randomRedis();
  }

  return client;
}

void DBImpl::eraseSlot(uint32 slot) {
  WLOG<< "erase slot: " << slot;
  MutexGuard l(_slot_mutex);
  _slot_map[slot] = std::shared_ptr<RedisClient>();
}

void DBImpl::updateSlot(uint32 slot, const std::string& ip, uint16 port) {
  auto client = getRedis(ip, port);
  if (client == NULL) {
    client = CreateRedisClient(ip, port, _timedout);
    MutexGuard l(_node_mutex);
    _nodes[toServer(ip, port)] = client;
  }

  WLOG<< "moved slot: " << slot << ": " << ip << ":" << port;
  MutexGuard l(_slot_mutex);
  _slot_map[slot] = client;
}

bool DBImpl::isMovedErr(const std::string &err) const {
  return err.substr(0, 5) == "MOVED";
}

bool DBImpl::handleErrorMsg(const std::string &err) {
  WLOG<< "redis handle err msg: " << err;
  if (!isMovedErr(err)) return false;

  std::string ip;
  uint32 slot;
  uint16 port;
  if (!parseMovedErrorMsg(err, &slot, &ip, &port)) {
    ELOG<< "parse moved error: " << err;
    return false;
  }

  updateSlot(slot, ip, port);
  return true;
}

/*
 *
 * err format: MOVED 3999 127.0.0.1:6381
 */
bool DBImpl::parseMovedErrorMsg(const std::string &err, uint32 *slot,
                                std::string *ip, uint16 *port) const {
  auto vec_err = util::split_string(err, ' ');
  if (vec_err.size() < 3) {
    ELOG<< "parse error: " << err;
    return false;
  }

  auto server = *vec_err.rbegin();
  *slot = util::to_int32(vec_err[1]);

  if (!parseServer(server, ip, port)) {
    return false;
  }

  WLOG<<"slot: "<< *slot <<" moved to: " << *ip << ":" << *port;
  return true;
}
}

