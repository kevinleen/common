#include "test_finder.h"
#include "test_server_finder.h"

DEF_bool(server_finder, false, "");

int main(int argc, char* argv[]) {
  ccflag::init_ccflag(argc, argv);
  cclog::init_cclog(*argv);

  if (FLG_server_finder) {
      test::TestServerFinder finder;
      finder.start();
      return -1;
  }

  test::TestFinder finder;
  finder.start();

  return 0;
}

