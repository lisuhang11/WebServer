#include "tcp_server.h"
#include <iostream>
#include <sstream>
#include <cassert>

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, 
                   const std::string& nameArg, bool reuseport)
    : loop_(loop),
      name_(nameArg),
      listenAddr_(listenAddr),
      acceptor_(new Acceptor(loop, listenAddr, reuseport)),
      threadPool_(new EventLoopThreadPool(loop, nameArg)),
      connectionCallback_(),
      messageCallback_(),
      writeCompleteCallback_(),
      threadInitCallback_(),
      started_(false),
      nextConnId_(1) {
    // 设置Acceptor的新连接回调
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, 
                 std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer() {
    loop_->assertInLoopThread();
    std::cout << "TcpServer::~TcpServer [" << name_ << "] destructing" << std::endl;

    for (auto& item : connections_) {
        TcpConnection::TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int numThreads) {
    assert(0 <= numThreads);
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start() {
    if (!started_) {
        started_ = true;
        threadPool_->start(threadInitCallback_);
        assert(!acceptor_->listening());
        loop_->runInLoop(
            std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr) {
    loop_->assertInLoopThread();
    
    // 为新连接选择一个EventLoop
    EventLoop* ioLoop = threadPool_->getNextLoop();
    
    // 生成连接名称
    char buf[64];
    snprintf(buf, sizeof buf, "%s#%d", name_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = buf;
    
    std::cout << "TcpServer::newConnection [" << name_ 
              << "] - new connection [" << connName 
              << "] from " << peerAddr.toIpPort() << std::endl;
    
    // 创建TcpConnection对象
    TcpConnection::TcpConnectionPtr conn(
        new TcpConnection(ioLoop, connName, sockfd, listenAddr_, peerAddr));
    
    // 记录连接
    connections_[connName] = conn;
    
    // 设置回调函数
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    
    // 在IO线程中建立连接
    ioLoop->runInLoop(
        std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnection::TcpConnectionPtr& conn) {
    // 在IO线程中移除连接
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnection::TcpConnectionPtr& conn) {
    loop_->assertInLoopThread();
    
    std::cout << "TcpServer::removeConnectionInLoop [" << name_ 
              << "] - connection [" << conn->name() << "]" << std::endl;
    
    // 从映射表中删除
    size_t n = connections_.erase(conn->name());
    assert(n == 1);
    
    // 在IO线程中销毁连接
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));
}

std::string TcpServer::removeConnectionName(int id) {
    char buf[64];
    snprintf(buf, sizeof buf, "%d", id);
    return buf;
}