#include "topic.h"
#include "closure.h"
#include "producer_impl.h"

#include <librdkafka/rdkafkacpp.h>

DEF_string(kafka_brokers, "", "server list for kafka brokers");

namespace util {

KafkaProducer* CreateKafkaProducer() {
  std::unique_ptr<KafkaProducerImpl> p(new KafkaProducerImpl);
  if (!p->init()) {
    ELOG<< "initialized kafka error";
    return NULL;
  }
  return p.release();
}

KafkaProducerImpl::KafkaProducerImpl()
    : _running(false) {
}

KafkaProducerImpl::~KafkaProducerImpl() {
  _running = false;
  if (_poll_thread != NULL) {
    _poll_thread->join();
    _poll_thread.reset();
  }
}

void KafkaProducerImpl::dumpConf(const std::list<std::string>& conf_list) {
  WLOG<< "kafka producer configure: ";
  for (auto it = conf_list.begin(); it != conf_list.end();) {
    std::string line(*it++);
    line += ": " + *it++;
    WLOG<< "\t" << line;
  }
}

bool KafkaProducerImpl::init() {
  std::unique_ptr<RdKafka::Conf> conf;
  conf.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));

  std::string err;
  CHECK(!FLG_kafka_brokers.empty()) << "kafka_brokers is empty";
  conf->set("metadata.broker.list", FLG_kafka_brokers, err);
  conf->set("event_cb", _ev_cb.get(), err);
//  conf->set("dr_cb", _delivery_cb.get(), err);

  dumpConf(*conf->dump());
  _producer.reset(RdKafka::Producer::create(conf.get(), err));
  if (_producer == NULL) {
    ELOG<< "create kafka producer error, msg: " << err;
    return false;
  }

  _poll_thread.reset(new Thread(std::bind(&KafkaProducerImpl::poll, this)));
  _poll_thread->start();

  WLOG<< "create kafka producer successfully, name: " << _producer->name();
  return true;
}

void KafkaProducerImpl::onEvent(RdKafka::Event &event) {
  LOG<< "kafka producer onEvent: " << event.str();
}

void KafkaProducerImpl::onDelivery(RdKafka::Message &message) {
  // FIXME: not used.
  LOG<< "kafka producer onDelivery: " << message.errstr();
}

void KafkaProducerImpl::poll() {
  _running = true;

  while (_running) {
    if (_producer != NULL) {
      _producer->poll(1 * 1000 /*milli seconds*/);
    }
  }
}

void KafkaProducerImpl::produce(const char* topic_name, const char* data,
                                uint32 len) {
  auto topic = findOrCreateTopic(topic_name);
  if (_producer != NULL) {
    LOG<< "topic: " << topic->topicName() << " data: " << data;
    auto ret = _producer->produce(topic->getTopic(), RdKafka::Topic::PARTITION_UA,
        RdKafka::Producer::RK_MSG_COPY,
        const_cast<char *>(data), len,
        NULL,
        NULL);
    if (ret != RdKafka::ERR_NO_ERROR) {
      ELOG<<"kafka produce error: " << RdKafka::err2str(ret);
    }
  }
}

std::shared_ptr<KafkaTopic> KafkaProducerImpl::findOrCreateTopic(
    const char* topic_name) {
  {
    ReadLockGuard l(_rw_lock);
    auto it = _topics.find(topic_name);
    if (it != _topics.end()) return it->second;
  }

  std::shared_ptr<KafkaTopic> topic;
  topic.reset(
      new KafkaTopic(std::string((char*) topic_name, strlen(topic_name)),
                     _producer.get()));
  if (!topic->init()) {
    ELOG<< "topic initialized error, name: " << topic_name;
    return std::shared_ptr<KafkaTopic>();
  }

  WriteLockGuard l(_rw_lock);
  auto it = _topics.find(topic_name);
  if (it != _topics.end()) return it->second;

  _topics.insert(std::make_pair(topic_name, topic));

  return topic;
}

}
