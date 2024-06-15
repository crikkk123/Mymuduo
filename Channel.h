#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;    // 类型的前置声明        头文件中只用到了类型的声明，没有用到类型的具体定义以及方法
                    // 在源文件中include，如果用没用到就include文件，在最后需要向用户提供.h头文件，如果这个文件包含其他的，都需要提供
// class Timestamp;    // handleEvent中是用定义Timestamp类型的变量，大小不确定，不能前置声明   EventLoop是指针类型大小为4或8


/*   
    理清楚：EventLoop、channel、Poller这间的关系   =》  Reactor模型上的Demultiplex
    channel 理解为通道，封装了sockfd和其感兴趣的event，如EPOLLIN、EPOLLOUT事件
    还绑定了poller返回的具体事件
*/

// EventLoop包含了channel和poller
// channel包含fd，以及感兴趣的事件,以及最终发生的事件
// 一个EventLoop包含很多个channel，一个channel属于一个EventLoop
// 一个poller监听很多个channel
class Channel:noncopyable{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;      // 读回调需要Timestamp

    // 属于哪一个loop  以及打包的fd
    Channel(EventLoop* loop,int fd);
    ~Channel();

    // fd得到poller通知以后，处理事件的
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb){ readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) {writeCallback_ = std::move(cb);}
    void setCloseCallback(EventCallback cb){closeCallback_ = std::move(cb);}
    void setErrorCallback(EventCallback cb){errorCallback_ = std::move(cb);}

    // 防止当Channel被手动remove掉，channel还在执行回调操作
    // TcpConnection => Channel 
    void tie(const std::shared_ptr<void>& obj);     // 疑问？？？

    int fd() const {return fd_;}
    int events() const {return events_;} 
    void set_revents(int revt) {revents_ = revt;}

    // 设置fd相应的事件状态
    void enableReading() {events_ |= KReadEvent;update();}
    void disableReading(){events_ &= ~KReadEvent;update();}
    void enableWriting() {events_ |= KWriteEvent;update();}
    void disableWriting(){events_ &= ~KWriteEvent;update();}
    void disableAll() {events_ = KNoneEvent;update();}

    // 返回fd当前的事件状态
    bool isNoneEvent() const {return events_== KNoneEvent;}
    bool isReading() const {return events_ & KReadEvent;}
    bool isWriting() const {return events_&KWriteEvent;}

    int index() {return index_;}        // 疑问
    void set_index(int idx){index_=idx;}

    EventLoop* ownerLoop(){return loop_;}   // 属于哪个loop
    void remove();      // 对某个fd事件不感兴趣了，把他从EPollPoller中移除

private:
    // 表示当前fd对什么感兴趣，什么都不感兴趣，只对读感兴趣， 只对写感兴趣
    static const int KNoneEvent;    // 0
    static const int KReadEvent;    // EPOLLIN
    static const int KWriteEvent;   // EPOLLOUT

    EventLoop* loop_;       // 事件循环  哪一个loop
    const int fd_;          // fd，打包的fd  poller监听的对象   
    int events_;            // 注册fd感兴趣的事件   (读事件，写事件)
    int revents_;            // poller返回的具体发生的事件   最终通知的fd上发生了什么事件
    int index_;         // 初始化为-1   kNew为-1，表示Channel未添加到Poller中

    void update();  // 底层EventLoop调用Poller的update  Poller是一个基类，提供的是纯虚函数让EPollPoller子类实现，muduo库还有Poll（没实现）
    void handleEventwithGuard(Timestamp receiveTime);   

    // 疑问？？？
    std::weak_ptr<void> tie_;   // 防止手动调用removeChannel后，还再使用Channel，所以进行了一个跨线程的对象的生存状态的监听
    bool tied_;     

    // 因为Channel通道里面能够获知fd最终发生的具体事件revents，所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
