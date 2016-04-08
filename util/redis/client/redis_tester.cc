#include "redis_tester.h"

DEF_string(server_list,
           "192.168.1.252:9980;192.168.1.252:9981;192.168.1.252:9982",
           "server list for redis cluster");
DEF_uint32(redis_cluster_timedout, 1000, "time out for redis cluster");

namespace test {

void RedisTester::start() {
  LOG<<"server list: " << FLG_server_list;
  _db = util::createDB(FLG_server_list, FLG_redis_cluster_timedout);

  doSearch();
}

void RedisTester::doSearch() {
  int z = 0;
  sys::timer timer;
  while (true) {
    std::string key = "key" + util::to_string(z);
    std::string value = "value" + util::to_string(timer.us());
    if (!_db->set(key, value)) {
      LOG<< "set " << key <<" fail";
    } else {
      LOG <<"set " << key << " OK";
    }

    std::string ret;
    if (!_db->get(key, &ret)) {
      LOG<< "get " << key << " value: " << value;
    } else {
      LOG <<"get " << key << " value: " << value;
    }

    CHECK_EQ(value, ret)
    << "value: " << value << ", ret: " << ret;

    if (!_db->del(key)) {
      LOG<< "del " << key << " fail";
    } else {
      LOG <<"del " << key << " OK";
    }

    sys::sleep(1);
    ++z;
  }

}
}
