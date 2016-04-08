#include "util/kafka/producer.h"

static void kafkaLog(const char* topic, const char* data, uint32 len) {
  static std::shared_ptr<util::KafkaProducer> producer;
  if (producer == NULL) {
    producer.reset(util::CreateKafkaProducer());
  }

  producer->produce(topic, data, len);
}

int main(int argc, char** argv) {
  ccflag::init_ccflag(argc, argv);
  cclog::init_cclog(*argv);

  cclog::klog.set_log_callback(std::bind(kafkaLog, std::_1, std::_2, std::_3));

//    KLOG("hello") << "hello";
//    KLOG("world") << "world";
  while (1) {
    KLOG("test1") << "hello";
    sys::sleep(1);
  }

  return 0;
}
