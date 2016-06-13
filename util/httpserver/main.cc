#include "httpserver_event.h"

DEF_uint32(http_port, 80, "http port");
DEF_uint32(http_timedout, 1000, "http time out");
DEF_bool(ssl_on, false, "true to turn on ssl");
DEF_bool(keepalived, true, "true to turn on keepalived");

int main(int argc, char ** argv) {
  ccflag::init_ccflag(argc, argv);
  cclog::init_cclog(*argv);


  //PollingConnectionManager<GateLogicServer,spServerType>::getInstance()->initialization();
  LOG << "GateLogicServer httpserver start.";
  struct event* ev_sigint;
  event_base* evbase = event_base_new();
  evhtp_t* htp = evhtp_new(evbase, NULL);

  struct timeval tv;
  tv.tv_usec = 0;
  tv.tv_sec = timeout;
  struct timeval tv1;
  tv1.tv_usec = 0;
  tv1.tv_sec = timeout;
  evhtp_set_timeouts(htp, &tv, &tv1);

  if (FLG_ssl_on) {
    char pemfile[] = "./conf/inveno.key";
    char privfile[] = "./conf/inveno.key";
    char cafile[] = "./conf/inveno.crt";
    char capath[] = "./";
    char ciphers[] = "AES256+RSA:HIGH:+MEDIUM:+LOW";

    evhtp_ssl_cfg_t scfg;
    scfg.pemfile = pemfile;
    scfg.privfile = privfile;
    scfg.cafile = cafile;
    scfg.capath = capath;
    scfg.ciphers = ciphers;
    scfg.ssl_opts = SSL_OP_NO_SSLv2;
    scfg.ssl_ctx_timeout = 60 * 60 * 48;
    scfg.verify_peer = SSL_VERIFY_NONE;
    scfg.verify_depth = 42;
    scfg.x509_verify_cb = dummy_ssl_verify_callback;
    scfg.x509_chk_issued_cb = dummy_check_issued_cb;
    scfg.scache_type = evhtp_ssl_scache_type_internal;
    scfg.scache_timeout = NULL;
    scfg.scache_size = NULL;
    scfg.scache_init = NULL;
    scfg.scache_add = NULL;
    scfg.scache_get = NULL;
    scfg.scache_del = NULL;
    scfg.named_curve = NULL;
    scfg.dhparams = NULL;

    evhtp_ssl_init(htp, &scfg);
  }

  evhtp_set_gencb(htp, inveno_default_cb, NULL);

  int result = 0;
  result = evhtp_use_threads(htp, init_thread_cb, threadNum,
                             (void*) (confPath.c_str()));
  if (0 != result) {
    printf("boot threads error!%d\n", result);
    return result;
  }
  result = evhtp_bind_socket(htp, http_ip.c_str(), http_port, backlog);
  if (0 != result) {
    printf("bind_socket error! ip or port set error:%d\n", result);
    return result;
  }

  ev_sigint = evsignal_new(evbase, SIGINT, sigint, evbase);
  evsignal_add(ev_sigint, NULL);

  event_base_loop(evbase, 0);

  event_free(ev_sigint);
  evhtp_unbind_socket(htp);

  evhtp_free(htp);
  event_base_free(evbase);

  return 0;
}
