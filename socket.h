#ifndef SOCKET_H
#define SOCKET_H

#include "inet_address.h"
#include <memory>
#include <unistd.h>

class Socket {
public:
    // 构造函数
    explicit Socket(int sockfd) : sockfd_(sockfd) {}
    
    // 析构函数
    ~Socket();
    
    // 禁止拷贝构造和赋值
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    
    // 移动构造和移动赋值
    Socket(Socket&& rhs) noexcept : sockfd_(rhs.sockfd_) {
        rhs.sockfd_ = -1;
    }
    
    Socket& operator=(Socket&& rhs) noexcept {
        if (this != &rhs) {
            ::close(sockfd_);
            sockfd_ = rhs.sockfd_;
            rhs.sockfd_ = -1;
        }
        return *this;
    }
    
    // 获取文件描述符
    int fd() const { return sockfd_; }
    
    // 绑定地址
    void bindAddress(const InetAddress& localaddr);
    
    // 监听连接
    void listen();
    
    // 接受连接
    int accept(InetAddress* peeraddr);
    
    // 关闭写端
    void shutdownWrite();
    
    // 设置选项
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
    
    // 创建非阻塞socket
    static int createNonblockingOrDie(sa_family_t family);
    
private:
    int sockfd_;
};

#endif // SOCKET_H