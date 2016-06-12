#pragma once

#include "elastic_search.h"

namespace http {
class HttpClient;
}

namespace util {

class ESNode : public ElasticSearch {
  public:
    ESNode(const std::string& index, const std::string& ip, uint16 port);
    virtual ~ESNode();

    bool init();

    uint16 port() const {
      return _port;
    }
    const std::string& server() const {
      return _ip;
    }

    virtual bool post(const std::string& types, const std::string& uri,
                      const std::string& condition, Json::Value* reply);
    virtual bool post(const std::string& types, const Json::Value& request,
                      Json::Value* reply);

  private:
    const std::string _ip;
    uint16 _port;

    std::unique_ptr<http::HttpClient> _http_client;
    const std::string toUri(const std::string& types) const;
    const std::string toUri(const std::string& types,
                            const std::string& uri) const {
      return toUri(types) + "/" + uri;
    }

    bool parseJson(const std::string& data, Json::Value* reply) const;

    DISALLOW_COPY_AND_ASSIGN(ESNode);
};

}

