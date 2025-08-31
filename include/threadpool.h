#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <atomic>

class ThreadPool {
public:
    using Task = std::function<void()>;

    // 构造函数，创建指定数量的线程
    explicit ThreadPool(size_t threadCount = 4);
    
    // 析构函数，等待所有线程完成
    ~ThreadPool();
    
    // 禁止拷贝构造和赋值
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    
    // 添加任务到线程池
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

private:
    // 工作线程函数
    void workerThread();
    
    // 线程池状态
    std::atomic<bool> stop_;
    
    // 线程列表
    std::vector<std::thread> workers_;
    
    // 任务队列
    std::queue<Task> tasks_;
    
    // 同步所需的互斥锁和条件变量
    std::mutex queueMutex_;
    std::condition_variable condition_;
};

// 模板函数实现
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using ReturnType = typename std::result_of<F(Args...)>::type;
    
    // 创建一个包装任务，返回future以便获取结果
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<ReturnType> result = task->get_future();
    
    // 将任务添加到队列
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        
        // 如果线程池已停止，则抛出异常
        if (stop_) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        
        // 添加任务到队列
        tasks_.emplace([task]() { (*task)(); });
    }
    
    // 通知一个等待的线程有新任务
    condition_.notify_one();
    
    return result;
}

#endif // THREADPOOL_H