#include "db_impl.h"

#include "redis.h"

DEF_uint32(moved_retry_cnt, 3, "times to retry after moved error");
DEF_uint32(slot_reflesh_interval, 3, "slot reflesh interval");

namespace {
static bool parseServer(const std::string &server, util::IpPort *ip_port) {
  auto vec_ipport = util::split_string(server, ':');
  if (vec_ipport.size() != 2) {
    ELOG<<"server in wrong format: " << server;
    return false;
  }

  *ip_port = std::make_pair(vec_ipport[0], util::to_uint32(vec_ipport[1]));
  return true;
}

static bool parseSlots(const std::string &slot, uint32 *slot_begin,
                       uint32 *slot_end) {
  auto vec_slot = util::split_string(slot, '-');
  if (vec_slot.size() != 2) {
    ELOG<<"slots in wrong format : " << slot;
    return false;
  }

  *slot_begin = util::to_uint32(vec_slot[0]);
  *slot_end = util::to_uint32(vec_slot[1]);
  CHECK(*slot_begin >= 0 && *slot_begin < 16384);
  CHECK(*slot_end >= 0 && *slot_end < 16384);
  return true;
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

DBImpl::DBImpl(const std::string &server_list, uint32 timedout) {
  init(server_list, timedout);
}

void DBImpl::init(const std::string &server_list, uint32 timedout) {
  CHECK(!server_list.empty());
  auto servs = util::split_string(server_list, ';');
  CHECK(!servs.empty());

  auto it = servs.begin();
  std::vector<IpPort> server_lists;
  for (; it != servs.end(); ++it) {
    const auto &server = *it;
    IpPort ip_port;
    if (!parseServer(server, &ip_port)) continue;

    LOG<<"server add: " << "[" << ip_port.first <<"," << ip_port.second << "]";
    server_lists.push_back(ip_port);
  }

  createSlotTable(server_lists, timedout);
}

void DBImpl::createSlotTable(const std::vector<IpPort> &server_list,
                             uint32 timedout) {
  _slot_table.reset(new SlotTable(server_list, timedout));
  CHECK_NOTNULL(_slot_table);
}

bool DBImpl::get(const std::string &key, std::string *value) {
  std::string err;
  int retry = 0;
  do {
    if (getInternal(key, value, &err)) return true;

    if (!handleErrorMsg(err)) break;

  } while (retry <= FLG_moved_retry_cnt);
  return false;
}

bool DBImpl::getInternal(const std::string &key, std::string *value,
                         std::string *err) {
  if ((*getRedisClient(key))->get(key, value, err)) return true;

  return err->empty() ? true : false;
}

bool DBImpl::set(const std::string &key, const std::string &value,
                 int32 expired) {
  std::string err;
  int retry = 0;
  do {
    if (setInternal(key, value, &err)) return true;

    if (!handleErrorMsg(err)) break;

  } while (retry <= FLG_moved_retry_cnt);
  return false;
}

bool DBImpl::setInternal(const std::string &key, const std::string &value,
                         std::string *err, int32 expired) {
  bool ret;
  if (expired == -1) {
    ret = (*getRedisClient(key))->set(key, value, err);
  } else {
    ret = (*getRedisClient(key))->set(key, value, expired, err);
  }

  if (ret) return true;
  return err->empty() ? true : false;
}

bool DBImpl::del(const std::string &key) {
  std::string err;
  int retry = 0;
  do {
    if (delInternal(key, &err)) return true;

    if (!handleErrorMsg(err)) break;

  } while (retry <= FLG_moved_retry_cnt);
  return false;
}

bool DBImpl::delInternal(const std::string &key, std::string *err) {
  if (!(*getRedisClient(key))->del(key, err)) {
    return false;  //need to parse error message
  }
  return true;
}

std::shared_ptr<RedisClient> DBImpl::getRedisClient(const std::string &key) {
  uint32 slot = HASH_SLOT(key);
  std::string ip;
  uint32 port;
  if (!_slot_table->get(slot, &ip, &port)) {
    //to handle: ip port not found
    LOG<< "slot: " << slot << " points to: " << ip << ", " << port;
  }

  LOG<< "redis slot: " << slot << " ip: " << ip << ", port: " << port;
  IpPort ip_port(ip, port);
  return _slot_table->createRedisClintIfNotExists(ip_port);
}

bool DBImpl::isMovedErr(const std::string &err) const {
  return err.substr(0, 5) == "MOVED";
}

bool DBImpl::handleErrorMsg(const std::string &err) {
  ELOG<< "err msg: " << err;
  if(!isMovedErr(err)) return false;

  uint32 slot_moved;
  std::string ip_moved;
  uint32 port_moved;

  if(!parseMovedErrorMsg(err, &slot_moved, &ip_moved, &port_moved))
  return false;

  _slot_table->update(slot_moved, ip_moved, port_moved);
//  _slot_table->refleshSlotTable();
  return true;
}

bool DBImpl::parseMovedErrorMsg(const std::string &err, uint32 *slot_moved,
                                std::string *ip_moved, uint32 *port_moved) {
//MOVED 3999 127.0.0.1:6381
  auto vec_err = util::split_string(err, ' ');
  if (vec_err.size() < 3) {
    ELOG<< "parse error: " << err;
    return false;
  }

  auto server = vec_err[vec_err.size() - 1];
  *slot_moved = util::to_int32(vec_err[1]);
  IpPort ip_port;
  if (!parseServer(server, &ip_port)) return false;

  LOG<<"slot: "<< vec_err[1]<<" moved to: " <<ip_port.first << "," << ip_port.second;
  *ip_moved = ip_port.first;
  *port_moved = ip_port.second;
  return true;
}

DBImpl::SlotTable::SlotTable(const std::vector<IpPort>& server_list,
                             uint32 timedout) {
  CHECK(!server_list.empty());
  _server_list = server_list;

  _table.resize(16384);
  CHECK(timedout >= 0 && timedout <= 3000);

  initSlotTable();
}

DBImpl::SlotTable::~SlotTable() {
}

void DBImpl::SlotTable::initSlotTable() {
  auto it = _server_list.begin();
  for (; it != _server_list.end(); ++it) {
    const auto &serv = *it;

    const auto &redis_client = createRedisClintIfNotExists(serv);
    const auto &cluster_nodes = (*redis_client)->cluster_nodes();
    LOG<< "cluster nodes: " << cluster_nodes;
    if (initClusterNodes(cluster_nodes)) break;
  }
}

bool DBImpl::SlotTable::initClusterNodes(const std::string &cluster_nodes) {
//ba699b74eb46c1e57c0c239eac643753f39ca0ab 192.168.1.252:9980 myself,master - 0 0 1 connected 0-5460
  if (cluster_nodes.empty()) return false;

  auto servers = util::split_string(cluster_nodes, '\n');
  for (int i = 0; i < servers.size(); ++i) {
    const auto &server = servers[i];
    auto elements = util::split_string(server, ' ');
    if (elements.size() < 9) {
      ELOG<< "wrong cluster: " << server << ", element size: " << elements.size();
      return false;
    }

    const auto &elem_ip_port = elements[1];
    IpPort ip_port;
    if (!parseServer(elem_ip_port, &ip_port)) return false;

    const auto &elem_slot = elements.back();
    uint32 slot_begin = 0, slot_end = 0;
    if (!parseSlots(elem_slot, &slot_begin, &slot_end)) return false;
    LOG<< "init server: "<< elem_ip_port << " slot from " << slot_begin << " to " << slot_end;

    for (int j = slot_begin; j <= slot_end; ++j) {
      _table[j] = ip_port;
    }
  }
  return true;
}

bool DBImpl::SlotTable::get(int32 slot, std::string *ip, uint32 *port) {
  {
    MutexGuard l(_mutex);
    if (checkSlotServerExists(slot)) {
      const auto &ip_port = _table[slot];
      *ip = ip_port.first;
      *port = ip_port.second;
      return true;
    }
  }

  getRandomIpPort(slot, ip, port);
  return false;
}

bool DBImpl::SlotTable::checkSlotServerExists(int32 slot) {
  const auto & server = _table[slot];
  if (server.first.empty() || server.second == 0) return false;

  return true;
}

void DBImpl::SlotTable::getRandomIpPort(int32 slot, std::string *ip,
                                        uint32 *port) {
  {
    MutexGuard l(_mutex);
    uint32 index = sys::local_time.us() % _server_list.size();
    *ip = _server_list[index].first;
    *port = _server_list[index].second;
  }

// need update
  update(slot, *ip, *port);
}

void DBImpl::SlotTable::update(int32 slot, const std::string &ip, uint32 port) {
  IpPort ip_port = std::make_pair(ip, port);
  {
    MutexGuard l(_mutex);
    _table[slot] = ip_port;
  }

  auto serv_idx = std::find(_server_list.begin(), _server_list.end(), ip_port);
  if (serv_idx == _server_list.end()) {
    _server_list.push_back(ip_port);
  }
}

std::shared_ptr<RedisClient> DBImpl::SlotTable::createRedisClintIfNotExists(
    const IpPort& ip_port) {
  MutexGuard l(_mutex);
  if (_redis_table.find(ip_port) == _redis_table.end()) {
    std::shared_ptr<RedisClient> redis_client = CreateRedisClient(
        ip_port.first, ip_port.second, 1000);
    CHECK_NOTNULL(redis_client);
    _redis_table[ip_port] = redis_client;
  }
  return _redis_table[ip_port];
}

void DBImpl::SlotTable::remove(const std::string &ip, uint32 port) {
  MutexGuard l(_mutex);
//server list can not be empty
  if (_server_list.size() == 1) return;

  IpPort ip_port = std::make_pair(ip, port);
  auto serv_idx = std::find(_server_list.begin(), _server_list.end(), ip_port);
  if (serv_idx != _server_list.end()) {
    _server_list.erase(serv_idx);
    return;
  }
  ELOG<< "server: " <<ip << "," << port << " not found ";
}

std::shared_ptr<DB> createDB(const std::string &server_list, uint32 timedout) {
  return std::shared_ptr<DB>(new DBImpl(server_list, timedout));
}

}
