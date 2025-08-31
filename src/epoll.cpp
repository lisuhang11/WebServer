#include "epoll.h"
#include <unistd.h>
#include <cstring>
#include <iostream>

// 构造函数
Epoll::Epoll() {
    epollFd_ = epoll_create1(0);
    if (epollFd_ < 0) {
        std::cerr << "epoll_create1 failed" << std::endl;
        exit(1);
    }
    events_.resize(MAX_EVENTS);
}

// 析构函数
Epoll::~Epoll() {
    close(epollFd_);
}

// 添加文件描述符到epoll
bool Epoll::addFd(int fd, uint32_t events) {
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.fd = fd;
    event.events = events;
    
    return epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) == 0;
}

// 修改文件描述符的事件
bool Epoll::modFd(int fd, uint32_t events) {
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.fd = fd;
    event.events = events;
    
    return epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) == 0;
}

// 从epoll中删除文件描述符
bool Epoll::delFd(int fd) {
    return epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) == 0;
}

// 等待事件发生
int Epoll::wait(int timeoutMs) {
    return epoll_wait(epollFd_, events_.data(), MAX_EVENTS, timeoutMs);
}

// 获取活跃的事件列表
const std::vector<struct epoll_event>& Epoll::getEvents() const {
    return events_;
}

// 获取epoll文件描述符
int Epoll::getEpollFd() const {
    return epollFd_;
}