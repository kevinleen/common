#include "thrift_socket.h"
#include "rpc/socket_util.h"
#include "rpc/socket_waiter.h"
#include "rpc/event_manager.h"

#include <thrift/transport/TTransportException.h>
using apache::thrift::transport::TTransportException;

namespace {

bool createStreamSocket(int family, int* sock) {
  int fd = ::socket(AF_INET, SOCK_STREAM, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (fd == -1) {
    WLOG<< "socket error";
    return false;
  }

  *sock = fd;
  return true;
}
}

namespace util {

ThriftSocket::ThriftSocket(const std::string& ip, uint16 port, uint32 timedout)
    : _ip(ip), _port(port), _timedout(timedout) {
  CHECK(!ip.empty());
  CHECK_NE(port, 0);
  CHECK_GT(timedout, 0);
  _socket = kInvalidFd;
}

bool ThriftSocket::peek() {
  if (!isOpen()) {
    ELOG<< "peek error, no valid socket: ";
    return false;
  }

  char buf;
  int32 ret = ::recv(_socket, &buf, sizeof(char), MSG_PEEK);
  if (ret == 1) return true;
  else if (ret == 0) {
    LOG<< "socket recived: EOF, fd: " << _socket;
    close();
    return false;
  }

  if (errno != EINTR && errno != EAGAIN) {
    close();
  }

  return false;
}

void ThriftSocket::openSocket(uint32 timedout) {
  int fd;
  if (!rpc::createSocket(&fd)) {
    auto error = std::string("socket error: ") + ::strerror(errno);
    throw TTransportException(TTransportException::TIMED_OUT, error);
  }

  if (!rpc::connectWithTimedout(fd, _ip, _port, timedout)) {
    closeWrapper(fd);
    throw TTransportException(TTransportException::INTERNAL_ERROR,
                              "can't connected: " + _ip);
  }

  _socket = fd;
}

bool ThriftSocket::isOpen() {
  return _socket != kInvalidFd;
}

void ThriftSocket::close() {
  if (_socket != kInvalidFd) {
    LOG<< "close socket: " << _socket;
    closeWrapper(_socket);
  }
}

void ThriftSocket::open() {
  CHECK_EQ(_socket, kInvalidFd);
  openSocket(_timedout);
}

bool ThriftSocket::timedWait(int32 timedout, uint8 event) {
  if (timedout <= 0 || !isOpen()) return false;
  if (timedout > _timedout) timedout = _timedout;
  rpc::SocketWaiter waiter(_socket);
  return waiter.timedWait(event, timedout);
}

uint32 ThriftSocket::read(u_char* buf, uint32 len) {
  sys::timer timer;
  if (!isOpen()) open();

  CHECK_NE(_socket, kInvalidFd);
  uint32 left = len;
  while (timer.ms() < _timedout && left > 0) {
    int readn = ::read(_socket, buf, left);
    if (readn == 0) {
      close();
      return 0;
    } else if (readn == -1) {
      switch (errno) {
        case EINTR:
          continue;
        case EAGAIN:
          if (_timedout < timer.ms()
              || !timedWait(_timedout - timer.ms(), EV_READ)) {
            close();
            throw TTransportException(TTransportException::TIMED_OUT,
                                      "recv: no data");
          }
          continue;
        default:
          close();
          throw TTransportException(TTransportException::INTERNAL_ERROR,
                                    ::strerror(errno));
      }
    }

    CHECK_GT(readn, 0);
    buf += readn;
    left -= readn;
  }

  return len - left;
}

void ThriftSocket::write(const u_char* buf, uint32 len) {
  sys::timer timer;
  if (!isOpen()) open();

  CHECK_NE(_socket, kInvalidFd);
  uint32 left = len;
  while (timer.ms() < _timedout && left > 0) {
    int readn = ::write(_socket, buf, len);
    if (readn == -1) {
      switch (errno) {
        case EINTR:
          continue;
        case EAGAIN:
          if (_timedout < timer.ms()
              || !timedWait(_timedout - timer.ms(), EV_WRITE)) {
            close();
            throw TTransportException(TTransportException::TIMED_OUT,
                                      "write error");
          }
          continue;
        default:
          close();
          throw TTransportException(TTransportException::INTERNAL_ERROR,
                                    ::strerror(errno));
      }
    }

    buf += readn;
    left -= readn;
  }

  if (left != 0) {
    throw TTransportException(TTransportException::TIMED_OUT,
                              "write error: not ready");
  }
}

}
