#pragma once

#include "data_types.h"
#include "cclog/cclog.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <endian.h>

namespace net {
inline uint16 hton16(uint16 v) {
    return htobe16(v);
}

inline uint32 hton32(uint32 v) {
    return htobe32(v);
}

inline uint64 hton64(uint64 v) {
    return htobe64(v);
}

inline uint16 ntoh16(uint16 v) {
    return be16toh(v);
}

inline uint32 ntoh32(uint32 v) {
    return be32toh(v);
}

inline uint64 ntoh64(uint64 v) {
    return be64toh(v);
}

/*
 * ip: network byte order
 */
inline std::string IpToString(uint32 ip) {
    struct in_addr addr;
    addr.s_addr = ip;

    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr, buf, sizeof(buf));
    return std::string(buf);
}

inline uint32 StringToIp(const std::string& ip) {
    struct in_addr addr;
    CHECK_EQ(inet_pton(AF_INET, ip.c_str(), &addr), 1);
    return addr.s_addr;
}

/*
 * get all ips by the name, return false on any error
 */
bool GetIpByName(const std::string& name, std::vector<uint32>& ips);

/*
 * get all ips as string by the name, return false on any error
 */
bool GetIpStrByName(const std::string& name, std::vector<std::string>& ips);
} // namespace net
