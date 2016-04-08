#include "redis_tester.h"

int main(int argc, char* argv[]) {
  ccflag::init_ccflag(argc, argv);
  cclog::init_cclog(*argv);

  test::RedisTester redis_tester;
  redis_tester.start();
  return 0;
}
