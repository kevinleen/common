#pragma once

#include "base/base.h"

namespace inv {

class WatchDog {
  public:
    class Server {
      public:
        virtual ~Server() {
        }

        virtual void loop() = 0;
    };

    WatchDog(const std::string& stop_file, Server* server)
        : _stop_file(stop_file), _server(server) {
      CHECK(!stop_file.empty());
      CHECK_NOTNULL(server);
    }
    ~WatchDog() {
    }

    void watch(int argc, char* argv[]);

  private:
    const std::string _stop_file;
    std::unique_ptr<Server> _server;

    void removeStopFile() const;
    bool stopFileExist() const;

    DISALLOW_COPY_AND_ASSIGN(WatchDog);
};

}
