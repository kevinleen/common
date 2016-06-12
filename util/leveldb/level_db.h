#pragma once

#include "base/base.h"
#include <leveldb/db.h>

namespace util {

// threasafe,
//    can be called from any thread.
class LevelDB {
  public:
    explicit LevelDB(const std::string& db_dir);
    ~LevelDB();

    bool init();

    bool read(const std::string& key, std::string* val);
    bool write(const std::string& key, const std::string& val);

    void remove(const std::string& key);

    template<typename T>
    bool read(const std::string& key, T* t) {
      CHECK_NOTNULL(t);
      std::string val;
      if (!read(key, &val)) return false;

      bool ret = parseFromString(val, t);
      if (!ret) {
        *t = T();
      }
      return ret;
    }

    template<typename T>
    bool write(const std::string& key, const T& t) {
      return write(key, serializeAsString(t));
    }

  private:
    const std::string _db_dir;
    std::unique_ptr<leveldb::DB> _db;

    DISALLOW_COPY_AND_ASSIGN(LevelDB);
};

}

