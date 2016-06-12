#include "util/leveldb/db_key.h"
#include "util/leveldb/level_db.h"

DEF_string(db_dir, "/tmp/db", "test db for leveldb");

static void simpleTest(std::shared_ptr<util::LevelDB> db) {
  const std::string key("ssssssssssss");
  const std::string val("2222222222222");

  CHECK(db->write(key, val));
  std::string tmp;
  CHECK(db->read(key, &tmp));

  WLOG<< "read: " << tmp;
  CHECK_EQ(val, tmp);
}

static void testForKey() {
  util::KeyEncoder encoder(util::user);
  uint32 i32 = 23456;
  uint64 i64 = 123499766555;
  std::string str = "womendoushihaowawa";
  encoder.encodeUint32(i32);
  encoder.encodeUint64(i64);
  encoder.encodeBytes(str);
  auto key = encoder.asString();

  util::KeyDecoder decoder(util::user, key);
  CHECK_EQ(decoder.decodeUint32(), i32);
  CHECK_EQ(decoder.decodeUint64(), i64);
  CHECK_EQ(decoder.decodeString(), str);
}

int main(int argc, char** argv) {
  ccflag::init_ccflag(argc, argv);
  cclog::init_cclog(*argv);

  testForKey();

  std::shared_ptr<util::LevelDB> db(new util::LevelDB(FLG_db_dir));
  CHECK(db->init());

  simpleTest(db);

  while (1) {
    sys::sleep(1);
  }

  return 0;
}
