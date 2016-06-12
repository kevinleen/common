#pragma once

#include "base/base.h"

namespace RdKafka {
class Topic;
class Producer;

class Handle;
}

namespace util {

class KafkaTopic {
  public:
    KafkaTopic(const std::string& topic, RdKafka::Handle* handler);
    ~KafkaTopic();

    bool init();

    RdKafka::Topic* getTopic() const {
      return _topic.get();
    }

    const std::string& topicName() const {
      return _topic_name;
    }

  private:
    RdKafka::Handle* _handler;
    const std::string _topic_name;

    std::unique_ptr<RdKafka::Topic> _topic;
    void dumpConfig(const std::list<std::string>& conf_list) const;

    DISALLOW_COPY_AND_ASSIGN(KafkaTopic);
};

}
