#pragma once

#include "base/base.h"

#include "thrift_server.h"
#include "thrift_socket.h"
#include <thrift/config.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/protocol/TCompactProtocol.h>

namespace util {

namespace protocol = apache::thrift::protocol;
namespace transport = apache::thrift::transport;

template<typename Message>
bool parseFromArray(const char* data, uint32 len, Message* msg) {
  try {
    shared_ptr<transport::TMemoryBuffer> buffer;
    buffer.reset(new transport::TMemoryBuffer);
    buffer->write((const uint8_t*) data, len);

    shared_ptr<transport::TTransport> trans(buffer);
    protocol::TCompactProtocol protocol(trans);
    msg->read(&protocol);
  } catch (std::exception &) {
    return false;
  }

  return true;
}

template<typename Message>
bool parseFromString(const std::string& str, Message* msg) {
  CHECK_NOTNULL(msg);
  return parseFromArray(str.data(), str.size(), msg);
}

template<typename Message>
void serializeAsString(const Message& msg, std::string* value) {
  CHECK_NOTNULL(value);
  value->clear();

  shared_ptr<transport::TMemoryBuffer> buffer(new transport::TMemoryBuffer);
  shared_ptr<transport::TTransport> trans(buffer);
  protocol::TCompactProtocol protocol(trans);
  msg.write(&protocol);

  uint8_t* buf;
  uint32_t size;
  buffer->getBuffer(&buf, &size);
  *value = std::string((char*) buf, (unsigned int) size);
}

template<typename M>
const std::string serializeAsString(const M& msg) {
  std::string value;
  serializeAsString(msg, &value);
  return value;
}

}
