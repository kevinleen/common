#pragma once

#include <evhtp.h>
#include "http_message.h"

namespace http {

class HttpRequest : public HttpAbstruct {
  public:
    explicit HttpRequest(evhtp_request_t* req)
        : _request(req) {
      CHECK_NOTNULL(req);
    }
    virtual ~HttpRequest() = default;

    bool parse() {
      return parseHeader() && parseBody();
    }

  private:
    evhtp_request_t* _request;

    KVMap _query_map;
    bool parseHeader();
    bool parseBody();

    void getQuery(evhtp_query_t *query, KVMap* kv_map);

    const std::string clientIp();

    DISALLOW_COPY_AND_ASSIGN(HttpRequest);
};

}
