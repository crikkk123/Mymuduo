#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

std::atomic_int Thread::numCreated_(0);      // 静态初始化为0

Thread::Thread(ThreadFunc func,const std::string &name):started_(false),joined_(false),tid_(0),func_(std::move(func)),name_(name){
    setDefaultName();
}

Thread::~Thread(){
    // 如果线程是运行的或者没有joined的时候调用detach
    if(started_ && !joined_){
        thread_->detach();      // thread类提供的设置分离线程的方法
    }
}

// 一个Thread对象，记录的就是一个新线程的详细信息
void Thread::start(){       
    started_ = true;
    sem_t sem;
    sem_init(&sem,false,0);
    // 开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        tid_ = CurrentThread::tid();    // 获取线程的tid值
        sem_post(&sem);
        func_();    // 开启一个新线程，专门执行该线程函数
    }));

    // 这里必须等待获取上面创建的线程的tid值
    sem_wait(&sem);
}

void Thread::join(){
    joined_ = true;
    thread_->join();        // 调用detach
}

void Thread::setDefaultName(){
    int num = ++numCreated_;     // numCreated_在多线程访问需要用atomic_int
    if(name_.empty()){
        char buf[32] = {0};
        snprintf(buf,sizeof(buf),"Thread %d \n",num);
        name_ = buf;        // 成员变量，线程的名字
    }
}