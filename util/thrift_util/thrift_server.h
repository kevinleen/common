#pragma once

#include <thrift/protocol/TCompactProtocol.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/server/TNonblockingServer.h>

#include "base/base.h"

namespace thr {
using ::apache::thrift::concurrency::Runnable;
using ::apache::thrift::concurrency::ThreadManager;
using ::apache::thrift::concurrency::PosixThreadFactory;
using ::apache::thrift::server::TNonblockingServer;

using ::apache::thrift::protocol::TProtocolFactory;
using ::apache::thrift::protocol::TBinaryProtocolFactory;

using ::apache::thrift::concurrency::Thread;
}

namespace util {

class ThriftServer {
  public:
    virtual ~ThriftServer() {
    }

//   thread logic,
//      one and only one per thread.
    class ThreadLogic {
      public:
        virtual ~ThreadLogic() {
        }

        virtual std::shared_ptr<ThreadLogic> clone() = 0;
    };
    std::shared_ptr<ThreadLogic> getLogic() const;

  protected:
    ThriftServer() {
      _port = _io_num = _worker_num = 0;
    }

    // logic will wrappered by shared_ptr, and hold by ThritServer.
    void init(ThreadLogic* logic);

    bool startLoop();
    void stopLoop();

    shared_ptr<thr::ThreadManager> _thread_mgr;

    uint16 _port;
    uint32 _io_num;
    uint32 _worker_num;
    std::unique_ptr<thr::TNonblockingServer> _server;

  private:
    void doEventLoop();
    std::unique_ptr<Thread> _loop_thread;

    class LogicThreadSafeMap;
    std::shared_ptr<LogicThreadSafeMap> _logic_map;

    DISALLOW_COPY_AND_ASSIGN(ThriftServer);
};

#define THRIFT_LOGIC(__s, __t) \
  (static_cast<__t*>((__s)->getLogic().get()))

}
