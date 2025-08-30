#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "channel.h"
#include "socket.h"
#include "inet_address.h"
#include <functional>
#include <memory>

class EventLoop;

class Acceptor {
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    // 禁止拷贝构造和赋值
    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;

    // 设置新连接回调函数
    void setNewConnectionCallback(const NewConnectionCallback& cb) {
        newConnectionCallback_ = cb;
    }

    // 判断是否正在监听
    bool listening() const { return listening_; }

    // 开始监听
    void listen();

private:
    // 处理连接事件
    void handleRead();

    // 成员变量
    EventLoop* loop_; // 所属EventLoop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listening_;
    int idleFd_; // 用于处理文件描述符耗尽的情况
};

#endif // ACCEPTOR_H