#include "topic.h"

#include <librdkafka/rdkafkacpp.h>

namespace util {

KafkaTopic::~KafkaTopic() = default;
KafkaTopic::KafkaTopic(const std::string& topic, RdKafka::Handle* handler)
    : _topic_name(topic), _handler(handler) {
  CHECK(!topic.empty());
  CHECK_NOTNULL(handler);

  WLOG<< "create topic: " << topic;
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
  _topic.reset(RdKafka::Topic::create(_handler, topic, conf.get(), err));
  if (_topic == NULL) {
    ELOG<< "create kafka topic error: " << err;
    return false;
  }

  return true;
}

}
