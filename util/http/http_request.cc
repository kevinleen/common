#include "http_request.h"

namespace http {

const std::string HttpRequest::clientIp() {
  auto sin =
      (struct sockaddr_in *) (evhtp_request_get_connection(_request)->saddr);
  auto client_ip = net::IpToString(sin->sin_addr.s_addr);
  const char *http_x_forwarded_for = evhtp_kv_find(_request->headers_in,
                                                   "X-Forwarded-For");
  if (http_x_forwarded_for != NULL) {
    client_ip.assign(http_x_forwarded_for);
  }

  return client_ip;
}

void HttpRequest::getQuery(evhtp_query_t *query, KVMap* kv_map) {
  if (query == NULL) return;

  auto kv = query->tqh_first;
  while (query != NULL && kv != NULL) {
    if (kv != NULL) {
      std::string key(kv->key, kv->klen);
      std::string val(kv->val, kv->vlen);
//      LOG<< "parse: " << key << " " << val;
      _query_map[key] = val;
    }

    kv = kv->next.tqe_next;
  }
}

bool HttpRequest::parseHeader() {
  auto header = getHttpHeader();
  auto http_header = header->getHeader();

  const std::string client_ip(clientIp());
  if (!client_ip.empty()) {
    (*http_header)["client_ip"] = client_ip;
  }

//  WLOG << "raw: " << (char*)(_request->uri->query_raw);
  getQuery(_request->uri->query, &_query_map);
  getQuery(_request->headers_in, http_header);

  return true;
}

bool HttpRequest::parseBody() {
  auto body = getHttpBody();
  uint32 len = evbuffer_get_length(_request->buffer_in);
  if (len > 0) {
    body->setBody(evbuffer_pullup(_request->buffer_in, len), len);
  }

  return true;
}

}
