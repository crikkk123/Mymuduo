#pragma once

#include "noncopyable.h"
#include "Thread.h"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

class EventLoop;

class EventLoopThread :noncopyable{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),const std::string &name = std::string());
    ~EventLoopThread();

    EventLoop* startloop();
private:
    void threadFunc();

    EventLoop* loop_;       // 记录当前的loop
    bool exiting_;          // 标记是否退出
    Thread thread_;         // 线程
    std::mutex mutex_;      // 锁
    std::condition_variable cond_;      // 条件变量
    ThreadInitCallback callback_;       // using ThreadInitCallback = std::function<void(EventLoop*)>;
};
