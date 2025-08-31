#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "event_loop.h"
#include "inet_address.h"
#include "tcp_connection.h"
#include "event_loop_thread_pool.h"
#include "acceptor.h"
#include <memory>
#include <string>
#include <map>
#include <functional>

class TcpServer {
public:
    using ConnectionCallback = std::function<void(const TcpConnection::TcpConnectionPtr&)>;
    using MessageCallback = std::function<void(const TcpConnection::TcpConnectionPtr&, std::string*)>;
    using WriteCompleteCallback = std::function<void(const TcpConnection::TcpConnectionPtr&)>;
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    TcpServer(EventLoop* loop, const InetAddress& listenAddr, 
             const std::string& nameArg, bool reuseport = false);
    ~TcpServer();

    // 禁止拷贝构造和赋值
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    // 设置线程池大小
    void setThreadNum(int numThreads);

    // 启动服务器
    void start();

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
    
    void setThreadInitCallback(const ThreadInitCallback& cb) {
        threadInitCallback_ = cb;
    }

    // 获取连接名称
    std::string removeConnectionName(int id);

private:
    // 新连接回调
    void newConnection(int sockfd, const InetAddress& peerAddr);
    
    // 移除连接
    void removeConnection(const TcpConnection::TcpConnectionPtr& conn);
    
    // 在IO线程中移除连接
    void removeConnectionInLoop(const TcpConnection::TcpConnectionPtr& conn);

    // 成员变量
    EventLoop* loop_;                                  // 主事件循环
    const std::string name_;                           // 服务器名称
    const InetAddress listenAddr_;                     // 监听地址
    std::unique_ptr<Acceptor> acceptor_;               // 接受器
    std::unique_ptr<EventLoopThreadPool> threadPool_;  // 线程池
    
    // 回调函数
    ConnectionCallback connectionCallback_;            // 连接回调
    MessageCallback messageCallback_;                  // 消息回调
    WriteCompleteCallback writeCompleteCallback_;      // 写完成回调
    ThreadInitCallback threadInitCallback_;            // 线程初始化回调
    
    // 连接管理
    std::map<std::string, TcpConnection::TcpConnectionPtr> connections_; // 连接映射
    bool started_;                                     // 是否启动
    int nextConnId_;                                   // 下一个连接ID
};

#endif // TCP_SERVER_H