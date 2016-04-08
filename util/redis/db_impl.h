#pragma once

#include "db.h"

namespace util {

typedef std::pair<std::string, uint32> IpPort;

typedef std::vector<IpPort> ServerMap;

class RedisClient;
typedef std::map<IpPort, std::shared_ptr<RedisClient>> RedisTable;

class DBImpl : public DB {
  public:
    explicit DBImpl(const std::string &server_list, uint32 timedout);
    ~DBImpl() = default;

    virtual bool get(const std::string &key, std::string *value);

    virtual bool set(const std::string& key, const std::string& value,
                     int32 expired = -1);

    virtual bool del(const std::string& key);

    template<class T>
    bool get(const std::string& key, T* value, std::string* err = NULL);

    template<class T>
    bool set(const std::string& key, const T& value, std::string* err = NULL);

    template<class T>
    bool set(const std::string& key, const T& value, int expired,
             std::string* err = NULL);

  private:

    class SlotTable {
      public:
        SlotTable(const std::vector<IpPort> &ip_ports, uint32 timedout);
        ~SlotTable();

        bool get(int32 slot, std::string *ip, uint32 *port);
        void update(int32 slot, const std::string &ip, uint32 port);
        void remove(const std::string &ip, uint32 port);

        std::shared_ptr<RedisClient> createRedisClintIfNotExists(
            const IpPort& ip_port);

        void initSlotTable();

      private:
        ServerMap _table;
        bool checkSlotServerExists(int32 slot);
        Mutex _mutex;

        std::vector<IpPort> _server_list;
        void getRandomIpPort(int32 slot, std::string *ip, uint32 *port);

        uint32 timedout;
        RedisTable _redis_table;
//        std::unique_ptr<StoppableThread> _slot_reflesh_thread;
        bool initClusterNodes(const std::string &cluster_nodes);

        DISALLOW_COPY_AND_ASSIGN(SlotTable);
    };

    std::unique_ptr<SlotTable> _slot_table;
    void init(const std::string &server_list, uint32 timedout);
    void createSlotTable(const std::vector<IpPort> &ip_ports, uint32 timedout);

    bool getInternal(const std::string &key, std::string *value,
                     std::string *err);
    bool setInternal(const std::string &key, const std::string &value,
                     std::string *err, int32 expired = -1);
    bool delInternal(const std::string &key, std::string *err);

    std::shared_ptr<RedisClient> getRedisClient(const std::string &key);

    bool isMovedErr(const std::string &err) const;
    bool handleErrorMsg(const std::string &err);
    bool parseMovedErrorMsg(const std::string &err, uint32 *slot_moved,
                            std::string *ip_moved, uint32 *port_moved);

    DISALLOW_COPY_AND_ASSIGN(DBImpl);
};
}
