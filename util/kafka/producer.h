#include "base/base.h"

namespace util {

class KafkaProducer {
  public:
    virtual ~KafkaProducer() = default;

    virtual void produce(const char* topic, const char* data, uint32 len) = 0;

  protected:
    KafkaProducer() = default;

  private:
    DISALLOW_COPY_AND_ASSIGN(KafkaProducer);
};

KafkaProducer* CreateKafkaProducer();

}
