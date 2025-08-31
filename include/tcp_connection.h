#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include "inet_address.h"
#include "channel.h"
#include "socket.h"
#include <memory>
#include <string>
#include <functional>

class EventLoop;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
    using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&, std::string*)>;

    TcpConnection(EventLoop* loop, const std::string& nameArg, int sockfd,
                 const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection();

    // 禁止拷贝构造和赋值
    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    // 获取EventLoop
    EventLoop* getLoop() const { return loop_; }

    // 获取连接名称
    const std::string& name() const { return name_; }

    // 获取本地地址和对端地址
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }

    // 判断连接是否建立
    bool connected() const { return state_ == kConnected; }

    // 判断连接是否断开
    bool disconnected() const { return state_ == kDisconnected; }

    // 发送数据
    void send(const void* message, int len);
    void send(const std::string& message);
    
    // 关闭连接
    void shutdown();
    
    // 强制关闭连接
    void forceClose();
    
    // 设置回调函数
    void setConnectionCallback(const ConnectionCallback& cb) {
        connectionCallback_ = cb;
    }
    
    void setMessageCallback(const MessageCallback& cb) {
        messageCallback_ = cb;
    }
    
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
        writeCompleteCallback_ = cb;
    }
    
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }
    
    void setCloseCallback(const CloseCallback& cb) {
        closeCallback_ = cb;
    }
    
    // 连接建立
    void connectEstablished();
    
    // 连接销毁
    void connectDestroyed();
    
    // 获取socket文件描述符
    int getFd() const { return socket_->fd(); }

private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
    
    void setState(StateE s) { state_ = s; }
    
    // 处理读写事件
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();
    
    // 发送缓冲区数据
    void sendInLoop(const void* message, size_t len);
    
    // 关闭连接
    void shutdownInLoop();
    
    void forceCloseInLoop();

    // 成员变量
    EventLoop* loop_;
    const std::string name_;
    StateE state_;
    
    // 套接字相关
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    
    // 地址信息
    const InetAddress localAddr_;
    const InetAddress peerAddr_;
    
    // 回调函数
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    
    // 缓冲区和水位线
    size_t highWaterMark_;
    std::string inputBuffer_;
    std::string outputBuffer_;
};

#endif // TCP_CONNECTION_H