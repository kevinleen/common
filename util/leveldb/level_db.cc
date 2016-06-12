#include "level_db.h"

namespace util {

LevelDB::~LevelDB() = default;
LevelDB::LevelDB(const std::string& db_dir)
    : _db_dir(db_dir) {
  CHECK(!db_dir.empty());

  WLOG<< "the dir of leveldb: " << db_dir;
}

bool LevelDB::init() {
  leveldb::Options option;
  option.create_if_missing = true;
  option.error_if_exists = false;
  option.max_open_files = 10240;

  leveldb::DB *db;
  auto status = leveldb::DB::Open(option, _db_dir, &db);
  if (!status.ok() || db == NULL) {
    ELOG<< "opendb error: " << status.ToString();
    return false;
  }

  _db.reset(db);
  return true;
}

bool LevelDB::read(const std::string& key, std::string* val) {
  CHECK_NOTNULL(val);

  leveldb::ReadOptions option;
  leveldb::Slice skey(key.data(), key.size());
  auto ret = _db->Get(option, skey, val);
  if (!ret.ok()) {
    ELOG<< "db get error, key: " << key << ",\t" << ret.ToString();
    return false;
  }

  return true;
}

bool LevelDB::write(const std::string& key, const std::string& val) {
  leveldb::WriteOptions option;
  option.sync = true;
  leveldb::Slice skey(key.data(), key.size());
  leveldb::Slice sval(val.data(), val.size());
  auto ret = _db->Put(option, skey, sval);
  if (!ret.ok()) {
    ELOG<< "db write error, key: " << key << ",\t" << ret.ToString();
    return false;
  }

  return true;
}

void LevelDB::remove(const std::string& key) {
  leveldb::WriteOptions option;
  option.sync = true;
  leveldb::Slice skey(key.data(), key.size());
  auto ret = _db->Delete(option, skey);
  if (!ret.ok()) {
    ELOG<<"db delete error, key: " << key;
  }
}

}
