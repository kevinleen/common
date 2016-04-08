#pragma once

#include "base/base.h"
#include "../thrift_util/thrift_util.h"

namespace util {

class DB {
  public:
    virtual ~DB() = default;

    virtual bool del(const std::string& key) = 0;
    virtual bool get(const std::string &key, std::string *value) = 0;
    virtual bool set(const std::string& key, const std::string& value,
                     int32 expired = -1) = 0;

    template<class T>
    bool get(const std::string& key, T* ts);

    template<class T>
    bool set(const std::string& key, const T& ts, int expired = -1);

  protected:
    DB() = default;

  private:

    DISALLOW_COPY_AND_ASSIGN(DB);
};

template<class T>
bool DB::get(const std::string& key, T* ts) {
  std::string val;
  if (!get(key, &val)) return false;

  bool ret = util::parseFromString(val, ts);
  if (!ret) {
    *ts = T();
  }
  return ret;
}

template<class T>
bool DB::set(const std::string& key, const T& ts, int expired) {
  return set(key, ::util::serializeAsString(ts), expired);
}

std::shared_ptr<DB> createDB(const std::string &server_list, uint32 timedout);

}
