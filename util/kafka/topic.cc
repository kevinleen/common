#include "topic.h"
#include "producer_impl.h"

#include <librdkafka/rdkafkacpp.h>

namespace util {

KafkaTopic::~KafkaTopic() = default;
KafkaTopic::KafkaTopic(const char* topic, KafkaProducerImpl* producer)
    : _topic_name(topic), _producer(producer) {
  CHECK_NOTNULL(topic);
  CHECK_NOTNULL(producer);
}

void KafkaTopic::dumpConfig(const std::list<std::string>& conf_list) const {
  WLOG<< "topic configure, name: " << _topic_name;
  for (auto it = conf_list.begin(); it != conf_list.end();) {
    std::string line(*it++);
    line += ": " + *it++;
    WLOG<< "\t" << line;
  }
}

bool KafkaTopic::init() {
  std::unique_ptr<RdKafka::Conf> conf;
  conf.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC));

  std::string err;
//  conf->set("partitioner_cb", NULL, err);

  dumpConfig(*conf->dump());
  std::string topic(_topic_name);
  _topic.reset(
      RdKafka::Topic::create(_producer->_producer.get(), topic, conf.get(),
                             err));
  if (_topic == NULL) {
    ELOG<< "create kafka topic error: " << err;
    return false;
  }

  return true;
}

void KafkaTopic::produce(const char* data, uint32 len) {
  auto producer = _producer->_producer.get();
  if (producer != NULL) {
    auto ret = producer->produce(_topic.get(), RdKafka::Topic::PARTITION_UA,
                                 RdKafka::Producer::RK_MSG_COPY,
                                 const_cast<char *>(data), len,
                                 NULL,
                                 NULL);
    if (ret != RdKafka::ERR_NO_ERROR) {
      ELOG<<"kafka produce error: " << RdKafka::err2str(ret);
    }
  }
}

}
