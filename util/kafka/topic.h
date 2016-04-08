#pragma once

#include "base/base.h"

namespace RdKafka {
class Topic;
class Producer;
}

namespace util {

class KafkaProducerImpl;
class KafkaTopic {
  public:
    KafkaTopic(const char* topic, KafkaProducerImpl* producer);
    ~KafkaTopic();

    bool init();

    void produce(const char* data, uint32 len);
    const char* topicName() const {
      return _topic_name;
    }

  private:
    const char* _topic_name;

    KafkaProducerImpl* _producer;
    std::unique_ptr<RdKafka::Topic> _topic;
    void dumpConfig(const std::list<std::string>& conf_list) const;

    DISALLOW_COPY_AND_ASSIGN(KafkaTopic);
};

}
