#include "es_client.h"

#include "util/elastic_search/elastic_search.h"

#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>

DEF_string(es_zk_path, "", "zookeeper path for elastic search");
DEF_string(es_index, "", "index for elastic search");

DEF_string(es_uri, "_search", "uri for es");

namespace test {

ESClient::~ESClient() = default;
ESClient::ESClient(const std::string& es_types)
    : _types(es_types) {
  CHECK(!es_types.empty());

  WLOG<< "es types: " << es_types;
}

bool ESClient::init() {
  CHECK(!FLG_es_zk_path.empty() && !FLG_es_index.empty());

  WLOG<< "es zk path: " << FLG_es_zk_path;
  WLOG<< "es index: " << FLG_es_index;
  _es.reset(util::CreateElasticSearch(FLG_es_index, FLG_es_zk_path));
  if (_es == NULL) {
    ELOG<< "create es error";
    return false;
  }

  // todo: more

  return true;
}

void ESClient::testString() {
  WLOG<< "es uri: " << FLG_es_uri;
  std::string uri(FLG_es_uri);
  std::string cond;  // todo:

  Json::Value reply;
  if (!_es->post(_types, uri, cond, &reply)) {
    ELOG<< "es post string error";
    return;
  }

  LOG<< "string result: " << reply.toStyledString();
}

void ESClient::testJson() {
  Json::Value request;
  // todo: build request.

  Json::Value reply;
  if (!_es->post(_types, request, &reply)) {
    ELOG<< "es post json error, req: " << request.toStyledString();
    return;
  }

  LOG<<"json result: " << reply.toStyledString();
}

}
