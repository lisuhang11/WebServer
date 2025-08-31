#ifndef CHANNEL_H
#define CHANNEL_H

#include <functional>
#include <memory>
#include <sys/epoll.h>

class EventLoop;

class Channel {
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void()>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    // 禁止拷贝构造和赋值
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    // 处理事件
    void handleEvent();

    // 设置回调函数
    void setReadCallback(ReadEventCallback cb) {
        readCallback_ = std::move(cb);
    }

    void setWriteCallback(EventCallback cb) {
        writeCallback_ = std::move(cb);
    }

    void setCloseCallback(EventCallback cb) {
        closeCallback_ = std::move(cb);
    }

    void setErrorCallback(EventCallback cb) {
        errorCallback_ = std::move(cb);
    }

    // 获取文件描述符
    int fd() const { return fd_; }

    // 获取关注的事件
    uint32_t events() const { return events_; }

    // 设置发生的事件
    void set_revents(uint32_t revt) { revents_ = revt; }

    // 判断是否没有事件关注
    bool isNoneEvent() const { return events_ == kNoneEvent; }

    // 启用读事件
    void enableReading() {
        events_ |= kReadEvent;
        update();
    }

    // 禁用读事件
    void disableReading() {
        events_ &= ~kReadEvent;
        update();
    }

    // 启用写事件
    void enableWriting() {
        events_ |= kWriteEvent;
        update();
    }

    // 禁用写事件
    void disableWriting() {
        events_ &= ~kWriteEvent;
        update();
    }

    // 禁用所有事件
    void disableAll() {
        events_ = kNoneEvent;
        update();
    }

    // 判断是否关注读事件
    bool isReading() const { return events_ & kReadEvent; }

    // 判断是否关注写事件
    bool isWriting() const { return events_ & kWriteEvent; }

    // 获取Channel索引（用于Poller）
    int index() { return index_; }

    // 设置Channel索引
    void set_index(int idx) { index_ = idx; }

    // 获取EventLoop指针
    EventLoop* ownerLoop() { return loop_; }

private:
    // 更新Channel在Poller中的状态
    void update();

    // 处理事件的辅助函数
    static const uint32_t kNoneEvent;
    static const uint32_t kReadEvent;
    static const uint32_t kWriteEvent;

    // 成员变量
    EventLoop* loop_;  // 所属EventLoop
    const int fd_;     // 文件描述符
    uint32_t events_;  // 关注的事件
    uint32_t revents_; // 发生的事件
    int index_;        // 在Poller中的索引
    
    // 回调函数
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};

#endif // CHANNEL_H