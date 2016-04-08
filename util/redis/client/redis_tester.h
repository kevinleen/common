#pragma once

#include "../db.h"

namespace test {

class RedisTester {
  public:
    RedisTester() = default;
    ~RedisTester() = default;

    void start() ;

private:
    std::shared_ptr<util::DB> _db;
    void doSearch();

    DISALLOW_COPY_AND_ASSIGN(RedisTester);
};

}
