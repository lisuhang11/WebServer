#include "inet_address.h"
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <cassert>

static const in_addr_t kInaddrAny = INADDR_ANY;
static const in6_addr kIn6addrAny = IN6ADDR_ANY_INIT;

InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6) 
    : ipv6_(ipv6) {
    if (ipv6) {
        memset(&addr6_, 0, sizeof addr6_);
        addr6_.sin6_family = AF_INET6;
        in6_addr ip = loopbackOnly ? in6addr_loopback : kIn6addrAny;
        addr6_.sin6_addr = ip;
        addr6_.sin6_port = htons(port);
    } else {
        memset(&addr_, 0, sizeof addr_);
        addr_.sin_family = AF_INET;
        in_addr_t ip = loopbackOnly ? htonl(INADDR_LOOPBACK) : kInaddrAny;
        addr_.sin_addr.s_addr = ip;
        addr_.sin_port = htons(port);
    }
}

InetAddress::InetAddress(std::string ip, uint16_t port, bool ipv6) 
    : ipv6_(ipv6) {
    if (ipv6) {
        memset(&addr6_, 0, sizeof addr6_);
        addr6_.sin6_family = AF_INET6;
        addr6_.sin6_port = htons(port);
        if (inet_pton(AF_INET6, ip.c_str(), &addr6_.sin6_addr) <= 0) {
            std::cerr << "inet_pton error: " << ip << std::endl;
        }
    } else {
        memset(&addr_, 0, sizeof addr_);
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0) {
            std::cerr << "inet_pton error: " << ip << std::endl;
        }
    }
}

InetAddress::InetAddress(const struct sockaddr_in& addr) 
    : ipv6_(false) {
    memcpy(&addr_, &addr, sizeof addr_);
}

InetAddress::InetAddress(const struct sockaddr_in6& addr6) 
    : ipv6_(true) {
    memcpy(&addr6_, &addr6, sizeof addr6_);
}

std::string InetAddress::toIp() const {
    char buf[64] = {0};
    if (ipv6_) {
        inet_ntop(AF_INET6, &addr6_.sin6_addr, buf, sizeof buf);
    } else {
        inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    }
    return buf;
}

std::string InetAddress::toIpPort() const {
    char buf[64] = {0};
    if (ipv6_) {
        inet_ntop(AF_INET6, &addr6_.sin6_addr, buf, sizeof buf);
        size_t end = strlen(buf);
        uint16_t port = ntohs(addr6_.sin6_port);
        sprintf(buf + end, ":%u", port);
    } else {
        inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
        size_t end = strlen(buf);
        uint16_t port = ntohs(addr_.sin_port);
        sprintf(buf + end, ":%u", port);
    }
    return buf;
}

uint16_t InetAddress::port() const {
    return ipv6_ ? ntohs(addr6_.sin6_port) : ntohs(addr_.sin_port);
}

const struct sockaddr* InetAddress::getSockAddr() const {
    return ipv6_ ? reinterpret_cast<const struct sockaddr*>(&addr6_) 
                 : reinterpret_cast<const struct sockaddr*>(&addr_);
}

void InetAddress::setSockAddrInet(const struct sockaddr_in& addr) {
    ipv6_ = false;
    addr_ = addr;
}

void InetAddress::setSockAddrInet6(const struct sockaddr_in6& addr6) {
    ipv6_ = true;
    memcpy(&addr6_, &addr6, sizeof addr6_);
}

socklen_t InetAddress::getSockAddrLen() const {
    return ipv6_ ? sizeof addr6_ : sizeof addr_;
}

bool InetAddress::isIpv4MappedIpv6() const {
    if (!ipv6_) return false;
    // IPv4映射的IPv6地址格式为::ffff:IPv4地址
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&addr6_.sin6_addr);
    for (int i = 0; i < 10; ++i) {
        if (bytes[i] != 0) return false;
    }
    return bytes[10] == 0xff && bytes[11] == 0xff;
}

const struct sockaddr_in& InetAddress::getSockAddrInet() const {
    if (ipv6_ && isIpv4MappedIpv6()) {
        // 返回IPv4映射地址中的IPv4部分
        static struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = addr6_.sin6_port;
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&addr6_.sin6_addr);
        memcpy(&addr.sin_addr, bytes + 12, sizeof addr.sin_addr);
        return addr;
    }
    assert(!ipv6_);
    return addr_;
}

const struct sockaddr_in6& InetAddress::getSockAddrInet6() const {
    assert(ipv6_);
    return addr6_;
}

InetAddress InetAddress::getAnyIpv4() {
    return InetAddress(0, false, false);
}

InetAddress InetAddress::getAnyIpv6() {
    return InetAddress(0, false, true);
}