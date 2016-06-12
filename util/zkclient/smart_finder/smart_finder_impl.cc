#include "finder_action.h"
#include "smart_finder_impl.h"
#include "../zk_client.h"

namespace util {

SmartFinderImpl::SmartFinderImpl(const std::string& server,
                                 const std::string &path)
    : SmartFinder(server, path) {
  WLOG<< "finder service name: " << server;
  WLOG<< "finder service path: " << path;

  MutexGuard l(_watch_mutex);
  _watched = false;
}

SmartFinderImpl::~SmartFinderImpl() = default;

bool SmartFinderImpl::init(const std::string& zk_server) {
  _action.reset(new FinderAction(this));
  _zk_client.reset(new ZkClient(_action));

  if (!_zk_client->init(zk_server)) {
    ELOG<< "init zk error ";
    return false;
  }

  if (!_action->init(_zk_client.get())) {
    DLOG("zk_error") << "finder action initialized error";
    return false;
  }

  return onCheck();
}

void SmartFinderImpl::onAbort() {
  MutexGuard l(_watch_mutex);
  _watched = false;
}

void SmartFinderImpl::onEstablished() {
  watchDir(_path);
}

void SmartFinderImpl::onChild() {
  onCheck();
}

void SmartFinderImpl::onChanged() {
  onCheck();
}

bool SmartFinderImpl::onCheck() {
  if (!watchDir(_path)) return false;

  MutexGuard l(_mutex);
  std::set<std::string> nodes;
  std::vector<ServerEntry> server_list;
  if (!queryFromZk(&nodes, &server_list)) {
    DLOG("zk_error") << "query form zk error, server: " << _server;
    return false;
  }

  std::vector<std::string> added, removed;
  diff(nodes, &added, &removed);

  handleDown(removed);
  handleUp(added);

  return true;
}

void SmartFinderImpl::diff(const std::set<std::string>& nodes,
                           std::vector<std::string>* added,
                           std::vector<std::string>* removed) {
  for (auto it = nodes.begin(); it != nodes.end(); ++it) {
    const auto& node = *it;
    if (_map.find(node) == _map.end()) {
      added->push_back(node);
    }
  }

  for (auto it = _map.begin(); it != _map.end(); ++it) {
    const auto& node = it->first;
    if (nodes.find(node) == nodes.end()) {
      removed->push_back(node);
    }
  }
}

void SmartFinderImpl::handleUp(const std::vector<std::string>& nodes) {
  for (auto it = nodes.begin(); it != nodes.end(); ++it) {
    const auto& node = *it;
    WLOG<< "servive(" << _server << ") up, node: " << node;

    ServerEntry entry;
    if (!parseEntry(*it, &entry)) continue;

    _map[*it] = entry;
    if (_up_closure != NULL) {
      _up_closure(entry.first, entry.second);
    }
  }
}

void SmartFinderImpl::handleDown(const std::vector<std::string>& nodes) {
  for (auto it = nodes.begin(); it != nodes.end(); ++it) {
    const auto& node = *it;
    WLOG<< "servive(" << _server << ") down, node: " << node;

    ServerEntry entry;
    if (!popEntry(node, &entry)) {
      continue;
    }

    if (_down_closure != NULL) {
      _down_closure(entry.first, entry.second);
    }
  }
}

bool SmartFinderImpl::popEntry(const std::string& name, ServerEntry* entry) {
  auto it = _map.find(name);
  if (it == _map.end()) {
    ELOG<< "not find entry, name: " << name;
    return false;
  }

  *entry = it->second;
  _map.erase(it);
  return true;
}

bool SmartFinderImpl::watchDir(const std::string& full_path) {
  bool watched = false;
  {
    MutexGuard l(_watch_mutex);
    watched = _watched;
  }

  if (watched) return true;
  if (!_zk_client->exist(_path)) {
    DLOG("zk_error") << "zk node not exist, path: " << _path;
    return false;
  }

  if (!_zk_client->watchDir(_path)) {
    ELOG<< "zk watch failed, path: " << _path;
    return false;
  }

  MutexGuard l(_watch_mutex);
  _watched = true;
  return true;
}

bool SmartFinderImpl::parseEntry(const std::string& node, ServerEntry* entry) {
  std::string data, full_path(_path + "/" + node);
  if (!_zk_client->read(full_path, &data)) {
    DLOG("zk_error") << "zk read error, path: " << full_path;
    return false;
  }

  auto ip_ports = util::split_string(data, ":");
  if (ip_ports.size() != 2) {
    DLOG("zk_error") << "parse entry error, data: " << data;
    return false;
  }

  const auto& ip = ip_ports[0];
  auto port = util::to_uint32(ip_ports[1]);
  entry->first = ip;
  entry->second = port;
//  WLOG<< "server: " << _server << ", ip: " << ip << ", port: " << port;

  return true;
}

bool SmartFinderImpl::queryFromZk(std::set<std::string>* nodes,
                                  std::vector<ServerEntry> *server_list) {
  std::vector<std::string> dirs;
  if (!_zk_client->listDir(_path, &dirs)) {
    DLOG("zk_error") << "zk list error, path: " << _path;
    return false;
  }

  nodes->clear();
  server_list->clear();
  for (auto it = dirs.begin(); it != dirs.end(); ++it) {
    const auto& node = *it;
    ServerEntry entry;
    if (!parseEntry(node, &entry)) continue;

    nodes->insert(node);
    server_list->push_back(entry);
  }

  return true;
}

bool SmartFinderImpl::query(std::vector<ServerEntry> *server_list) {
  MutexGuard l(_mutex);
  for (auto it = _map.begin(); it != _map.end(); ++it) {
    const auto& entry = it->second;
    server_list->push_back(entry);
  }

  return true;
}

SmartFinder *CreateSmartFinder(const std::string& server,
                               const std::string& path) {
  return new SmartFinderImpl(server, path);
}

}
