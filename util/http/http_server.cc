#include "http_server.h"
#include "http_scheduler.h"

DEF_uint32(http_timedout, 300, "timedout for http, mill second");
DEF_bool(ssl, false, "whether enable ssl");

DEF_string(pem_file, "conf/inveno.key", "ssl perm file");
DEF_string(priv_key, "conf/inveno.key", "ssl priv file");
DEF_string(ca_file, "conf/inveno.crt", "ssl crt file");

DEF_uint32(http_thread, 8, "thread number for http server");
DEF_string(http_ip, "192.168.1.251", "ip for http server");
DEF_uint32(http_port, 80, "port for http server");

namespace {

int dummy_ssl_verify_callback(int ok, X509_STORE_CTX * x509_store) {
  return 1;
}

int dummy_check_issued_cb(X509_STORE_CTX * ctx, X509 * x, X509 * issuer) {
  return 1;
}

void http_default_cb(evhtp_request_t* req, void * arg) {
  http::HttpScheduler* scheduler = static_cast<http::HttpScheduler*>(arg);
  scheduler->dispatch(req);
}

void init_thread_cb(evhtp_t * htp, evthr_t * thr, void * arg) {
  http::HttpServer* hs = static_cast<http::HttpServer*>(arg);
  // todo:
}
}

namespace http {

HttpServer::HttpServer(HandlerMap* handler_map)
    : _ev_base(NULL), _ev_http(NULL) {
  CHECK_NOTNULL(handler_map);
  _handlers.reset(handler_map);
}

HttpServer::~HttpServer() {
  stop();
}

bool HttpServer::initSSL() {
  evhtp_ssl_cfg_t scfg = { 0 };
  scfg.pemfile = (char*) FLG_pem_file.data();
  scfg.privfile = (char*) FLG_priv_key.data();
  scfg.cafile = (char*) FLG_ca_file.data();
  scfg.capath = (char*) "./conf/";
  scfg.ciphers = (char*) "AES256+RSA:HIGH:+MEDIUM:+LOW";

  scfg.ssl_opts = SSL_OP_NO_SSLv2;
  scfg.ssl_ctx_timeout = 60 * 60 * 48;
  scfg.verify_peer = SSL_VERIFY_NONE;
  scfg.verify_depth = 42;
  scfg.x509_verify_cb = dummy_ssl_verify_callback;
  scfg.x509_chk_issued_cb = dummy_check_issued_cb;
  scfg.scache_type = evhtp_ssl_scache_type_internal;

  auto ret = evhtp_ssl_init(_ev_http, &scfg);
  if (ret != 0) {
    ELOG<< "evhtp_ssl_init error, ret: " << ret;
    return false;
  }

  return true;
}

bool HttpServer::start() {
  _ev_base = event_base_new();
  CHECK_NOTNULL(_ev_base);
  _ev_http = evhtp_new(_ev_base, NULL);
  CHECK_NOTNULL(_ev_http);

  struct timeval tv { 0, FLG_http_timedout * 1000 };
  WLOG<< "http timedout(mill second): " << FLG_http_timedout;
  evhtp_set_timeouts(_ev_http, &tv, &tv);
  WLOG<< "http enable ssl: " << FLG_ssl;
  if (FLG_ssl && !initSSL()) {
    ELOG<< "init ssl error: ";
    return false;
  }

  _scheduler.reset(new HttpScheduler(this));
  evhtp_set_gencb(_ev_http, http_default_cb, _scheduler.get());
  evhtp_use_threads(_ev_http, init_thread_cb, FLG_http_thread, this);
  WLOG<< "http bind ip: " << FLG_http_ip << ", port: " << FLG_http_port;
  auto ret = evhtp_bind_socket(_ev_http, FLG_http_ip.c_str(), FLG_http_port,
                               8192);
  if (ret != 0) {
    ELOG<< "evhtp_bind_socket error";
    return false;
  }

  WLOG<< "start http loop successfully ...";
  event_base_loop(_ev_base, 0);
  return true;
}

void HttpServer::stop() {
  if (_ev_http != NULL) {
    evhtp_unbind_socket(_ev_http);
    evhtp_free(_ev_http);
    _ev_http = NULL;
  }

  if (_ev_base != NULL) {
    ::event_base_free(_ev_base);
    _ev_base = NULL;
  }
}

}
