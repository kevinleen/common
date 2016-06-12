#include "zk_action.h"
#include <zookeeper/zookeeper.h>

namespace util {

class ZkAction;
class ZkClient {
  public:
    explicit ZkClient(std::shared_ptr<ZkAction> action);
    virtual ~ZkClient();

    bool init(const std::string& zk_server = "");

    void markDown() {
      MutexGuard l(_mutex);
      _connected = false;
    }
    bool isConnected() {
      MutexGuard l(_mutex);
      return _connected;
    }

    bool remove(const std::string& path);
    bool create(const std::string& path, const std::string& value,
                bool permanent = false);
    bool create(const std::string& path, bool permanent) {
      return create(path, "", permanent);
    }

    bool exist(const std::string& path);
    bool watchDir(const std::string& path);

    bool read(const std::string& path, std::string* value);
    bool write(const std::string& path, const std::string& value);

    bool listDir(const std::string& path, std::vector<std::string>* children);

    void handleEvent(int type, int state, const std::string& path);

  private:
    std::string _server;
    int _timeout;  // seconds.
    std::shared_ptr<ZkAction> _action;

    bool _connected;
    zhandle_t* _zh_handle;

    Mutex _mutex;
    SyncEvent _event;
    std::unique_ptr<StoppableThread> _thread;

    void doConnect();
    bool reconnect();

    void handleExpired();
    void handleExpiredInternal();

    void handleConnected();

    DISALLOW_COPY_AND_ASSIGN(ZkClient);
};
}
