#pragma once

#include "base/base.h"
#include <evhtp.h>

namespace http {
class Handler;
class HttpServer;

class HttpScheduler {
  public:
    explicit HttpScheduler(http::HttpServer* hs)
        : _server(hs) {
      CHECK_NOTNULL(hs);
    }
    ~HttpScheduler() = default;

    void dispatch(evhtp_request_t* req);

  private:
    http::HttpServer* _server;

    void initHeader(evhtp_request_t* req) const;
    void addEntry(evhtp_request_t* req, const std::string& key,
                  const std::string& v) const;
    bool compress(const std::string& src, std::string* dst) const;

    const std::string GMTime() const;

    void handleError(evhtp_request_t* req,
                     uint32 status = EVHTP_RES_BADREQ) const;

    http::Handler* findHandler(const std::string& uri_path);

    DISALLOW_COPY_AND_ASSIGN(HttpScheduler);
};

}
