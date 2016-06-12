#pragma once

#include "smart_finder.h"

namespace util {

class FinderAction;
class SmartFinderImpl : public SmartFinder {
  public:
    SmartFinderImpl(const std::string& server, const std::string &path);
    virtual ~SmartFinderImpl();

    void onChild();
    void onAbort();

    void onChanged();
    void onEstablished();

    // callback for zk timer thread.
    bool onCheck();

  private:
    std::unique_ptr<ZkClient> _zk_client;
    std::shared_ptr<FinderAction> _action;

    virtual bool init(const std::string& zk_server);
    virtual bool query(std::vector<ServerEntry> *server_list);

    bool _watched;
    Mutex _watch_mutex;

    Mutex _mutex;
    std::map<std::string, ServerEntry> _map;

    bool popEntry(const std::string& path, ServerEntry* entry);
    void handleUp(const std::vector<std::string>& nodes);
    void handleDown(const std::vector<std::string>& nodes);

    bool watchDir(const std::string& full_path);
    bool parseEntry(const std::string& data, ServerEntry* entry);
    bool queryFromZk(std::set<std::string>* nodes,
                     std::vector<ServerEntry> *server_list);

    void diff(const std::set<std::string>& nodes,
              std::vector<std::string>* added,
              std::vector<std::string>* removed);

    DISALLOW_COPY_AND_ASSIGN(SmartFinderImpl);
};

}
