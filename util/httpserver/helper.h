#pragma once

#include "base/base.h"

DEF_string(
    default_html,
    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\"><html xmlns=\"http://www.w3.org/1999/xhtml\"><head> <script type=\"text/javascript\">               document.domain='inveno.cn';</script><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" /><meta name=\"robots\" content=\"all\" /><meta name=\"author\" content=\"zhanggaoqin\" /><meta name=\"Copyright\" content=\"inveno\" /><meta name=\"Description\" content=\"PIWindow\" /><meta name=\"Keywords\" content=\"兀橱窗,PIWindow,派橱窗,inveno,英威诺\" /><title>PIWindow</title><link type=\"text/css\" rel=\"stylesheet\" href=\"http://www.inveno.cn/css/inveno.css\" media=\"all\" /></head><body><div class=\"header\">  <h1><a href=\"http://www.inveno.cn/index.html\" title=\"PIWindow\"><img src=\"http://www.inveno.cn/images/logo.jpg\" width=\"40\" height=\"40\" alt=\"PIWindow\" /></a></h1>  <ul>    <li><a href=\"http://www.inveno.cn/join.html\">加入我们</a></li>    <li><a href=\"http://www.inveno.cn/business.html\">商务合作</a></li>    <li><a href=\"http://www.inveno.cn/company.html\" class=\"on\">关于公司</a></li>    <li><a href=\"http://www.inveno.cn/index.html\">产品</a></li>  </ul></div><div class=\"company\">  <div class=\"content\">    <h2 class=\"fx42 black\">移动互联网运营与服务提供商</h2>    <h3 class=\"fx34 gray\">英威诺（<span class=\"en\">INVENO</span>）创立于2010年，专注于移动互联网服务运营和数据挖掘，整合用户、运营商、终端商、内容与服务商之需求，提供独特的移动聚合产品及服务。</h3>  </div></div><div class=\"map\">  <div class=\"content\">    <div class=\"address\">      <h3>深圳市英威诺科技有限公司</h3>      <p>地址：深圳市南山区高新南四道18号创维半导体设计大厦西座六楼605-610<br />电话：0755-26717733<br />邮箱：cs@anchormobile.com</p>    </div>  </div>  </div><div class=\"footer\">&copy;&nbsp;&nbsp;2013&nbsp;&nbsp;INVENO</div></body></html>",
    "default page");

namespace inv {

typedef std::map<std::string, std::string> KeyValue;
bool getJsonStr(std::string& result, KeyValue& query, const std::string& body,
                const std::string& path, const std::string& cl_ip,
                KeyValue& head) {

  if ((query.size() + body.size() < 1) || query.size() > 30
      || path.size() < 1) {
    ELOG<<"query or path size ERROR: query size:["
    << query.size() << "] path size: [" << path.size() << "] bodysize:["
    << body.size() << "]" << "|path" << path;
    result = FLG_default_html;
    return false;
  }

  bool rslt = false;

  std::string uid = "";
  std::string fun = "";
  std::map<std::string, std::string>::const_iterator it = _funcs.find(path);
  if (it != _funcs.end()) {
    fun = (*it).second;
    try {
      query["Content-Disposition"] = head["Content-Disposition"];
      query["User-Agent"] = head["User-Agent"];

      rslt = _funcMap.GetFunc((*it).second)(result, uid, query,
          encrypted ? strDecryptBody : body,
          path, cl_ip);
    } catch (TTransportException &e) {
      ELOG <<
      ":TTransport:" << it->second << ":" << uid << ":" << cl_ip << ":alg:"
      << GET_ALG_CONNET()->GetNoPollingConect(*this).first->isOpen()
      << ":Ad:"
      << GET_AD_CONNET()->GetNoPollingConect(*this).first->isOpen()
      << ":" << e.getType() << ":" << e.what());

      if (e.getType() != transport::TTransportException::TIMED_OUT) {
        reconnect();
      }
    } catch (TProtocolException &e) {
      ELOG <<
      ":TProtocol:" << it->second << ":" << uid << ":" << cl_ip << ":alg:"
      << GET_ALG_CONNET()->GetNoPollingConect(*this).first->isOpen()
      << ":Ad:"
      << GET_AD_CONNET()->GetNoPollingConect(*this).first->isOpen()
      << ":" << e.getType() << ":" << e.what());
      reconnect();
    } catch (TApplicationException &e) {
      ELOG <<
      ":TApplication:" << it->second << ":" << uid << ":" << cl_ip
      << ":alg:"
      << GET_ALG_CONNET()->GetNoPollingConect(*this).first->isOpen()
      << ":Ad:"
      << GET_AD_CONNET()->GetNoPollingConect(*this).first->isOpen()
      << ":" << e.getType() << ":" << e.what());
      reconnect();
    } catch (std::exception &e) {
      ELOG <<
      ":std:" << it->second << ":" << uid << ":" << cl_ip << ":"
      << body.size() << ":" << query.size() << ":alg:"
      << GET_ALG_CONNET()->GetNoPollingConect(*this).first->isOpen()
      << ":Ad:"
      << GET_AD_CONNET()->GetNoPollingConect(*this).first->isOpen()
      << ":" << e.what());
    }

  } else {
    //备案
    ELOG <<"query for : " << path);
    result = FLG_default_html;
    rslt = false;
  }

  return rslt;
}

}
