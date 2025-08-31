#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <functional>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include "timestamp.h"

class Channel;
class Epoller;

class EventLoop {
public:
    using Functor = std::function<void()>;
    using TaskQueue = std::vector<Functor>;

    EventLoop();
    ~EventLoop();

    // 禁止拷贝构造和赋值
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    // 运行事件循环
    void loop();

    // 退出事件循环
    void quit();

    // 断言当前是否在创建该EventLoop的线程中
    void assertInLoopThread();

    // 检查当前线程是否是创建该EventLoop的线程
    bool isInLoopThread() const;

    // 在IO线程中执行回调
    void runInLoop(Functor cb);

    // 将回调放入队列，在IO线程中执行
    void queueInLoop(Functor cb);

    // 唤醒IO线程
    void wakeup();

    // 更新Channel
    void updateChannel(Channel* channel);

    // 移除Channel
    void removeChannel(Channel* channel);

    // 检查Channel是否在当前EventLoop中
    bool hasChannel(Channel* channel);

    // 添加定时任务
    void runAfter(double delay, Functor cb);
    void runEvery(double interval, Functor cb);

private:
    // 处理唤醒事件
    void handleRead();

    // 执行所有待处理的回调
    void doPendingFunctors();

    // 检查是否在创建线程中
    void abortNotInLoopThread();

    // 成员变量
    std::atomic<bool> looping_;
    std::atomic<bool> quit_;
    std::atomic<bool> callingPendingFunctors_;
    const pthread_t threadId_;
    int wakeupFd_;
    std::unique_ptr<Epoller> poller_;
    std::unique_ptr<Channel> wakeupChannel_;
    TaskQueue pendingFunctors_;
    std::mutex mutex_;
    
    // 活跃的Channel列表
    std::vector<Channel*> activeChannels_;
};

#endif // EVENT_LOOP_H