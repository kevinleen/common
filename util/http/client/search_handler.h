#pragma once

#include "base/base.h"

namespace http {
class HttpRequest;
class HttpReply;
}

namespace test {

class SearchHandler {
  public:
    SearchHandler() = default;
    ~SearchHandler() = default;

    void onMessage(const http::HttpRequest& req, http::HttpReply* reply);

  private:
    DISALLOW_COPY_AND_ASSIGN(SearchHandler);
};

}

