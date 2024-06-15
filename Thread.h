#pragma once

#include "noncopyable.h"

#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>

class Thread : noncopyable{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc,const std::string &name = std::string());
    ~Thread();

    void start();       // 启动线程
    void join();        // 回收线程

    bool started() const {return started_;}
    pid_t tid() const {return tid_;}
    const std::string &name () const {return name_;}

    static int numCreated() {return numCreated_;}
private:
    bool started_;      // 标记线程是否是执行状态
    bool joined_;       // 标记线程是否是joined状态
    void setDefaultName();   // 设置线程默认的名字
    std::shared_ptr<std::thread> thread_;       // 使用智能指针管理C++11的thread
    pid_t tid_;     // id号
    ThreadFunc func_;   
    std::string name_;
    static std::atomic_int numCreated_;     // 在线程没有名字的时候使用
};