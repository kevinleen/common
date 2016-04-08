#pragma once

#include "producer.h"

namespace RdKafka {
class Event;
class Message;
class Producer;
}

namespace util {

class KafkaEventClosure;
class KafkaDevelelyClosure;

class KafkaTopic;
class KafkaProducerImpl : public KafkaProducer {
  public:
    KafkaProducerImpl();
    virtual ~KafkaProducerImpl();

    bool init();

    void onEvent(RdKafka::Event &event);
    void onDelivery(RdKafka::Message &message);

  private:
    std::unique_ptr<KafkaEventClosure> _ev_cb;
    std::unique_ptr<KafkaDevelelyClosure> _delivery_cb;

    void poll();
    bool _running;
    std::unique_ptr<Thread> _poll_thread;
    std::unique_ptr<RdKafka::Producer> _producer;

    friend class KafkaTopic;
    void dumpConf(const std::list<std::string>& conf_list);
    virtual void produce(const char* topic_name, const char* data, uint32 len);

    RwLock _rw_lock;
    std::shared_ptr<KafkaTopic> findOrCreateTopic(const char* topic);
    std::unordered_map<const char*, std::shared_ptr<KafkaTopic>> _topics;

    DISALLOW_COPY_AND_ASSIGN(KafkaProducerImpl);
};

}
