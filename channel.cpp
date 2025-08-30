#include "channel.h"
#include "event_loop.h"
#include <iostream>
#include <cassert>
#include <poll.h>

// 静态常量定义
const uint32_t Channel::kNoneEvent = 0;
const uint32_t Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const uint32_t Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(kNoneEvent),
      revents_(kNoneEvent),
      index_(-1) {
}

Channel::~Channel() {
    assert(!isReading() && !isWriting());
}

void Channel::handleEvent() {
    // 处理关闭事件
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) closeCallback_();
    }
    
    // 处理错误事件
    if (revents_ & EPOLLERR) {
        if (errorCallback_) errorCallback_();
    }
    
    // 处理可读事件
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (readCallback_) readCallback_();
    }
    
    // 处理可写事件
    if (revents_ & EPOLLOUT) {
        if (writeCallback_) writeCallback_();
    }
}

void Channel::update() {
    loop_->updateChannel(this);
}