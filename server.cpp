#include "server.h"
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

// 全局服务器指针，用于信号处理
static Server* g_server = nullptr;

// 构造函数
Server::Server(int port, const std::string &rootPath, int threadCount)
    : port_(port), rootPath_(rootPath), listenFd_(-1), isRunning_(false) {
    g_server = this;
    
    // 初始化线程池
    threadPool_.reset(new ThreadPool(threadCount));
    
    // 初始化epoll
    epoll_.reset(new Epoll());
    
    // 初始化信号管道
    if (pipe(pipeFd_) < 0) {
        std::cerr << "pipe failed" << std::endl;
        exit(1);
    }
    
    // 设置信号处理函数
    setSignalHandler();
    
    // 初始化服务器套接字
    if (!initSocket()) {
        std::cerr << "init socket failed" << std::endl;
        exit(1);
    }
}

// 析构函数
Server::~Server() {
    stop();
}

// 启动服务器
void Server::start() {
    std::cout << "WebFileServer started on port " << port_ << std::endl;
    std::cout << "Root path: " << rootPath_ << std::endl;
    
    // 添加监听套接字到epoll
    epoll_->addFd(listenFd_, EPOLLIN | EPOLLET);
    
    // 添加信号管道读取端到epoll
    epoll_->addFd(pipeFd_[0], EPOLLIN | EPOLLET);
    
    isRunning_ = true;
    
    // 事件循环
    while (isRunning_) {
        int eventCount = epoll_->wait(-1);
        if (eventCount < 0) {
            if (errno == EINTR) {
                continue; // 被信号中断
            }
            std::cerr << "epoll_wait failed" << std::endl;
            break;
        }
        
        // 处理每个事件
        for (int i = 0; i < eventCount; ++i) {
            int sockFd = epoll_->getEvents()[i].data.fd;
            uint32_t events = epoll_->getEvents()[i].events;
            
            // 处理信号管道事件
            if (sockFd == pipeFd_[0] && (events & EPOLLIN)) {
                handleSignal(sockFd);
                continue;
            }
            
            // 处理监听套接字事件（新连接）
            if (sockFd == listenFd_ && (events & EPOLLIN)) {
                handleNewConnection();
                continue;
            }
            
            // 处理读写事件
            handleEvent(sockFd, events);
        }
    }
}

// 停止服务器
void Server::stop() {
    isRunning_ = false;
    
    // 关闭监听套接字
    if (listenFd_ != -1) {
        close(listenFd_);
        listenFd_ = -1;
    }
    
    // 关闭信号管道
    close(pipeFd_[0]);
    close(pipeFd_[1]);
    
    // 清空连接
    connections_.clear();
    
    std::cout << "WebFileServer stopped" << std::endl;
}

// 设置信号处理函数
void Server::setSignalHandler() {
    // 设置忽略SIGPIPE信号
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, nullptr);
    
    // 设置SIGINT和SIGTERM信号处理函数
    sa.sa_handler = [](int sig) {
        if (g_server) {
            // 向管道写入信号，通知主线程
            int msg = sig;
            send(g_server->pipeFd_[1], (char*)&msg, sizeof(msg), 0);
        }
    };
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}

// 初始化服务器套接字
bool Server::initSocket() {
    // 创建套接字
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        std::cerr << "socket failed" << std::endl;
        return false;
    }
    
    // 设置套接字选项：重用地址和端口
    int opt = 1;
    if (setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt failed" << std::endl;
        close(listenFd_);
        return false;
    }
    
    // 绑定地址和端口
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(listenFd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "bind failed" << std::endl;
        close(listenFd_);
        return false;
    }
    
    // 开始监听
    if (listen(listenFd_, 1024) < 0) {
        std::cerr << "listen failed" << std::endl;
        close(listenFd_);
        return false;
    }
    
    // 设置非阻塞
    int flags = fcntl(listenFd_, F_GETFL, 0);
    if (flags < 0) {
        std::cerr << "fcntl failed" << std::endl;
        close(listenFd_);
        return false;
    }
    if (fcntl(listenFd_, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "fcntl failed" << std::endl;
        close(listenFd_);
        return false;
    }
    
    return true;
}

// 处理新的连接
void Server::handleNewConnection() {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    
    // 循环接受所有连接（边缘触发模式需要一次性处理完）
    while (true) {
        int clientFd = accept(listenFd_, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientFd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // 没有更多连接了
            }
            std::cerr << "accept failed" << std::endl;
            break;
        }
        
        // 设置非阻塞
        int flags = fcntl(clientFd, F_GETFL, 0);
        if (flags >= 0) {
            fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);
        }
        
        // 添加到epoll
        if (epoll_->addFd(clientFd, EPOLLIN | EPOLLET | EPOLLRDHUP)) {
            // 创建HTTP连接对象
            connections_[clientFd].reset(new HttpConnection(clientFd, rootPath_));
            std::cout << "New connection from " << inet_ntoa(clientAddr.sin_addr) 
                      << ":" << ntohs(clientAddr.sin_port) << std::endl;
        } else {
            close(clientFd);
            std::cerr << "Failed to add client to epoll" << std::endl;
        }
    }
}

// 处理读写事件
void Server::handleEvent(int sockFd, uint32_t events) {
    // 查找对应的连接
    auto it = connections_.find(sockFd);
    if (it == connections_.end()) {
        return;
    }
    
    HttpConnection* conn = it->second.get();
    
    // 如果连接已关闭
    if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        std::cout << "Connection closed: " << sockFd << std::endl;
        epoll_->delFd(sockFd);
        connections_.erase(it);
        return;
    }
    
    // 处理读事件
    if (events & EPOLLIN) {
        int savedErrno = 0;
        ssize_t len = conn->read(&savedErrno);
        if (len < 0) {
            if (savedErrno != EAGAIN && savedErrno != EWOULDBLOCK) {
                std::cerr << "Read error: " << savedErrno << std::endl;
                epoll_->delFd(sockFd);
                connections_.erase(it);
            }
            return;
        } else if (len == 0) {
            std::cout << "Client closed connection: " << sockFd << std::endl;
            epoll_->delFd(sockFd);
            connections_.erase(it);
            return;
        }
        
        // 提交任务到线程池处理请求
        threadPool_->enqueue([conn, this, sockFd, it]() {
            if (conn->process()) {
                // 处理完成后，注册写事件
                epoll_->modFd(sockFd, EPOLLOUT | EPOLLET | EPOLLRDHUP);
            } else {
                // 处理失败，关闭连接
                epoll_->delFd(sockFd);
                connections_.erase(it);
            }
        });
    }
    
    // 处理写事件
    if (events & EPOLLOUT) {
        int savedErrno = 0;
        ssize_t len = conn->write(&savedErrno);
        if (len < 0) {
            if (savedErrno != EAGAIN && savedErrno != EWOULDBLOCK) {
                std::cerr << "Write error: " << savedErrno << std::endl;
                epoll_->delFd(sockFd);
                connections_.erase(it);
            }
            return;
        }
        
        // 检查是否所有数据都已发送
        if (conn->isProcessing() == false && len > 0) {
            // 重新注册读事件，等待下一次请求
            epoll_->modFd(sockFd, EPOLLIN | EPOLLET | EPOLLRDHUP);
            conn->reset();
        }
    }
}

// 处理信号
void Server::handleSignal(int pipeFd) {
    // 读取信号值
    int sigVal;
    read(pipeFd, &sigVal, sizeof(sigVal));
    
    // 处理不同的信号
    switch (sigVal) {
        case SIGINT:
        case SIGTERM:
            std::cout << "Received signal, stopping server..." << std::endl;
            stop();
            break;
        default:
            break;
    }
}