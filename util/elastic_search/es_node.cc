#include "es_node.h"
#include "rpc/http/http_client.h"

#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>

DEF_uint32(es_timedout, 100, "timedout for es");

namespace util {

ESNode::~ESNode() = default;
ESNode::ESNode(const std::string& index, const std::string& ip, uint16 port)
    : ElasticSearch(index), _ip(ip), _port(port) {
}

bool ESNode::init() {
  _http_client.reset(http::createHttpClient(_ip, _port, FLG_es_timedout));
  if (!_http_client->init()) {
    ELOG<< "es http client error, ip: " << _ip << " port: " << _port;
    return false;
  }

  return true;
}

const std::string ESNode::toUri(const std::string& types) const {
  return "/" + _index + "/" + types;
}

bool ESNode::parseJson(const std::string& data, Json::Value* reply) const {
  Json::Reader reader;
  if (!reader.parse(data, *reply)) {
    ELOG<< "json parse error, ret: " << data;
    return false;
  }

  return true;
}

bool ESNode::post(const std::string& types, const std::string& uri,
                  const std::string& condition, Json::Value* reply) {
  if (_http_client == NULL) return false;

  sys::timer timer;
  std::string url(toUri(types));
  if (!uri.empty()) {
    url += "/" + uri;
  }

  std::string val;
  auto ret = _http_client->post(url, condition, &val);
  uint64 used = timer.ms();
  if (used > 30) {
    DLOG("timedout") << "es expired, used: " << used;
  }

  if (!ret) {
    ELOG<< "http post error, url: " << url;
    return false;
  }

  return parseJson(val, reply);
}

bool ESNode::post(const std::string& types, const Json::Value& request,
                  Json::Value* reply) {
  sys::timer timer;
  std::string val;
  auto ret = _http_client->post(toUri(types), &val);
  uint64 used = timer.ms();
  if (used > 30) {
    DLOG("timedout") << "es expired, used: " << used;
  }

  return ret ? parseJson(val, reply) : false;
}
}
