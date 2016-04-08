#pragma once

#include "../smart_finder.h"
#include "util/zkclient/zk_client.h"

namespace test {

class TestFinder {
  public:
    TestFinder() = default;
    ~TestFinder() = default;

    void start();

  private:
    std::unique_ptr<util::ZkClient> _zk_client;
    std::unique_ptr<util::SmartFinder> _finder;

    void doCheck();
    void queryAndPrint();

    void handleUp(const std::string& ip, uint16 port);
    void handleDown(const std::string& ip, uint16 port);

    DISALLOW_COPY_AND_ASSIGN(TestFinder);
};

}
