#pragma once

#include "es_node.h"
#include "include/thread_safe.h"

namespace util {

typedef ThreadSafeHandlerWrapper<ESNode> ThreadsafeES;

class ServerFinder;
class ElasticSearchImpl : public ElasticSearch {
  public:
    ElasticSearchImpl(const std::string& index, const std::string& zk_path);
    virtual ~ElasticSearchImpl();

    bool init();

  private:
    const std::string _zk_path;
    std::shared_ptr<ServerFinder> _finder;

    class ESFactory : public ThreadsafeES::Delegate {
      public:
        ESFactory(const std::string& index,
                  std::shared_ptr<ServerFinder> finder, ElasticSearchImpl* impl)
            : _finder(finder), _index(index) {
          CHECK_NOTNULL(impl);
          _impl = impl;
        }
        virtual ~ESFactory() = default;

      private:
        const std::string _index;

        ElasticSearchImpl* _impl;
        std::shared_ptr<ServerFinder> _finder;

        virtual std::shared_ptr<ESNode> create();

        DISALLOW_COPY_AND_ASSIGN(ESFactory);
    };

    class ESThreadSafeNode : public ThreadsafeES {
      public:
        ESThreadSafeNode(const std::string& index,
                         std::shared_ptr<ServerFinder> finder,
                         ElasticSearchImpl* impl)
            : ThreadsafeES(new ESFactory(index, finder, impl)) {
        }
        virtual ~ESThreadSafeNode() = default;

      private:
        DISALLOW_COPY_AND_ASSIGN(ESThreadSafeNode);
    };

    std::unique_ptr<ESThreadSafeNode> _nodes;
    void onRemove(pthread_t tid, const std::string& ip, uint32 port);
    std::shared_ptr<ESNode> getEntry() {
      return _nodes->current();
    }

    virtual bool post(const std::string& types, const std::string& uri,
                      const std::string& condition, Json::Value* reply);
    virtual bool post(const std::string& types, const Json::Value& request,
                      Json::Value* reply);

    DISALLOW_COPY_AND_ASSIGN(ElasticSearchImpl);
};

}
