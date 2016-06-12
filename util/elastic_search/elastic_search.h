#pragma once

#include "base/base.h"

namespace Json {
class Value;
}

namespace util {

// git book: http://es.xiaoleilu.com/
// in elastic search.
//    index: like datebase.
//    types: like table.
class ElasticSearch {
  public:
    virtual ~ElasticSearch() = default;

    // dsl:
    virtual bool post(const std::string& types, const Json::Value& request,
                      Json::Value* reply) = 0;

    // get interface.
    virtual bool post(const std::string& types, const std::string& uri,
                      const std::string& condition, Json::Value* reply) = 0;

  protected:
    const std::string _index;

    explicit ElasticSearch(const std::string& index)
        : _index(index) {
      CHECK(!index.empty());
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(ElasticSearch);
};

// index: database.
// zk_path: path in zookeeper.
ElasticSearch* CreateElasticSearch(const std::string& index,
                                   const std::string& zk_path);

}

