#include "threadpool.h"

// 构造函数，创建指定数量的线程
ThreadPool::ThreadPool(size_t threadCount) : stop_(false) {
    // 创建指定数量的工作线程
    workers_.reserve(threadCount);
    for (size_t i = 0; i < threadCount; ++i) {
        workers_.emplace_back(&ThreadPool::workerThread, this);
    }
}

// 析构函数，等待所有线程完成
ThreadPool::~ThreadPool() {
    // 设置停止标志
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        stop_ = true;
    }
    
    // 通知所有等待的线程
    condition_.notify_all();
    
    // 等待所有线程完成
    for (std::thread &worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

// 工作线程函数
void ThreadPool::workerThread() {
    while (true) {
        Task task;
        
        // 尝试从队列获取任务
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            // 等待直到有任务或线程池停止
            condition_.wait(lock, [this] { 
                return stop_ || !tasks_.empty(); 
            });
            
            // 如果线程池已停止且任务队列为空，则退出循环
            if (stop_ && tasks_.empty()) {
                return;
            }
            
            // 获取任务
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        
        // 执行任务
        task();
    }
}