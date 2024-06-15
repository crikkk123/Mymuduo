#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <strings.h>

Socket::~Socket(){
    close(sockfd_);
}

void Socket::bindAddress(const InetAddress& localaddr){
    if(::bind(sockfd_,(sockaddr*)localaddr.getSockAddr(),sizeof(sockaddr_in))!=0){
        LOG_FATAL("bind sockfd:%d fail \n",sockfd_);
    }
}

void Socket::listen(){
    if(0!=::listen(sockfd_,1024)){
        LOG_FATAL("listen sockfd:%d fail \n",sockfd_);
    }
}

int Socket::accept(InetAddress* peeraddr){
    /**
     * 1.accept函数的参数不合法
     * 2.对返回的connfd没有设置非阻塞
     * Reactor模型，one loop per thread
     * Poller + non-blocking IO
     */
    sockaddr_in addr;
    socklen_t len = sizeof(len);
    bzero(&addr,sizeof(addr));
    int connfd = ::accept4(sockfd_,(sockaddr*)&addr,&len,SOCK_NONBLOCK|SOCK_CLOEXEC);
    if(connfd>=0){
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownwrite(){
    if(::shutdown(sockfd_,SHUT_WR)<0){
        LOG_ERROR("shundownwrite error\n");
    }
}

void Socket::setTcpNoDelay(bool on){        // 允许小数据块立即发送，提高实时性
    int optval = on?1:0;
    ::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof(optval));
}

void Socket::setReuseAddr(bool on){         // 允许在绑定之前关闭处于TIME_WAIT状态的套接字，从而允许新的套接字绑定到相同的地址和端口
    int optval = on?1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
}

void Socket::setReusePort(bool on){         // 允许多个套接字绑定到相同的地址和端口
    int optval = on?1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof(optval));
}

void Socket::setKeepAlive(bool on){         // 启用或禁用 TCP 的 keep-alive 检测机制
    int optval = on?1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof(optval));
}
