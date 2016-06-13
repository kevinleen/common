#include "http_server.h"

DEF_bool(daemon, false, "run as daemon model");

void registeHandler(http::HandlerMap* handlers) __attribute__ ((weak));

int main(int argc, char* argv[]) {
  ccflag::init_ccflag(argc, argv);
  cclog::init_cclog(*argv);

  if (FLG_daemon) {
    WLOG<< "daemon model...";
    ::daemon(1, 1);
  }

  std::unique_ptr<http::HandlerMap> handlers(new http::HandlerMap);
  if (registeHandler) {
    registeHandler(handlers.get());
  }

  http::HttpServer server(handlers.release());
  if (!server.start()) {
    ELOG<< "start http server error";
    return -1;
  }

  return 0;
}
