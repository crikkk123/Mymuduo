#include "InetAddress.h"

#include<string.h>

InetAddress::InetAddress(uint16_t port,std::string ip){
    bzero(&addr_,sizeof(addr_));    // 将addr_成员变量清空
    addr_.sin_family = AF_INET;     // ipv4
    addr_.sin_port = htons(port);   // 本地字节序转网络字节序
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());     // 绑定ip
}

std::string InetAddress::toIp() const{
    char buf[64] = {0};
    // 将二进制的网络字节序转换为人类可读的十进制的格式
    ::inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof(buf));
    return buf;
}

std::string InetAddress::toIpPort() const{
    char buf[64] = {0};
    ::inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof(buf));
    size_t end = strlen(buf);
    // 网络字节序转本地字节序（端口）
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf+end,":%u",port);
    return buf;
}

uint16_t InetAddress::toPort() const{
    return ntohs(addr_.sin_port);
}


// #include<iostream>
// int main(){
//     InetAddress addr(8000);
//     std::cout<<addr.toIpPort()<<std::endl;
//     return 0;
// }
