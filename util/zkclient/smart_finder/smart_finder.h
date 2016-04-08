#pragma once

#include "base/base.h"

namespace util {

typedef std::pair<std::string, uint32> ServerEntry;

class SmartFinder {
  public:
    virtual ~SmartFinder() = default;

    virtual bool init() = 0;

    virtual bool query(std::vector<ServerEntry> *server_list) = 0;

    typedef std::function<void(const std::string&, uint32)> callback;
    void setUpClosure(callback up_closure) {
      _up_closure = up_closure;
    }
    void setDownClosure(callback down_closure) {
      _down_closure = down_closure;
    }

  protected:
    const std::string _server;
    const std::string _path;

    callback _up_closure;
    callback _down_closure;

    SmartFinder(const std::string& server, const std::string& path)
        : _server(server), _path(path) {
      CHECK(!_server.empty());
      CHECK(!_path.empty());
    }

  private:

    DISALLOW_COPY_AND_ASSIGN(SmartFinder);
};

// server: service name.
// path:   zookeeper path for this service.
SmartFinder *CreateSmartFinder(const std::string& server,
                               const std::string& path);
}

