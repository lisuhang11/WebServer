#include "event_loop.h"
#include "channel.h"
#include "epoller.h"
#include <iostream>
#include <cassert>
#include <sys/eventfd.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <functional>

namespace {
    // 创建eventfd用于唤醒线程
    int createEventfd() {
        int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (evtfd < 0) {
            std::cerr << "Failed in eventfd" << std::endl;
            abort();
        }
        return evtfd;
    }

    // 忽略SIGPIPE信号
    class IgnoreSigPipe {
    public:
        IgnoreSigPipe() {
            ::signal(SIGPIPE, SIG_IGN);
        }
    };

    IgnoreSigPipe initObj;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      callingPendingFunctors_(false),
      threadId_(::pthread_self()),
      wakeupFd_(createEventfd()),
      poller_(new Epoller()),
      wakeupChannel_(new Channel(this, wakeupFd_)) {
    wakeupChannel_->setReadCallback(
        std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    wakeupChannel_->disableAll();
    // 使用disableAll()替代remove()，因为Channel类没有remove()方法
    ::close(wakeupFd_);
}

void EventLoop::loop() {
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;

    while (!quit_) {
        activeChannels_.clear();
        int numEvents = poller_->poll(-1, &activeChannels_);
        
        for (Channel* channel : activeChannels_) {
            channel->handleEvent();
        }
        
        doPendingFunctors();
    }

    looping_ = false;
}

void EventLoop::quit() {
    quit_ = true;
    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::assertInLoopThread() {
    if (!isInLoopThread()) {
        abortNotInLoopThread();
    }
}

bool EventLoop::isInLoopThread() const {
    return ::pthread_equal(threadId_, ::pthread_self()) != 0;
}

void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }
    
    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        std::cerr << "EventLoop::wakeup() writes " << n << " bytes instead of 8" << std::endl;
    }
}

void EventLoop::updateChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    return poller_->hasChannel(channel);
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        std::cerr << "EventLoop::handleRead() reads " << n << " bytes instead of 8" << std::endl;
    }
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    
    for (const Functor& functor : functors) {
        functor();
    }
    
    callingPendingFunctors_ = false;
}

void EventLoop::runAfter(double delay, Functor cb) {
    Timestamp time(addTime(Timestamp::now(), delay));
    // 这里暂时简化实现，实际应该使用定时器类
    queueInLoop(std::move(cb));
}

void EventLoop::runEvery(double interval, Functor cb) {
    // 这里暂时简化实现，实际应该使用定时器类
    auto task = [this, interval, cb]() {
        cb();
        runEvery(interval, cb);
    };
    runAfter(interval, task);
}

void EventLoop::abortNotInLoopThread() {
    std::cerr << "EventLoop::abortNotInLoopThread() - EventLoop was created in threadId_ = " 
              << threadId_ << ", current thread id = " << ::pthread_self() << std::endl;
    abort();
}