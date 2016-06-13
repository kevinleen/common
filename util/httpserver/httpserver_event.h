#pragma once

#include <sys/queue.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <event2/bufferevent_ssl.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "helper.h"

namespace inv {
class HttpServer {
  public:
    HttpServer() {
      cli_ip = "";
      body = "";
      path = "";
      query.clear();

      pid = pthread_self();
    }

    int httpget(std::string& out) {
      return getJsonStr(out, query, body, path, cli_ip, head);
    }

  private:
    pthread_t pid;
    std::string cli_ip;
    std::string body;
    std::string path;
    std::map<std::string, std::string> query;
    std::map<std::string, std::string> head;

    DISALLOW_COPY_AND_ASSIGN(HttpServer);
};

}
