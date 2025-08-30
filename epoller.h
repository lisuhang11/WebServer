#ifndef EPOLLER_H
#define EPOLLER_H

#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include <memory>

class Channel;

class Epoller {
public:
    using ChannelList = std::vector<Channel*>;

    Epoller();
    ~Epoller();

    // 禁止拷贝构造和赋值
    Epoller(const Epoller&) = delete;
    Epoller& operator=(const Epoller&) = delete;

    // 等待事件发生
    int poll(int timeoutMs, ChannelList* activeChannels);

    // 更新Channel
    void updateChannel(Channel* channel);

    // 移除Channel
    void removeChannel(Channel* channel);

    // 检查Channel是否在Epoller中
    bool hasChannel(Channel* channel);

private:
    // 用于填充活跃的Channel
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

    // 更新Channel的辅助函数
    void update(int operation, Channel* channel);

    // 成员变量
    static const int kInitEventListSize = 16;
    int epollfd_;
    std::vector<struct epoll_event> events_;
    std::unordered_map<int, Channel*> channels_; // fd到Channel的映射
};

#endif // EPOLLER_H