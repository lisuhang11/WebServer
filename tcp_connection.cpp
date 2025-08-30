#include "tcp_connection.h"
#include "event_loop.h"
#include "http_connection.h"
#include <iostream>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <cassert>

TcpConnection::TcpConnection(EventLoop* loop, const std::string& nameArg, int sockfd,
                           const InetAddress& localAddr, const InetAddress& peerAddr)
    : loop_(loop),
      name_(nameArg),
      state_(kDisconnected),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64*1024*1024) {
    // 设置Channel的回调函数
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));

    // 输出连接信息
    std::cout << "TcpConnection::ctor[" << name_ << "] at " << this
              << " fd=" << sockfd << std::endl;

    // 设置TCP_NODELAY选项，禁用Nagle算法
    socket_->setTcpNoDelay(true);
}

TcpConnection::~TcpConnection() {
    std::cout << "TcpConnection::dtor[" << name_ << "] at " << this
              << " fd=" << channel_->fd()
              << " state=" << state_ << std::endl;
}

void TcpConnection::send(const void* message, int len) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(message, len);
        } else {
            void (TcpConnection::*fp)(const std::string& message) = &TcpConnection::send;
            loop_->runInLoop(
                std::bind(fp, shared_from_this(), std::string(static_cast<const char*>(message), len)));
        }
    }
}

void TcpConnection::send(const std::string& message) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(message.data(), message.size());
        } else {
            void (TcpConnection::*fp)(const std::string& message) = &TcpConnection::send;
            loop_->runInLoop(
                std::bind(fp, shared_from_this(), message));
        }
    }
}

void TcpConnection::sendInLoop(const void* data, size_t len) {
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    if (state_ == kDisconnected) {
        std::cerr << "disconnected, give up writing" << std::endl;
        return;
    }

    // 如果输出缓冲区为空，尝试直接写入
    if (!channel_->isWriting() && outputBuffer_.empty()) {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_) {
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this()));
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                std::cerr << "TcpConnection::sendInLoop error" << std::endl;
                if (errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                }
            }
        }
    }

    assert(remaining <= len);
    // 如果还有数据未发送，添加到输出缓冲区
    if (!faultError && remaining > 0) {
        size_t oldLen = outputBuffer_.size();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_) {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaining);
        if (!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdown() {
    if (state_ == kConnected) {
        setState(kDisconnecting);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
    }
}

void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if (!channel_->isWriting()) {
        socket_->shutdownWrite();
    }
}

void TcpConnection::forceClose() {
    if (state_ == kConnected || state_ == kDisconnecting) {
        setState(kDisconnecting);
        loop_->queueInLoop(
            std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceCloseInLoop() {
    loop_->assertInLoopThread();
    if (state_ == kConnected || state_ == kDisconnecting) {
        handleClose();
    }
}

void TcpConnection::connectEstablished() {
    loop_->assertInLoopThread();
    // 允许state_为kDisconnected或kConnecting
    assert(state_ == kDisconnected || state_ == kConnecting);
    setState(kConnected);
    // 暂时移除tie()方法调用，因为Channel类没有该方法
    channel_->enableReading();

    if (connectionCallback_) {
        connectionCallback_(shared_from_this());
    }
}

void TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();
    if (state_ == kConnected) {
        setState(kDisconnected);
        if (channel_) {
            channel_->disableAll();
        }
        if (connectionCallback_) {
            connectionCallback_(shared_from_this());
        }
    }
    // 使用EventLoop的removeChannel()方法替代Channel的remove()方法
    if (loop_ && channel_) {
        loop_->removeChannel(channel_.get());
    }
}

void TcpConnection::handleRead() {
    loop_->assertInLoopThread();
    int savedErrno = 0;
    char buf[65536]; // 64KB buffer
    ssize_t n = ::read(channel_->fd(), buf, sizeof buf);
    
    if (n > 0) {
        // 读取数据到输入缓冲区
        inputBuffer_.append(buf, n);
        
        // 打印收到的请求（用于调试）
        printf("收到请求: %s\n", inputBuffer_.c_str());
        
        // 构建正确的HTTP响应
        std::string body = "Hello from WebFileServer!\n"; // 响应正文
        std::string response = 
            "HTTP/1.1 200 OK\r\n" 
            "Content-Type: text/plain\r\n" 
            "Content-Length: " + std::to_string(body.size()) + "\r\n" 
            "Connection: close\r\n" 
            "\r\n" 
            + body;
            
        send(response);
        
        // 发送完响应后关闭连接
        shutdown();
    } else if (n == 0) {
        handleClose();
    } else {
        errno = savedErrno;
        std::cerr << "TcpConnection::handleRead error" << std::endl;
        handleError();
    }
}

void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();
    if (channel_->isWriting()) {
        ssize_t n = ::write(channel_->fd(), outputBuffer_.data(), outputBuffer_.size());
        if (n > 0) {
            outputBuffer_.erase(0, n);
            if (outputBuffer_.empty()) {
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            std::cerr << "TcpConnection::handleWrite error" << std::endl;
        }
    } else {
        std::cerr << "Connection fd = " << channel_->fd()
                  << " is down, no more writing" << std::endl;
    }
}

void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    std::cout << "fd = " << channel_->fd() << " state = " << state_ << std::endl;
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);
    closeCallback_(guardThis);
}

void TcpConnection::handleError() {
    int optval;
    socklen_t optlen = sizeof optval;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        optval = errno;
    }
    std::cerr << "TcpConnection::handleError [" << name_ << "] - SO_ERROR = "
              << optval << " " << strerror(optval) << std::endl;
}