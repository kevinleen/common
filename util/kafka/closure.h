#pragma once

#include "base/base.h"
#include <librdkafka/rdkafkacpp.h>

namespace util {

class KafkaProducerImpl;
class KafkaDevelelyClosure : public RdKafka::DeliveryReportCb {
  public:
    explicit KafkaDevelelyClosure(KafkaProducerImpl* producer);
    virtual ~KafkaDevelelyClosure();

  private:
    KafkaProducerImpl* _producer;

    void dr_cb(RdKafka::Message &message);

    DISALLOW_COPY_AND_ASSIGN(KafkaDevelelyClosure);
};

class KafkaEventClosure : public RdKafka::EventCb {
  public:
    explicit KafkaEventClosure(KafkaProducerImpl* producer);
    virtual ~KafkaEventClosure();

  private:
    KafkaProducerImpl* _producer;

    void event_cb(RdKafka::Event &event);

    DISALLOW_COPY_AND_ASSIGN(KafkaEventClosure);
};

}

