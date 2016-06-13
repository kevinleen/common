#include "search_handler.h"

#include "util/http/handler_map.h"

static void onFree(void* arg) {
  test::SearchHandler* h = static_cast<test::SearchHandler*>(arg);
  delete h;
}

void registeHandler(http::HandlerMap* handlers) {
  test::SearchHandler* h = new test::SearchHandler;
  http::Handler* handler = new http::Handler("/search");
  handler->arg = h;
  handler->cb = std::bind(&test::SearchHandler::onMessage, h, std::_1, std::_2);
  handler->free_cb = std::bind(onFree, std::_1);

  handlers->addHandler(handler);
}
