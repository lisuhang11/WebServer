#include "event_loop.h"
#include "tcp_server.h"
#include "inet_address.h"
#include <iostream>
#include <signal.h>
#include <memory>
#include <cstring>

EventLoop* g_loop = nullptr;

// 信号处理函数
void sigHandler(int sig) {
    printf("signal: %d\n", sig);
    // 安全退出事件循环
    if (g_loop) {
        g_loop->quit();
    }
}

int main(int argc, char* argv[]) {
    // 设置信号处理
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
    
    // 创建事件循环
    EventLoop loop;
    g_loop = &loop;
    
    // 设置服务器参数
    const int port = 8888;
    const int threadNum = 4;
    InetAddress listenAddr(port);
    
    // 创建TCP服务器
    TcpServer server(&loop, listenAddr, "WebFileServer");
    
    // 设置线程池大小
    server.setThreadNum(threadNum);
    
    // 设置连接回调函数
    server.setConnectionCallback([](const TcpConnection::TcpConnectionPtr& conn) {
        if (conn->connected()) {
            printf("新连接建立: %s\n", conn->peerAddress().toIpPort().c_str());
        } else {
            printf("连接关闭: %s\n", conn->peerAddress().toIpPort().c_str());
        }
    });
    
    // 设置消息回调函数
    server.setMessageCallback([](const TcpConnection::TcpConnectionPtr& conn, std::string* message) {
        printf("收到消息: %s, 长度: %d\n", message->c_str(), static_cast<int>(message->size()));
        conn->send(*message); // 回显消息
    });
    
    // 设置写完成回调函数
    server.setWriteCompleteCallback([](const TcpConnection::TcpConnectionPtr& conn) {
        printf("写完成回调\n");
    });
    
    // 设置线程初始化回调函数
    server.setThreadInitCallback([](EventLoop* loop) {
        printf("线程初始化\n");
    });
    
    // 启动服务器
    printf("Starting HTTPServer on port %d\n", port);
    printf("Using %d threads for handling connections\n", threadNum);
    server.start();
    
    // 运行事件循环
    loop.loop();
    
    return 0;
}