#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

static int createNonblocking(){
    int sockfd = ::socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);
    if(sockfd<0){
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n",__FILE__,__FUNCTION__,__LINE__,errno);
    }
    return sockfd;
}

// 不能将启动读事件监控放到构造函数中，必须在设置回调函数之后，再去启动
    // 否则有可能造成启动监控后，立即有事件到来了，但是处理的时候回调函数还没设置，所以新连接得不到处理，并且资源泄露
Acceptor::Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport):loop_(loop),
                    acceptSocket_(createNonblocking()),acceptChannel_(loop,acceptSocket_.fd()),listenning_(false){
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);  // bind
    // TcpServer::start() Acceptor.listen  有新用户的连接，要执行一个回调（connfd=》channel=》subloop）
    // baseloop=》acceptChannel_(listenfd)=>
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
}

Acceptor::~Acceptor(){
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen(){
    listenning_ = true; 
    acceptSocket_.listen();     // listen
    acceptChannel_.enableReading();    // acceptChannel_  => Poller
}

// Channel收到读事件的时候执行，listenfd有事件发生了，就是有新用户连接了
/* 
    构造函数里设置的主Reactor可读事件触发回调函数是handleRead函数，这个函数也是Acceptor类内成员函数，该函数首先是调用accept函数操作，
    获取新连接。因为TCP服务器的套接字操作步骤是，socket创建套接字、bind绑定地址信息、listen设置监听状态，最后accept获取连接。
    当有连接到来的时候，进程会从accept函数调用处返回，返回以后，handleRead函数再去调用外部设置的新连接到来触发的回调函数，
    即调用成员变量_acceptCallBack。
*/
void Acceptor::handleRead(){
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd>=0){
        if(newConnectionCallback_){
            newConnectionCallback_(connfd,peerAddr);    // 轮询找到subloop，唤醒分发当前的新客户端的Channel 由TcpServer模块设置的
        }
        else{
            ::close(connfd);
        }
    }
    else{
        LOG_ERROR("%s:%s:%d accept err:%d \n",__FILE__,__FUNCTION__,__LINE__,errno);
        if(errno==EMFILE){
            LOG_ERROR("%s:%s:%d sockfd reached limit \n",__FILE__,__FUNCTION__,__LINE__);
        }
    }
}