#ifndef INET_ADDRESS_H
#define INET_ADDRESS_H

#include <string>
#include <netinet/in.h>

class InetAddress {
public:
    // 构造函数
    explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);
    InetAddress(std::string ip, uint16_t port, bool ipv6 = false);
    explicit InetAddress(const struct sockaddr_in& addr);
    explicit InetAddress(const struct sockaddr_in6& addr6);

    // 获取字符串表示的IP地址
    std::string toIp() const;

    // 获取字符串表示的IP:端口
    std::string toIpPort() const;

    // 获取端口号
    uint16_t port() const;

    // 获取套接字地址
    const struct sockaddr* getSockAddr() const;
    void setSockAddrInet6(const struct sockaddr_in6& addr6);
    void setSockAddrInet(const struct sockaddr_in& addr);

    // 获取地址长度
    socklen_t getSockAddrLen() const;

    // 检查是否是IPv4映射的IPv6地址
    bool isIpv4MappedIpv6() const;

    // 获取sockaddr_in结构体
    const struct sockaddr_in& getSockAddrInet() const;
    const struct sockaddr_in6& getSockAddrInet6() const;

    // 返回一个代表任意地址的InetAddress（用于bind ANY）
    static InetAddress getAnyIpv4();
    static InetAddress getAnyIpv6();

private:
    union {
        struct sockaddr_in addr_;
        struct sockaddr_in6 addr6_;
    };
    bool ipv6_;
};

#endif // INET_ADDRESS_H