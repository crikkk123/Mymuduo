#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

// 防止一个线程创建多个EventLoop        
//  __thread  thread_local    不加__thread  后面的变量是全局的，所有线程共享的，一个线程就有一个EventLoop，
// 每一个线程访问的时候在每一个线程中都有其副本，每一个线程中只有一个EventLoop，不能有多个EventLoop
thread_local EventLoop* t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000;

// 创建wakeupfd，用来notify唤醒subReactor处理新来的Channel
int createEventfd(){
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd<0){
        LOG_FATAL("eventfd error:%d \n",errno);
    }
    return evtfd;
}

EventLoop::EventLoop():looping_(false),quit_(false),callingPendingFunctors_(false),threadId_(CurrentThread::tid()),
                        poller_(Poller::newDefaultPoller(this)),wakeupFd_(createEventfd()),
                        wakeupChannel_(new Channel(this,wakeupFd_)){
    LOG_DEBUG("EventLoop create %p in thread %d \n",this,threadId_);
    if(t_loopInThisThread){
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n",t_loopInThisThread,threadId_);
    }                 
    else{
        t_loopInThisThread = this;
    }

    // 设置wakeup的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
    // 每一个eventloop都将监听wakeupchannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();
}  

EventLoop::~EventLoop(){
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread=nullptr;
}

void EventLoop::handleRead(){
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_,&one,sizeof(one));
    if(n!=sizeof(one)){
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8 \n",n);
    }
}

void EventLoop::loop(){
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n",this);

    while(!quit_){
        activeChannels_.clear();
        // 监听两类fd  一种是client的fd   一中wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs,&activeChannels_);
        for(Channel* channel:activeChannels_){
            // Poller监听了哪些Channel发生事件了，然后上报给Eventloop，通知Channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前Eventloop事件循环需要处理的回调操作
        /**
         * IO线程  mainloop  accept  channel => fd  subloop
         * mainloop 事先注册一个回调cb，需要subloop来执行   wakeup subloop后，执行下面的方法，执行之前mainloop注册的cb操作
        */
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping \n",this);
    looping_ = false;
}

// 退出事件循环    1.loop在自己的线程中调用quit    2.在非loop的线程中，调用loop的quit
/**
 *                  mainloop
 * 
 *                                                   no ========================  生产者-消费者的线程安全的队列
 * 
 * subloop1         subloop2       subloop3
 * 
 * 在subloop1中退出subloop2，得先唤醒subloop2，让其不阻塞在poll中，通过while(!quit_)判断，转一圈就回去了
*/
void EventLoop::quit(){
    quit_ = true;

    if(!isInLoopThread()){  // 如果是在其他线程中，调用quit  在一个subloop（workthread）中，调用了mainloop（IO）的quit
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb){
    if(isInLoopThread()){   // 在当前的loop线程中，执行cb
        cb();
    }
    else{      // 在非当前loop线程中执行cb，就需要唤醒loop所在线程，执行cb
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb){
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // pendingFunctors_存储的是loop需要执行的回调操作
        pendingFunctors_.emplace_back(cb);      // 这个的意思就是在pendingFunctors_这个数组中构造一个cb回调对象存储起来
    }

    // 唤醒相应的，需要执行上面回调操作的loop的线程了
    // || callingPendingFunctors_的意思是：当前loop正在执行回调，但是loop又有了新的回调
    if(!isInLoopThread() || callingPendingFunctors_){
        wakeup();
    }
}

// 用来唤醒loop所在的线程的，向wakeupfd_写一个数据,wakeupchannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup(){
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_,&one,sizeof(one));
    if(n!=sizeof(one)){
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n",n);
    }
}

void EventLoop::updateChannel(Channel*channel){
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel){
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel){
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors(){
    std::vector<Functor> functors;      // using Functor = std::function<void()>;
    callingPendingFunctors_ = true;     // 标识当前的loop是否有需要执行的回调操作

    {
        std::unique_lock<std::mutex> lock(mutex_);  // C++11的用法，独占式锁，除了这个大括号就自动释放锁了
        functors.swap(pendingFunctors_);        // pendingFunctors_存储的是loop需要执行的回调操作
    }

    for(const Functor& functor:functors){
        functor();      // 执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}
