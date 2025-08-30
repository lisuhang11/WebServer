#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <memory>
#include "epoll.h"
#include "threadpool.h"
#include "http_connection.h"

class Server {
public:
    // 构造函数
    explicit Server(int port = 8888, const std::string &rootPath = "./", int threadCount = 4);
    
    // 析构函数
    ~Server();
    
    // 禁止拷贝构造和赋值
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    
    // 启动服务器
    void start();
    
    // 停止服务器
    void stop();
    
    // 设置信号处理函数
    void setSignalHandler();
    
private:
    // 初始化服务器套接字
    bool initSocket();
    
    // 处理新的连接
    void handleNewConnection();
    
    // 处理读写事件
    void handleEvent(int sockFd, uint32_t events);
    
    // 处理信号
    void handleSignal(int sig);
    
    // 服务器端口
    int port_;
    
    // 服务器根目录
    std::string rootPath_;
    
    // 服务器套接字
    int listenFd_;
    
    // 线程池
    std::unique_ptr<ThreadPool> threadPool_;
    
    // epoll对象
    std::unique_ptr<Epoll> epoll_;
    
    // HTTP连接映射
    std::unordered_map<int, std::unique_ptr<HttpConnection>> connections_;
    
    // 服务器是否运行
    bool isRunning_;
    
    // 信号管道
    int pipeFd_[2];
};

#endif // SERVER_H