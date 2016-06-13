#include "search_handler.h"

#include "util/http/http_request.h"
#include "util/http/http_reply.h"

namespace test {

void SearchHandler::onMessage(const http::HttpRequest& req,
                              http::HttpReply* reply) {
  reply->getHttpBody()->setBody("hello world!");
}

}
