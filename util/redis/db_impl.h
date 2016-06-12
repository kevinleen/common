#pragma once

#include "db.h"

namespace util {
class RedisClient;

class SlotTable;
class DBImpl : public DB {
  public:
    explicit DBImpl(uint32 timedout);
    virtual ~DBImpl();

    bool init(const std::string &server_list);

  private:
    const uint32 _timedout;
    virtual bool del(const std::string& key);
    virtual bool get(const std::string &key, std::string *value);
    virtual bool set(const std::string& key, const std::string& value,
                     int32 expired = -1);

    typedef std::unordered_map<std::string, std::shared_ptr<RedisClient>> NodeMap;

    NodeMap _nodes;
    Mutex _node_mutex;
    void updateNodes(const NodeMap& nodes);
    void eraseNode(const std::string& ip, uint16 port);
    const std::string toServer(const std::string& ip, uint16 port) const {
      return ip + ":" + util::to_string(port);
    }

    std::shared_ptr<RedisClient> randomRedis();
    std::shared_ptr<RedisClient> getRedis(const std::string& server);
    std::shared_ptr<RedisClient> getRedis(const std::string& ip, uint16 port) {
      return getRedis(toServer(ip, port));
    }

    Mutex _slot_mutex;
    std::vector<std::shared_ptr<RedisClient>> _slot_map;
    void eraseSlot(uint32 slot);
    void updateSlot(uint32 slot, const std::string& ip, uint16 port);

    bool initCluster(const std::string& ip, uint16 port);

    std::shared_ptr<RedisClient> getRedisClient(const std::string &key);
    bool setInternal(const std::string &key, const std::string &value,
                     std::string *err, int32 expired = -1);

    bool isMovedErr(const std::string &err) const;
    bool handleErrorMsg(const std::string &err);
    bool parseMovedErrorMsg(const std::string &err, uint32 *slot,
                            std::string *ip, uint16 *port) const;

    DISALLOW_COPY_AND_ASSIGN(DBImpl);
};
}
