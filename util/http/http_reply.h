#pragma once

#include "http_message.h"

namespace http {

class HttpReply : public HttpAbstruct {
  public:
    HttpReply() = default;
    virtual ~HttpReply() = default;

  private:
    DISALLOW_COPY_AND_ASSIGN(HttpReply);
};

}
