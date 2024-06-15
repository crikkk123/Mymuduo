#pragma once

#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    // 构造函数设置线程的数量是0，如果没有设置线程的数量baseloop既是主线程也是subloop
    EventLoopThreadPool(EventLoop* baseLoop,const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads){numThreads_ = numThreads;}

    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    // 如果工作在多线程中，baseLoop默认以轮询的方式分配channel给subloop
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();

    bool started() const {return started_;}

    const std::string name() const {return name_;}
private:

    EventLoop* baseLoop_;      // EventLoop loop;
    std::string name_;         // 名字
    bool started_;          // 是否是运行的
    int numThreads_;        // 设置几个线程
    int next_;              // l轮询时在vector数组中查找，构造中初始化为0
    std::vector<std::unique_ptr<EventLoopThread>> threads_;      // 包含所有创建的线程
    std::vector<EventLoop*> loops_;      // 包含事件线程里面的EventLoop指针
};
