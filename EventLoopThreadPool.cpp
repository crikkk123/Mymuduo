#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

#include <memory>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop,const std::string &nameArg):baseLoop_(baseLoop),name_(nameArg),
                    started_(false),numThreads_(0),next_(0){

}
EventLoopThreadPool::~EventLoopThreadPool(){
    // 底层创建的事件循环的线程是在栈上的，会循环到loop底层的poller中，不需要析构
}   

// using ThreadInitCallback = std::function<void(EventLoop*)>;
void EventLoopThreadPool::start(const ThreadInitCallback& cb){
    started_ = true;
    for(int i =0;i<numThreads_;++i){
        char buf[name_.size()+32];
        snprintf(buf,sizeof(buf),"%s%d",name_.c_str(),i);
        EventLoopThread* t = new EventLoopThread(cb,buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));        // std::vector<std::unique_ptr<EventLoopThread>> threads_; 
        // std::vector<EventLoop*> loops_;
        loops_.push_back(t->startloop());   // 底层创建线程，绑定一个新的EventLoop，并返回该loop的地址
    }

    // 整个服务端只有一个线程，运行着baseloop
    if(numThreads_==0&&cb){
        cb(baseLoop_);
    }
}

// 如果工作在多线程中，baseLoop默认以轮询的方式分配channel给subloop
EventLoop* EventLoopThreadPool::getNextLoop(){
    EventLoop* loop = baseLoop_;

    if(!loops_.empty()){    //通过轮询获取下一个处理事件的loop
        loop = loops_[next_];       // std::vector<EventLoop*> loops_;
        ++next_;
        if(next_>=loops_.size()){
            next_ = 0;
        }
    }

    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops(){
    if(loops_.empty()){
        return std::vector<EventLoop*>(1,baseLoop_);
    }
    else{
        return loops_;    // std::vector<EventLoop*> loops_;
    }
}

/*
    用户通过设置EventLoop，baseloop，如果没有设置那就工作线程和连接线程共用一个线程
    mainloop主要处理accept，将感兴趣的fd打包为channel，通过轮询机制发送给工作线程（Poller中），
    并将工作线程唤醒（eventfd）（wakeup）写一个字节的数据
    
*/