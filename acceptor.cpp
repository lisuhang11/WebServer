#include "acceptor.h"
#include "event_loop.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cassert>

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
    : loop_(loop),
      acceptSocket_(Socket::createNonblockingOrDie(listenAddr.getSockAddr()->sa_family)),
      acceptChannel_(loop, acceptSocket_.fd()),
      newConnectionCallback_(),
      listening_(false),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
    assert(idleFd_ >= 0);
    
    // 设置socket选项
    acceptSocket_.setReuseAddr(true);
    if (reuseport) {
        acceptSocket_.setReusePort(true);
    }
    
    // 绑定地址
    acceptSocket_.bindAddress(listenAddr);
    
    // 设置Channel的读事件回调
    acceptChannel_.setReadCallback(
        std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    // 使用EventLoop的removeChannel()方法替代Channel的remove()方法
    loop_->removeChannel(&acceptChannel_);
    ::close(idleFd_);
}

void Acceptor::listen() {
    loop_->assertInLoopThread();
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead() {
    loop_->assertInLoopThread();
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0) {
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        } else {
            // 如果没有设置回调，关闭连接
            ::close(connfd);
        }
    } else {
        // 错误处理
        int savedErrno = errno;
        std::cerr << "in Acceptor::handleRead" << std::endl;
        if (savedErrno == EMFILE) {
            // 文件描述符耗尽，使用预先准备的idleFd_处理
            ::close(idleFd_);
            idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}