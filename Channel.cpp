#include"Channel.h"
#include"Logger.h"
#include "EventLoop.h"

#include<sys/epoll.h>

const int Channel::KNoneEvent = 0;
const int Channel::KReadEvent = EPOLLIN|EPOLLPRI;
const int Channel::KWriteEvent = EPOLLOUT;

// EventLoop: ChannelList Poller
Channel::Channel(EventLoop* loop,int fd):loop_(loop),fd_(fd),events_(0),revents_(0),index_(-1),tied_(false){

}

Channel::~Channel(){
    
}

// channel的tie方法什么时候调用过  一个TcpConnection新连接创建的时候  TcpConnection =》 channel    
void Channel::tie(const std::shared_ptr<void>&obj){
    tie_ = obj;
    tied_ = true;
}

// 当改变channel所表示fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
void Channel::update(){
    // 通过channel所属的EventLoop，调用poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}

// 在channel所属的EventLoop中，把当前的channel删除掉
void Channel::remove(){
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime){
    if(tied_){      // 疑问？？？
        std::shared_ptr<void> guard = tie_.lock();
        if(guard){      // 当前的channel是否存活
            handleEventwithGuard(receiveTime);
        }
    }
    else{
        handleEventwithGuard(receiveTime);
    }
}

// 根据poller通知的channel发生的具体事件，由channel负责调用具体的回调操作
// Channel通知相应事件的时候会执行Tcpconnection注册的回调函数
// 用户给TcpServer的   =》 TcpConnection   =》Channel  注册到Poller中   Poller通知Channel  Channel执行相应的回调
void Channel::handleEventwithGuard(Timestamp receiveTime){
    LOG_INFO("channel handleEvent revents:%d\n",revents_);
    // EPOLLHUP表示对端已经关闭连接，通常和EPOLLIN一起使用，表示有数据可读但已经关闭连接
    if((revents_&EPOLLHUP)&&!(revents_&EPOLLIN)){        
        if(closeCallback_){             // 成员变量如果已经注册了回调执行回调  
            closeCallback_();       
        }
    }
    
    // 错误回调
    if(revents_&EPOLLERR){
        if(errorCallback_){         // 成员变量如果已经注册了回调执行回调   
            errorCallback_();
        }
    }

    // EPOLLPRI表示可读事件，但是有优先级设置，当设置优先级时会触发这个事件
    if(revents_&(EPOLLIN|EPOLLPRI)){
        if(readCallback_){              // 成员变量如果已经注册了回调执行回调  
            readCallback_(receiveTime);
        }
    }

    if(revents_&EPOLLOUT){
        if(writeCallback_){             // 成员变量如果已经注册了回调执行回调   
            writeCallback_();
        }
    }
}