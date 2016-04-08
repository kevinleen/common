#pragma once

#include "base/base.h"
#include "include/thread_safe.h"

#include <thrift/transport/TTransport.h>
#include <thrift/transport/TVirtualTransport.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/PlatformSocket.h>

#include <thrift/protocol/TProtocol.h>
#include <thrift/protocol/TBinaryProtocol.h>

namespace util {

namespace transport = apache::thrift::transport;

// not threadsafe.
// it's caller's responsility that sure only called from one thread.
class ThriftSocket : public transport::TVirtualTransport<ThriftSocket> {
  public:
    // timedout: million second.
    ThriftSocket(const std::string& ip, uint16 port, uint32 timedout);
    virtual ~ThriftSocket() {
      close();
    }

    int fileHandle() const {
      return _socket;
    }

    const std::string& ip() const {
      return _ip;
    }
    uint16 port() const {
      return _port;
    }

    virtual void close();
    virtual bool peek();
    virtual void open();
    virtual bool isOpen();

    virtual uint32_t read(u_char* buf, uint32 len);
    virtual void write(const u_char* buf, uint32 len);

  private:
    const std::string _ip;
    const uint16 _port;
    const uint32 _timedout;

    int _socket;

    void openSocket(uint32 timedout);
    bool timedWait(int32 timedout, uint8 event);

    DISALLOW_COPY_AND_ASSIGN(ThriftSocket);
};

template<typename T>
T* NewThriftClient(const std::string& ip, uint32 port, uint32 ms) {
  shared_ptr<ThriftSocket> socket(new ThriftSocket(ip, port, ms));

  // try for 3 times
  for (int i = 0; i < 3; ++i) {
    try {
      socket->open();
      break;
    } catch (std::exception& e) {
      ELOG<< "CreateThriftClient, ip: " << ip << ", port: " << port
      << ", timeout: " << ms << ", exception: " << e.what();
    }
  }

  shared_ptr<apache::thrift::transport::TTransport> transport(
      new apache::thrift::transport::TFramedTransport(socket));
  shared_ptr<apache::thrift::protocol::TProtocol> protocol(
      new apache::thrift::protocol::TBinaryProtocol(transport));
  return new T(protocol);
}

template<typename T>
class ThriftClientDelegate : public ThreadSafeHandlerWrapper<T>::Delegate {
  public:
    ThriftClientDelegate(const std::string& ip, uint32 port, uint32 ms)
        : _ip(ip), _port(port), _ms(ms) {
      CHECK(!_ip.empty()) << "ThriftClient, ip is empty!";
      CHECK(_port != 0) << "ThriftClient, port not set!";
    }
    virtual ~ThriftClientDelegate() = default;

    virtual std::shared_ptr<T> create();

  private:
    std::string _ip;
    uint32 _port;
    uint32 _ms;

    DISALLOW_COPY_AND_ASSIGN(ThriftClientDelegate);
};

template<typename T>
std::shared_ptr<T> ThriftClientDelegate<T>::create() {
  LOG<<"create ThriftClient, ip: " << _ip << ", port: " << _port
  << ", timedout: " << _ms;

  std::shared_ptr<T> client(NewThriftClient<T>(_ip, _port, _ms));
  CHECK_NOTNULL(client);
  return client;
}

template<typename T>
class ThriftClient : public ThreadSafeHandlerWrapper<T> {
  public:
    ThriftClient() = default;

    ThriftClient(const std::string& ip, uint32 port, uint32 ms)
        : ThreadSafeHandlerWrapper<T>(
            new util::ThriftClientDelegate<T>(ip, port, ms)) {
    }

    virtual ~ThriftClient() = default;
};

template<typename T>
inline std::shared_ptr<ThriftClient<T>>
CreateThriftClient(const std::string& ip, uint32 port, uint32 ms) {
    return std::shared_ptr<ThriftClient<T>>(new ThriftClient<T>(ip, port, ms));
}

}
