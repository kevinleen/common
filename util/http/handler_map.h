#pragma once

#include "base/base.h"

namespace http {

class HttpReply;
class HttpRequest;
struct Handler {
  public:
    explicit Handler(const std::string& uri_path)
        : uri(uri_path), arg(NULL) {
    }
    ~Handler() {
      if (free_cb) free_cb(arg);
    }

    const std::string uri;
    void* arg;
    std::function<void(void*)> free_cb;

    // not threadsafe.
    std::function<void(const HttpRequest&, HttpReply*)> cb;
};

// thread safe.
class HandlerMap {
  public:
    explicit HandlerMap(Handler* handler = NULL) {
      addHandler(handler);
    }
    ~HandlerMap() {
      WriteLockGuard l(_rw_lock);
      STLMapClear(&_handlers);
    }

    void addHandler(Handler* handler) {
      WriteLockGuard l(_rw_lock);
      if (handler != NULL) {
        const auto& uri = handler->uri;
        WLOG<< "registe uri: " << uri;
        _handlers[uri] = handler;
      }
    }

    Handler* findHandlerByUri(const std::string& uri) const {
      ReadLockGuard l(_rw_lock);
      auto it = _handlers.find(uri);
      if (it == _handlers.end()) {
        ELOG<< "not find uri: " << uri;
        return NULL;
      }
      return it->second;
    }

  private:
    mutable RwLock _rw_lock;
    std::unordered_map<std::string, Handler*> _handlers;

    DISALLOW_COPY_AND_ASSIGN(HandlerMap);
};

}
