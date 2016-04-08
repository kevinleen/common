#include "thrift_server.h"
#include "include/thread_safe.h"

DEF_uint32(port, 8889, "the port of proxy server");
DEF_uint32(thrift_io, 2, "number  of thrift io thread");
DEF_uint32(thrift_worker, 4, "number of thrift worker thread");

namespace util {

class ThriftServer::LogicThreadSafeMap : public ThreadSafeHandler<ThreadLogic> {
  public:
    explicit LogicThreadSafeMap(ThreadLogic* logic)
        : _logic(logic) {
      CHECK_NOTNULL(logic);
      set(_logic);
    }
    ~LogicThreadSafeMap() {
      WriteLockGuard l(_rw_lock);
      clear();
    }

    std::shared_ptr<ThreadLogic> getLogic() {
      std::shared_ptr<ThreadLogic> logic = get();
      if (logic != NULL) return logic;
      logic = _logic->clone();
      set(logic);
      return logic;
    }

  private:
    std::shared_ptr<ThreadLogic> _logic;

    DISALLOW_COPY_AND_ASSIGN(LogicThreadSafeMap);
};

void ThriftServer::init(ThreadLogic* l) {
  _port = FLG_port;
  _io_num = FLG_thrift_io;
  _worker_num = FLG_thrift_worker;
  WLOG<< "thrift listen on port: " << _port;
  WLOG<< "thrift io thread number: " << _io_num;
  WLOG<<"thrift task thread number: " << _worker_num;

  _thread_mgr = thr::ThreadManager::newSimpleThreadManager(_worker_num);
  _logic_map.reset(new LogicThreadSafeMap(l));

  shared_ptr<thr::PosixThreadFactory> threads(new thr::PosixThreadFactory);
  _thread_mgr->threadFactory(threads);
}

void ThriftServer::doEventLoop() {
  CHECK_NOTNULL(_server);
  _server->serve();
}

bool ThriftServer::startLoop() {
  CHECK_NOTNULL(_thread_mgr);
  _thread_mgr->start();

  CHECK_NOTNULL(_server);
  CHECK_GT(_io_num, 0);
  _server->setNumIOThreads(_io_num);

  _loop_thread.reset(new Thread(std::bind(&ThriftServer::doEventLoop, this)));
  if (!_loop_thread->start()) {
    ELOG<< "start loop thread error";
    return false;
  }

  WLOG<< "start thrift successfully";
  return true;
}

void ThriftServer::stopLoop() {
  WLOG<< "will stop thrift server...";
  if (_server != NULL) {
    _server->stop();
    _server.reset();
  }

  if (_thread_mgr != NULL) {
    _thread_mgr->stop();
    _thread_mgr.reset();
  }

  if (_loop_thread != NULL) {
    _loop_thread->join();
    _loop_thread.reset();
  }
}

std::shared_ptr<ThriftServer::ThreadLogic> ThriftServer::getLogic() const {
  return _logic_map->getLogic();
}

}
