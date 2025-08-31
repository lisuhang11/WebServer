#include "socket.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>

Socket::~Socket() {
    if (sockfd_ >= 0) {
        ::close(sockfd_);
        sockfd_ = -1;
    }
}

void Socket::bindAddress(const InetAddress& localaddr) {
    int ret = ::bind(sockfd_, localaddr.getSockAddr(), localaddr.getSockAddrLen());
    if (ret < 0) {
        std::cerr << "Socket::bindAddress error" << std::endl;
        abort();
    }
}

void Socket::listen() {
    int ret = ::listen(sockfd_, SOMAXCONN);
    if (ret < 0) {
        std::cerr << "Socket::listen error" << std::endl;
        abort();
    }
}

int Socket::accept(InetAddress* peeraddr) {
    sockaddr_in6 addr6;
    socklen_t addrlen = sizeof addr6;
    int connfd = ::accept4(sockfd_, reinterpret_cast<sockaddr*>(&addr6), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0) {
        if (addr6.sin6_family == AF_INET6) {
            peeraddr->setSockAddrInet6(addr6);
        } else {
            // IPv4地址
            const sockaddr_in* addr4 = reinterpret_cast<const sockaddr_in*>(&addr6);
            peeraddr->setSockAddrInet(*addr4);
        }
    } else {
        int savedErrno = errno;
        std::cerr << "Socket::accept error" << std::endl;
        switch (savedErrno) {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO:
            case EPERM:
            case EMFILE:
                // 这些错误是可以忽略的临时错误
                errno = savedErrno;
                break;
            default:
                std::cerr << "Unexpected error in Socket::accept" << std::endl;
                abort();
                break;
        }
    }
    return connfd;
}

void Socket::shutdownWrite() {
    if (::shutdown(sockfd_, SHUT_WR) < 0) {
        std::cerr << "Socket::shutdownWrite error" << std::endl;
    }
}

void Socket::setTcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}

void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}

void Socket::setReusePort(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}

void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}

int Socket::createNonblockingOrDie(sa_family_t family) {
    int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        std::cerr << "Socket::createNonblockingOrDie error" << std::endl;
        abort();
    }
    return sockfd;
}