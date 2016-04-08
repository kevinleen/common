#include "test_finder.h"

DEF_string(serv_name, "", "");
DEF_string(serv_path, "", "");

namespace test {

void TestFinder::queryAndPrint() {

}

void TestFinder::doCheck() {
  queryAndPrint();
}

void TestFinder::start() {
  CHECK(!FLG_serv_name.empty());
  CHECK(!FLG_serv_path.empty());

  _zk_client.reset();

  _finder.reset(util::CreateSmartFinder(FLG_serv_name, FLG_serv_path));

  while (true) {
    doCheck();

    sys::sleep(3);
  }
}

}
