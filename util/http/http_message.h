#pragma once

#include "base/base.h"

namespace http {

typedef std::map<const std::string, std::string> KVMap;

class HttpHeader {
  public:
    HttpHeader() = default;
    ~HttpHeader() = default;

    const std::string acceptEnCoding() const {
      return findKey("accept-encoding");
    }

    const std::string findKey(const std::string& key) const {
      auto it = _header_map.find(key);
      if (it == _header_map.end()) {
        return "";
      }
      return it->second;
    }

    const KVMap& header() const {
      return _header_map;
    }

    KVMap* getHeader() {
      return &_header_map;
    }

  private:
    KVMap _header_map;

    DISALLOW_COPY_AND_ASSIGN(HttpHeader);
};

class HttpBody {
  public:
    HttpBody() = default;
    ~HttpBody() = default;

    void setBody(const std::string& body) {
      _body = body;
    }
    void setBody(unsigned char* body, uint32 len) {
      _body.assign((char*) body, len);
    }

    const std::string* body() const {
      return &_body;
    }

  private:
    std::string _body;

    DISALLOW_COPY_AND_ASSIGN(HttpBody);
};

class HttpAbstruct {
  public:
    virtual ~HttpAbstruct() = default;

    HttpHeader* getHttpHeader() {
      return _http_header.get();
    }
    const HttpHeader& httpHeader() const {
      return *_http_header;
    }

    HttpBody* getHttpBody() {
      return _http_body.get();
    }
    const HttpBody& httpBody() const {
      return *_http_body;
    }

  protected:
    HttpAbstruct() {
      _http_header.reset(new HttpHeader);
      _http_body.reset(new HttpBody);
    }

    std::unique_ptr<HttpHeader> _http_header;
    std::unique_ptr<HttpBody> _http_body;

  private:
    DISALLOW_COPY_AND_ASSIGN(HttpAbstruct);
};

}
