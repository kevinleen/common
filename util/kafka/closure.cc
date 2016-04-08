#include "closure.h"
#include "producer_impl.h"

#include <librdkafka/rdkafkacpp.h>

namespace util {

KafkaDevelelyClosure::~KafkaDevelelyClosure() = default;
KafkaDevelelyClosure::KafkaDevelelyClosure(KafkaProducerImpl* producer)
    : _producer(producer) {
  CHECK_NOTNULL(producer);
}

void KafkaDevelelyClosure::dr_cb(RdKafka::Message &message) {
  _producer->onDelivery(message);
}

KafkaEventClosure::~KafkaEventClosure() = default;
KafkaEventClosure::KafkaEventClosure(KafkaProducerImpl* producer)
    : _producer(producer) {
  CHECK_NOTNULL(producer);
}

void KafkaEventClosure::event_cb(RdKafka::Event &event) {
  _producer->onEvent(event);
}

}
