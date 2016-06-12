#pragma once

#include "base/base.h"

namespace util {
class ElasticSearch;
}

namespace test {

class ESClient {
  public:
    explicit ESClient(const std::string& es_types);
    ~ESClient();

    bool init();

    void testString();
    void testJson();

  private:
    const std::string _types;
    std::unique_ptr<util::ElasticSearch> _es;

    DISALLOW_COPY_AND_ASSIGN(ESClient);
};

}
