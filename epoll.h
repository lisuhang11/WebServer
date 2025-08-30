#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>
#include <vector>
#include <functional>
#include <memory>

class Epoll {
public:
    using EventCallback = std::function<void(int)>;
    using ReadEventCallback = std::function<void(int)>;
    using WriteEventCallback = std::function<void(int)>;
    using CloseEventCallback = std::function<void(int)>;
    using ErrorEventCallback = std::function<void(int)>;

    // 构造函数
    Epoll();
    
    // 析构函数
    ~Epoll();
    
    // 禁止拷贝构造和赋值
    Epoll(const Epoll&) = delete;
    Epoll& operator=(const Epoll&) = delete;
    
    // 添加文件描述符到epoll
    bool addFd(int fd, uint32_t events);
    
    // 修改文件描述符的事件
    bool modFd(int fd, uint32_t events);
    
    // 从epoll中删除文件描述符
    bool delFd(int fd);
    
    // 等待事件发生
    int wait(int timeoutMs = -1);
    
    // 获取活跃的事件列表
    const std::vector<struct epoll_event>& getEvents() const;
    
    // 获取epoll文件描述符
    int getEpollFd() const;
    
private:
    // epoll文件描述符
    int epollFd_;
    
    // 事件列表
    std::vector<struct epoll_event> events_;
    
    // 最大事件数量
    static const int MAX_EVENTS = 1024;
};

#endif // EPOLL_H