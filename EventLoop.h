#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

class Channel;
class Poller;

// 时间循环类  主要包含两个大模块 Channel  Poller（epoll的抽象）
class EventLoop:noncopyable{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();      // 开启时间循环
    void quit();      // 退出时间循环

    Timestamp pollReturnTime() const {return pollReturnTime_;}

    void runInLoop(Functor cb);     // 在当前loop中执行回调 （当前线程和执行回调的线程是一个线程）
    void queueInLoop(Functor cb);   // 把cb放入队列中，唤醒loop所在的线程，执行cb （当前线程和执行回调的线程不是一个线程）

    void wakeup();      // 唤醒loop所在线程的

    // EventLoop的方法  =》 Poller的方法
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    // 创建loop的线程id与当前线程的Id进行比较，如果相等，证明Eventloop这个对象在它所创建它的loop线程中，可以正常回调
    // 如果没在创建它的线程loop中，就得queueInLoop，当唤醒到它自己线程的时候，才去执行loop相关的回调操作
    bool isInLoopThread() const {return threadId_ == CurrentThread::tid();}    // 判断EventLoop对象是否在自己的线程里面    
private:
    using ChannelList = std::vector<Channel*>;

    void handleRead();      // 处理wakeup   往wakeupFd_里写一个字节的数据唤醒subloop
    void doPendingFunctors();   // 执行回调

    std::atomic_bool looping_;  // 原子操作，通过CAS实现的
    std::atomic_bool quit_;     // 标志退出loop循环

    const pid_t threadId_;      // 记录当前loop所在线程的id

    Timestamp pollReturnTime_;  // poller返回channel发生事件的时间点
    std::unique_ptr<Poller> poller_;    // EventLoop所管理的Poller   使用智能指针管理Poller

    int wakeupFd_; // 通过eventfd()函数创建唤醒工作线程，当mainloop获取一个新用户的Channel，通过轮询算法选择一个subloop，通过该成员唤醒subloop，处理Channel
    std::unique_ptr<Channel> wakeupChannel_;    // EventLoop所管理的Channel   使用智能指针管理Channel

    // using ChannelList = std::vector<Channel*>;
    ChannelList activeChannels_;        // eventloop管理的所有Channel

    std::atomic_bool callingPendingFunctors_;  // 标识当前的loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_;      // 存储loop需要执行的所有回调操作
    std::mutex mutex_;          // 互斥锁，用来保护上面的vector容器的线程安全操作
};
