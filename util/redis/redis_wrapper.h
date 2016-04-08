#pragma once

#include "redis.h"

namespace util {

class RedisCacheWrapper {
  public:
    virtual ~RedisCacheWrapper() {
    }

    virtual bool init() = 0;

  protected:
    RwLock _rw_lock;
    shared_ptr<util::Redis> _redis;
    RedisCacheWrapper() {
    }

    RwLock* rw_lock() {
      return &_rw_lock;
    }

    template<typename T>
    bool find(const std::string& key, T* t) {
      ReadLockGuard l(_rw_lock);
      return findUnLock(key, t);
    }
    template<typename T>
    bool findUnLock(const std::string& key, T* t) {
      return _redis->get(key, t);
    }

    template<typename T>
    void updateUnLock(const std::string& key, const T& t, uint32 expire = -1) {
      if (!_redis->set(key, t, expire)) {
        ELOG<<"redis error, key: " << key;
      }
    }
    template<typename T>
    void update(const std::string& key, const T& t, uint32 expire = -1) {
      WriteLockGuard l(_rw_lock);
      updateUnLock(key, t, expire);
    }

    private:
    DISALLOW_COPY_AND_ASSIGN(RedisCacheWrapper);
  };
}
