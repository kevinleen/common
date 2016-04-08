#pragma once

#include "base/base.h"
#include "proto/gen-cpp/cassandra_types.h"
#include "proto/gen-cpp/cassandra_constants.h"
#include "proto/gen-cpp/Cassandra.h"
#include "../thrift_util/thrift_util.h"
#include "util/server_finder.h"

namespace cass = org::apache::cassandra;

namespace util {

class Cassandra {
  public:
    Cassandra(const std::string& ip, uint16 port, uint32 timeout/*ms*/,
              const std::string& keyspace, const std::string& column_family);

    ~Cassandra();

    bool get(const std::string& key, const std::string& column,
             std::string* value);

    bool set(const std::string& key, const std::string& column,
             const std::string& value, int32 ttl = -1);

    void remove(const std::string& key, const std::string& column);

    template<class T>
    bool get(const std::string& key, const std::string& column,
             T* value);

    template<class T>
    bool set(const std::string& key, const std::string& column,
             const T& value, int32 ttl = -1);

  private:
    bool connect();

  private:
    std::string _ip;
    uint16 _port;
    uint32 _timeout;

    std::string _keyspace;
    std::string _column_family;

    std::shared_ptr<cass::CassandraClient> _client;

    DISALLOW_COPY_AND_ASSIGN(Cassandra);
};

template<class T>
inline bool Cassandra::get(const std::string& key, const std::string& column,
                           T* ts) {
    std::string val;
    if (!this->get(key, column, &val)) {
        return false;
    }

    bool ret = ::util::parseFromString(val, ts);
    if (!ret) {
        TLOG("cassandra") << "get, parseFromString error, key: " << key
            << ", column: " << column;
        *ts = T();
    }
    return ret;
}

template<class T>
inline bool Cassandra::set(const std::string& key, const std::string& column,
                           const T& ts, int32 ttl) {
    std::string val;
    ::util::serializeAsString(ts, &val);

    bool ret = ttl == -1 ? set(key, column, val) : set(key, column, val, ttl);
    if (!ret) {
        return false;
    }

    return true;
}

class CassandraDelegate : public ThreadSafeDelegate<Cassandra> {
  public:
    CassandraDelegate(std::shared_ptr<ServerFinder> finder,
                      ThreadSafeClient<Cassandra>* client,
                      uint32 timeout,
                      const std::string& keyspace,
                      const std::string& column_family)
        : ThreadSafeDelegate<Cassandra>(finder, client), _timeout(timeout),
          _keyspace(keyspace), _column_family(column_family) {
    }

    virtual ~CassandraDelegate() = default;

    virtual std::shared_ptr<Cassandra> create();

  private:
    uint32 _timeout;
    std::string _keyspace;
    std::string _column_family;

    DISALLOW_COPY_AND_ASSIGN(CassandraDelegate);
};

struct CassandraClient : public ThreadSafeClient<Cassandra> {
    CassandraClient(std::shared_ptr<ServerFinder> finder,
                    uint32 timeout,
                    const std::string& keyspace,
                    const std::string& column_family)
        : ThreadSafeClient<Cassandra>(
            new util::CassandraDelegate(finder, this, timeout,
                                        keyspace, column_family)) {
    }

    virtual ~CassandraClient() = default;
};

} // namespace util
