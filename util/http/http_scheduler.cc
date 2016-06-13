#include "http_server.h"
#include "http_scheduler.h"

#include "http_request.h"
#include "http_reply.h"

DEF_string(serv_name, "inveno", "the name of http server");

namespace http {

http::Handler* HttpScheduler::findHandler(const std::string& uri_path) {
  auto handler = _server->getHandlerMap()->findHandlerByUri(uri_path);
  if (handler == NULL) {
    ELOG<< "not find uri: " << uri_path;
    // todo: handle error.
    return NULL;
  }

  return handler;
}

void HttpScheduler::handleError(evhtp_request_t* req, uint32 status) const {
  evhtp_send_reply(req, status);
}

void HttpScheduler::addEntry(evhtp_request_t* req, const std::string& key,
                             const std::string& v) const {
//  WLOG<< "add: " << key << " " << v;
  auto kv = evhtp_header_new(key.data(), v.data(), 1, 1);
  evhtp_headers_add_header(req->headers_out, kv);
}

const std::string HttpScheduler::GMTime() const {
  return sys::local_time.to_string("%a, %d %b %Y %H:%M:%S GMT");
}

void HttpScheduler::initHeader(evhtp_request_t* req) const {
  addEntry(req, "Date", GMTime());
  addEntry(req, "Content-Type", "text/html; charset=UTF-8");
  addEntry(req, "Server", FLG_serv_name);
  addEntry(req, "Cache-Control", "no-cache");
  addEntry(req, "Connection", "keep-alive");
}

#include <zlib.h>
#define CHUNK (1000UL)
bool HttpScheduler::compress(const std::string& src, std::string* dst) const {
  // todo: need refactor.
  z_stream strm;
  strm.next_in = (Bytef*) src.data();
  strm.avail_in = src.size();
  uint32 buflen = std::max(src.size(), CHUNK);

  dst->resize(src.size());
  strm.avail_out = dst->size();
  strm.next_out = (Bytef*) &((*dst)[0]);
  auto ret = deflate(&strm, Z_FINISH);
  if (ret == Z_STREAM_ERROR) {
    deflateEnd(&strm);
    return false;
  }

  deflateEnd(&strm);
  return true;
}

void HttpScheduler::dispatch(evhtp_request_t* req) {
  std::unique_ptr<http::HttpRequest> request(new http::HttpRequest(req));
  if (!request->parse()) {
    ELOG<< "parse http protocol error";
    handleError(req);
    return;
  }

//  WLOG<< "full: " << req->uri->path->full;
  const auto& uri_path = req->uri->path->full;
  auto handler = findHandler(uri_path);
  if (handler == NULL) {
    handleError(req, EVHTP_RES_NOTFOUND);
    return;
  }

  std::unique_ptr<http::HttpReply> reply(new http::HttpReply);
  handler->cb(*request, reply.get());

  initHeader(req);
  const std::string* body = reply->httpBody().body();
  std::string buffer;
//  WLOG<< "coding: " << request->httpHeader().acceptEnCoding();
  if (request->httpHeader().acceptEnCoding() == "deflate") {
    if (compress(*body, &buffer)) {
      body = &buffer;
    } else {
      ELOG<< "compress error";
    }
  }

  evbuffer_add(req->buffer_out, body->data(), body->length());
  evhtp_send_reply(req, EVHTP_RES_OK);
}

}
