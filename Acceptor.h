#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

class Acceptor:noncopyable{
public:
    using NewConnectionCallback = std::function<void(int sockfd,const InetAddress&)>;
    Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &cb){    // 设置回调函数NewConnectionCallback
        newConnectionCallback_ = cb;
    }

    bool listenning() const {return listenning_;}    // 返回成员变量listenning
    void listen();      // 底层调用socket里的listen函数
private:
    void handleRead();      // 注册的回调函数

    EventLoop* loop_;   // Acceptor用的就是用户定义的那个baseloop也称作mainloop  // 用于对监听套接字进行事件监控
    Socket acceptSocket_;   // 用于创建监听套接字
    Channel acceptChannel_; // 用于对监听套接字进行事件管理

    NewConnectionCallback newConnectionCallback_;

    bool listenning_;   // 是否是监听状态
};
