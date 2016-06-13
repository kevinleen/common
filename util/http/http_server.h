#pragma once

#include <evhtp.h>
#include "handler_map.h"

namespace http {

class HttpScheduler;
class HttpServer {
  public:
    explicit HttpServer(HandlerMap* handler_map);
    ~HttpServer();

    bool start();
    void stop();

    HandlerMap* getHandlerMap() const {
      return _handlers.get();
    }

  private:
    std::unique_ptr<HandlerMap> _handlers;
    std::unique_ptr<HttpScheduler> _scheduler;

    evbase_t* _ev_base;
    evhtp_t* _ev_http;

    bool initSSL();

    DISALLOW_COPY_AND_ASSIGN(HttpServer);
};

}

