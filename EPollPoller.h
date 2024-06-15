#pragma once 

#include "Poller.h"
#include "Timestamp.h"

#include<vector>
#include<sys/epoll.h>

class Channel;

/*
    epoll的使用
    epoll_create    EPollPoller(EventLoop* loop);    
    epoll_ctl    add/mod/del    Timestamp poll(int timeoutsMs,ChannelList* activeChannels) override;   updateChannel  removeChannel
    epoll_wait 
*/

// 继承的时候需要知道基本的详细情况，需要包含头文件，不能是前置类型声明
class EPollPoller:public Poller{
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    // 重写基类的Poller的抽象方法
    Timestamp poll(int timeoutMs,ChannelList* activeChannels) override;    // epoll_wait
    void updateChannel(Channel* channel) override;      // channel调用的update的底层
    void removeChannel(Channel* channel) override;      // channel调用的remove的底层
private:
    static const int kInitEventListSize = 16;       // 底层存放epoll_event的vector大小为16，双倍扩容

    // 填写活跃的连接   using ChannelList = std::vector<Channel*>;
    void fillActiveChannels(int numEvents,ChannelList* activeChannels) const;
    // 更新Channel通道
    void update(int operation,Channel* channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList eventlist_;
};
