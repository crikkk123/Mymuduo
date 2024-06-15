#pragma once

#include "noncopyable.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

// 对外的服务器编程使用的类
class TcpServer:noncopyable{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option{
        kNoReusePort,
        KReusePort,
    };

    TcpServer(EventLoop* loop,const InetAddress& listenAddr,const std::string& nameArg,Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitcallback(const ThreadInitCallback& cb){threadInitCallback_ = cb;}
    void setConnectionCallback(const ConnectionCallback& cb){connectionCallback_ = cb;}
    void setMessageCallback(const MessageCallback& cb){messageCallback_ = cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback& cb){writeCompleteCallback_ = cb;}

    void setThreadNum(int numThreads);      // 设置底层subloop的个数

    void start();   // 开启服务器监听  mainloop accept中的listen
private:
    void newConnection(int sockfd,const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::unordered_map<std::string,TcpConnectionPtr>;

    EventLoop* loop_;   // baseloop    用户定义的loop

    const std::string ipPort_;      // ip+端口
    const std::string name_;        // 名字

    std::unique_ptr<Acceptor> acceptor_;    // 运行在mainLoop,任务就是监听新连接事件

    std::shared_ptr<EventLoopThreadPool> threadPool_;  // one loop  per thread

    ConnectionCallback connectionCallback_;     // 有新连接时的回调   std::function<void(const TcpConnectionPtr&)>;
    MessageCallback messageCallback_;           // 有读写消息时的回调  std::function<void(const TcpConnectionPtr&,Buffer*,Timestamp)>;
    WriteCompleteCallback writeCompleteCallback_;   // 消息发送完成以后得回调  std::function<void(const TcpConnectionPtr&)>;

    ThreadInitCallback threadInitCallback_;     //loop线程初始化的回调   std::function<void(EventLoop*)>;

    std::atomic_int started_;           // 

    int nextConnId_;            // 轮询查找subloop
    ConnectionMap connections_;     // std::unordered_map<std::string,TcpConnectionPtr>;
};
