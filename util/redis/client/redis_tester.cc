#include "redis_tester.h"

//DEF_string(server_list, "192.168.1.235:9000", "server list for redis cluster");
DEF_string(
    server_list,
    "192.168.1.252:9977;192.168.1.252:9978;192.168.1.252:9979,192.168.1.252:9980;192.168.1.252:9981;192.168.1.252:9982",
    "server list for redis cluster");
DEF_uint32(redis_cluster_timedout, 1000, "time out for redis cluster");
DEF_uint32(redis_expired, 1, "expired time of redis");
DEF_string(key, "test", "key for redis");

namespace test {

void RedisTester::start() {
  LOG<<"server list: " << FLG_server_list;
  _db = util::createDB(FLG_server_list, FLG_redis_cluster_timedout);

  doSearch();
}

void RedisTester::doSearch() {
  int z = 0;
  sys::timer timer;
  int index = 0;
  while (true) {
    std::string key = "key" + util::to_string(z);
    std::string value = "value" + util::to_string(timer.us());
    if (!_db->set(key, value)) {
      LOG<< "set " << key <<" fail";
    } else {
      LOG <<"set " << key << " OK";
    }

    if (!getKey(key, value)) {
      ++index;
    }
//    sleep(FLG_redis_expired + 1);
//    getKey(key, value);
    if (!_db->del(key)) {
      LOG<< "del " << key << " fail";
    } else {
      LOG <<"del " << key << " OK";
    }

    sleep(1);
    ++z;
//    if(index > 3)
//      break;
  }
}

bool RedisTester::getKey(const std::string &key, const std::string &value) {
  std::string ret;
  if (!_db->get(key, &ret)) {
    ELOG<<"get " << key << " error ";
    return false;
  }

  LOG<<"get " << key << " value: " << ret;
  if (value != ret) ELOG<<"value: " << value << ", ret: " << ret;
  return true;
}

}
