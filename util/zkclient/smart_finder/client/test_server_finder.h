#pragma once

#include "util/server_finder.h"

namespace test {

class TestServerFinder {
  public:
    TestServerFinder() = default;
    ~TestServerFinder() = default;

    void start();

  private:
    std::unique_ptr<util::ServerFinder> _serv_finder;

    DISALLOW_COPY_AND_ASSIGN(TestServerFinder);
};

}
