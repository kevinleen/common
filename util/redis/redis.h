#pragma once

#include <hiredis/hiredis.h>

#include "base/base.h"
#include "../thrift_util/thrift_util.h"

namespace util {

class Redis {
  public:
    Redis(const std::string& ip, uint32 port, uint32 timeout/* milliseconds */)
        : _ip(ip), _port(port), _timeout(timeout) {
      _ctx = NULL;

      this->connect();
    }

    ~Redis() {
      disconnect();
    }

    bool get(const std::string& key, std::string* value,
             std::string* err = NULL);

    bool set(const std::string& key, const std::string& value,
             std::string* err = NULL);

    bool set(const char* key, const char* value, std::string* err = NULL) {
      return this->set(std::string(key), std::string(value), err);
    }

    bool set(const std::string& key, const std::string& value, int32 expired, /* expired in seconds */
             std::string* err = NULL);

    template<class T>
    bool get(const std::string& key, T* value, std::string* err = NULL);

    template<class T>
    bool set(const std::string& key, const T& value, std::string* err = NULL);

    template<class T>
    bool set(const std::string& key, const T& value, int expired,
             std::string* err = NULL);

    bool del(const std::string& key, std::string* err = NULL);

    std::string cluster_nodes();

    // "|0~100 127.0.0.1:999,127.0.0.1:888|101~300 127.0.0.1:777,127.0.0.1:666|"
    std::string cluster_slots();

  private:
    bool connect();

    void disconnect() {
      if (_ctx == NULL) return;
      redisFree(_ctx);
      _ctx = NULL;
    }

    const std::string& on_error(int err);

  private:
    redisContext* _ctx;

    const std::string _ip;
    uint32 _port;
    uint32 _timeout;

    DISALLOW_COPY_AND_ASSIGN(Redis);
};

template<class T>
inline bool Redis::get(const std::string& key, T* ts, std::string* err) {
  std::string val;
  if (!this->get(key, &val, err)) {
    return false;
  }

  bool ret = util::parseFromString(val, ts);
  if (!ret) {
    TLOG("redis")<< "get, parseFromString error, key: " << key;
    *ts = T();
  }
  return ret;
}

template<class T>
inline bool Redis::set(const std::string& key, const T& ts, std::string* err) {
  std::string val;
  ::util::serializeAsString(ts, &val);

  return this->set(key, val, err);
}

template<class T>
inline bool Redis::set(const std::string& key, const T& ts, int32 expired,
                       std::string* err) {
  std::string val;
  ::util::serializeAsString(ts, &val);

  return this->set(key, val, expired, err);
}

typedef ::ThreadSafeHandlerWrapper<util::Redis> ThreadSafeRedis;

class ThreadSafeRedisDelegate : public ThreadSafeRedis::Delegate {
  public:
    ThreadSafeRedisDelegate(const std::string& ip, uint32 port, uint32 timeout)
        : _ip(ip), _port(port), _timeout(timeout) {
    }

    virtual ~ThreadSafeRedisDelegate() = default;

    virtual std::shared_ptr<util::Redis> create() {
      std::shared_ptr<util::Redis> redis(new Redis(_ip, _port, _timeout));
      return redis;
    }

  private:
    const std::string _ip;
    uint32 _port;
    uint32 _timeout;

    DISALLOW_COPY_AND_ASSIGN(ThreadSafeRedisDelegate);
};

struct RedisClient : public ThreadSafeRedis {
    RedisClient(const std::string& ip, uint32 port, uint32 timeout)
        : ThreadSafeRedis(new util::ThreadSafeRedisDelegate(ip, port, timeout)), _ip(
            ip), _port(port) {
    }

    const std::string& ip() const {
      return _ip;
    }
    uint16 port() const {
      return _port;
    }

    virtual ~RedisClient() = default;

  private:
    const std::string _ip;
    uint32 _port;
};

inline std::shared_ptr<RedisClient> CreateRedisClient(const std::string& ip,
                                                      uint32 port,
                                                      uint32 timeout) {
  return std::shared_ptr<RedisClient>(new RedisClient(ip, port, timeout));
}

}  // namespace util
