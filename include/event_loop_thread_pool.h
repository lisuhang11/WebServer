#ifndef EVENT_LOOP_THREAD_POOL_H
#define EVENT_LOOP_THREAD_POOL_H

#include "event_loop.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

class EventLoopThread {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    explicit EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                            const std::string& name = "");
    ~EventLoopThread();

    // 启动线程并返回EventLoop指针
    EventLoop* startLoop();

private:
    // 线程函数
    void threadFunc();

    // 成员变量
    EventLoop* loop_;
    bool exiting_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
    std::string name_;
};

class EventLoopThreadPool {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    explicit EventLoopThreadPool(EventLoop* baseLoop,
                               const std::string& nameArg = "");
    ~EventLoopThreadPool();

    // 禁止拷贝构造和赋值
    EventLoopThreadPool(const EventLoopThreadPool&) = delete;
    EventLoopThreadPool& operator=(const EventLoopThreadPool&) = delete;

    // 设置线程数
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    // 启动线程池
    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    // 获取下一个EventLoop（轮询方式）
    EventLoop* getNextLoop();

    // 获取所有EventLoop
    std::vector<EventLoop*> getAllLoops();

    // 判断线程池是否已启动
    bool started() const { return started_; }

    // 获取线程池名称
    const std::string& name() const { return name_; }

private:
    // 成员变量
    EventLoop* baseLoop_; // 主线程的EventLoop
    std::string name_;
    bool started_;
    int numThreads_;
    int next_; // 轮询索引
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};

#endif // EVENT_LOOP_THREAD_POOL_H