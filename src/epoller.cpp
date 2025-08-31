#include "epoller.h"
#include "channel.h"
#include <iostream>
#include <cassert>
#include <unistd.h>
#include <cstring>

const int Epoller::kInitEventListSize;

Epoller::Epoller()
    : epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
    if (epollfd_ < 0) {
        std::cerr << "epoll_create1 error" << std::endl;
        abort();
    }
}

Epoller::~Epoller() {
    ::close(epollfd_);
}

int Epoller::poll(int timeoutMs, ChannelList* activeChannels) {
    int numEvents = ::epoll_wait(epollfd_, 
                                &*events_.begin(), 
                                static_cast<int>(events_.size()),
                                timeoutMs);
    int savedErrno = errno;
    
    if (numEvents > 0) {
        fillActiveChannels(numEvents, activeChannels);
        // 如果事件数量接近数组容量，扩容
        if (numEvents == static_cast<int>(events_.size())) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {
        // 超时，没有事件发生
    } else {
        // 错误处理
        if (savedErrno != EINTR) {
            std::cerr << "epoll_wait error" << std::endl;
        }
    }
    
    return numEvents;
}

void Epoller::updateChannel(Channel* channel) {
    int fd = channel->fd();
    int index = channel->index();
    
    if (index == -1) {
        // 新Channel，添加到epoll
        assert(channels_.find(fd) == channels_.end());
        channels_[fd] = channel;
        channel->set_index(0);
        update(EPOLL_CTL_ADD, channel);
    } else {
        // 已存在的Channel，更新事件
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        if (channel->isNoneEvent()) {
            // 如果没有关注的事件，从epoll中删除
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(-1);
        } else {
            // 更新事件
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void Epoller::removeChannel(Channel* channel) {
    int fd = channel->fd();
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());
    
    int index = channel->index();
    size_t n = channels_.erase(fd);
    assert(n == 1);
    
    if (index == 1) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(-1);
}

bool Epoller::hasChannel(Channel* channel) {
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

void Epoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const {
    assert(static_cast<size_t>(numEvents) <= events_.size());
    for (int i = 0; i < numEvents; ++i) {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void Epoller::update(int operation, Channel* channel) {
    struct epoll_event event;
    memset(&event, 0, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        std::cerr << "epoll_ctl error: operation=" << operation 
                  << ", fd=" << fd << std::endl;
        if (operation == EPOLL_CTL_DEL) {
            std::cerr << "epoll_ctl del error" << std::endl;
        } else {
            std::cerr << "epoll_ctl add/mod error" << std::endl;
            abort();
        }
    }
}