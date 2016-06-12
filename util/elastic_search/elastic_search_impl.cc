#include "es_node.h"
#include "elastic_search_impl.h"

#include "util/server_finder.h"
#include "util/zkclient/smart_finder/smart_finder.h"

#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

DEF_string(es_zk, "elastic_search", "es name in zookeeper");

namespace util {

ElasticSearch* CreateElasticSearch(const std::string& index,
                                   const std::string& zk_path) {
  if (index.empty() || zk_path.empty()) return NULL;

  std::unique_ptr<ElasticSearchImpl> es;
  es.reset(new ElasticSearchImpl(index, zk_path));
  if (!es->init()) {
    ELOG<< "ES init error, zk_path: " << zk_path;
    return NULL;
  }

  WLOG<< "create es successfully, index: " << index << " zk: " << zk_path;
  return es.release();
}

ElasticSearchImpl::~ElasticSearchImpl() = default;
ElasticSearchImpl::ElasticSearchImpl(const std::string& index,
                                     const std::string& zk_path)
    : ElasticSearch(index), _zk_path(zk_path) {
  CHECK(!zk_path.empty());
  WLOG<< "ES index: " << index << " zk_path: " << zk_path;
}

bool ElasticSearchImpl::init() {
  _finder.reset(new ServerFinder(CreateSmartFinder(FLG_es_zk, _zk_path)));
  if (!_finder->init()) {
    ELOG<< "create server finder error, index: " << _index;
    return false;
  }

  _nodes.reset(new ESThreadSafeNode(_index, _finder, this));

  return true;
}

std::shared_ptr<ESNode> ElasticSearchImpl::ESFactory::create() {
  auto server = _finder->next_server();

  auto& ip = server.first;
  uint32 port = server.second;
  WLOG<< "es find server, ip: " << ip << " port: " << port;
  std::shared_ptr<ESNode> node(new ESNode(_index, ip, port));
  if (!node->init()) {
    node.reset();
  }

  _finder->add_down_callback(
      ip,
      port,
      std::bind(&ElasticSearchImpl::onRemove, _impl, pthread_self(), std::_1,
                std::_2));
  return node;
}

void ElasticSearchImpl::onRemove(pthread_t tid, const std::string& ip,
                                 uint32 port) {
  WLOG<< "es remove, tid: " << tid << " " << ip << ":" << port;
  _nodes->erase(tid);
}

bool ElasticSearchImpl::post(const std::string& types, const std::string& uri,
                             const std::string& condition, Json::Value* reply) {
  auto entry = getEntry();
  if (entry == NULL) {
    ELOG<< "can't find entry, types: " << types;
    return false;
  }

  sys::timer timer;
  bool ret = entry->post(types, uri, condition, reply);
  uint32 us = timer.ms();
  if (us > 50) {
    DLOG("timedout") << "ES post expired, used: " << us;
  }

  if (!ret) {
    ELOG<< "post error,  server: " << entry->server() << " uri: " << uri;
  }

  return ret;
}

bool ElasticSearchImpl::post(const std::string& types,
                             const Json::Value& request, Json::Value* reply) {
  auto entry = getEntry();
  if (entry == NULL) {
    ELOG<< "can't find entry, types: " << types;
    return false;
  }

  sys::timer timer;
  bool ret = entry->post(types, request, reply);
  uint32 us = timer.ms();
  if (us > 50) {
    DLOG("timedout") << "ES post expired, used: " << us;
  }

  if (!ret) {
    ELOG<< "post error,  server: " << entry->server() << " types: " << types;
  }

  return ret;
}

}
