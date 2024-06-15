#pragma once 

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

/**
 * TcpServer => Acceptor => 有一个新用户连接，通过accept函数拿到connfd
 * 
 * =》 TcpConnection 设置回调 =》Channel =》Poller =》 Channel的回调
 * 
 */

class TcpConnection:noncopyable,public std::enable_shared_from_this<TcpConnection>{
public:
    TcpConnection(EventLoop* loop,const std::string &name,int sockfd,const InetAddress& localAddr,const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const {return loop_;}
    const std::string& name() const {return name_;}
    const InetAddress& localAddress() const {return localAddr_;}
    const InetAddress& peerAddress() const {return peerAddr_;}

    bool connected() const {return state_ == kConnected;}

    // 发送数据
    void send(const std::string& buf);

    // 关闭连接
    void shutdown();

    void setConnectionCallback(const ConnectionCallback& cb){connectionCallback_ = cb;}
    void setMessageCallback(const MessageCallback& cb){messageCallback_ = cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback& cb){writeCompleteCallback_ = cb;}
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb,size_t highWaterMark){highWaterMarkCallback_ = cb;highWaterMark_ = highWaterMark;}

    void setCloseCallback(const CloseCallback& cb){closeCallback_ = cb;}

    // 建立连接
    void connectEstablished();
    // 连接销毁
    void connectDestroyed();

private:    
    enum StateE{
        kDisconnected,          // 断开连接
        kConnecting,            // 正在建立连接
        kConnected,             // 建立连接
        kDisconnecting          // 正在断开连接
    };

    void setState(StateE state){ state_ = state; };

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();
   
    void sendInLoop(const void* message,size_t len);
    void shutdownInLoop();
    
    EventLoop* loop_;   // 这里绝对不是baseloop，因为TcpConnection都是在subloop里面管理的
    const std::string name_;        // 名字
    std::atomic_int state_;         // 状态
    bool reading_;                  // 

    // 这里和Acceptor类似， Acceptor=》mainloop    TcpConnection=>subloop
    std::unique_ptr<Socket> socket_;        // 智能指针管理socket
    std::unique_ptr<Channel> channel_;      // 智能指针管理Channel

    const InetAddress localAddr_;           // 本机地址
    const InetAddress peerAddr_;            // 对端地址

    ConnectionCallback connectionCallback_;     // 有新连接时的回调  function<void(const TcpConnectionPtr&)>;
    MessageCallback messageCallback_;           // 有读写消息时的回调  function<void(const TcpConnectionPtr&,Buffer*,Timestamp)>;
    WriteCompleteCallback writeCompleteCallback_;   // 消息发送完成以后得回调  function<void(const TcpConnectionPtr&)>;
    CloseCallback closeCallback_;    // 断开连接的时候的回调    function<void(const TcpConnectionPtr&)>;
    HighWaterMarkCallback highWaterMarkCallback_;   // 高水位回调   function<void(const TcpConnectionPtr&,size_t)>;
    size_t highWaterMark_;      // 害怕我们发送的过来，对方接收的太慢

    Buffer inputBuffer_;        // 接收数据的缓冲区
    Buffer outputBuffer_;       // 发送数据的缓冲区
};
