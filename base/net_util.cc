#include "net_util.h"
#include "base.h"

namespace net {
namespace xx {
struct addrinfo* GetAddrInfo(const char* name) {
    struct addrinfo hints;
    ::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* ret = NULL;
    int err = getaddrinfo(name, NULL, &hints, &ret);

    if (err != 0) {
        ELOG << "getaddrinfo error, domain: " << name << ", err: "
             << gai_strerror(err);
        return NULL;
    }

    return ret;
}
}

bool GetIpByName(const std::string& name, std::vector<uint32>& ips) {
    struct addrinfo* ret = xx::GetAddrInfo(name.c_str());
    if (ret == NULL) return false;

    for (auto it = ret; it != NULL; it = it->ai_next) {
        sockaddr_in* addr = cstyle_cast<sockaddr_in*>(it->ai_addr);
        ips.push_back(addr->sin_addr.s_addr);
    }

    freeaddrinfo(ret);
    return true;
}

bool GetIpStrByName(const std::string& name, std::vector<std::string>& ips) {
    struct addrinfo* ret = xx::GetAddrInfo(name.c_str());
    if (ret == NULL) return false;

    for (auto it = ret; it != NULL; it = it->ai_next) {
        sockaddr_in* addr = cstyle_cast<sockaddr_in*>(it->ai_addr);
        ips.push_back(IpToString(addr->sin_addr.s_addr));
    }

    freeaddrinfo(ret);
    return true;
}
} // namespace net
