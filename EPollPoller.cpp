#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <strings.h>

const int kNew = -1;    // Channel未添加到Poller中      Channel的成员index_ = -1
const int kAdded = 1;   // Channel已经添加到Poller中
const int kDeleted = 2; // Channel从Poller中删除

// muduo库底层的epoll所使用的epoll_events的vector的长度是16
// events_(kInitEventListSize)   vector<epoll_event>
/** epoll_create1(EPOLL_CLOEXEC) 设置 close-on-exec 标志
 *  EPOLL_CLOEXEC 是一个文件描述符标志，当设置这个标志时，创建的 epoll 文件描述符在调用 exec 系列函数（如 execve、execl 等）时会自动关闭。
 *  这意味着子进程不会继承这个文件描述符。这是一个安全特性，避免文件描述符意外泄漏到子进程中。
*/   
EPollPoller::EPollPoller(EventLoop* loop):Poller(loop),epollfd_(::epoll_create1(EPOLL_CLOEXEC)),eventlist_(kInitEventListSize){
    if(epollfd_<0){
        LOG_FATAL("epoll_create error:%d \n",errno);
    }
}

EPollPoller::~EPollPoller(){
    ::close(epollfd_);      // 关闭epoll_create的文件描述符epollfd
}

// poll通过epoll_wait将发生的事件传入ChannelList中
Timestamp EPollPoller::poll(int timeoutMs,ChannelList* activeChannels){
    // 实际上应该用LOG_DEGUG输出日志更为合理
    LOG_INFO("func=%s => fd total count:%lu \n",__FUNCTION__,channelmaps_.size());

    int numEvents = ::epoll_wait(epollfd_,&*eventlist_.begin(), static_cast<int>(eventlist_.size()),timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if(numEvents>0){
        LOG_INFO("%d events happened \n",numEvents);
        fillActiveChannels(numEvents,activeChannels);
        if(numEvents == eventlist_.size()){
            eventlist_.resize(eventlist_.size()*2);
        }
    }
    else if(numEvents==0){
        LOG_DEBUG("%s timeout! \n",__FUNCTION__);
    }
    else{
        if(saveErrno!=EINTR){
            // 其他线程可能改变全局变量errno，所以打印日志之前赋值给他
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() error ");
        }
    }
    return now;
}

// channel update remove => EventLoop updateChannel removeChannel  => Poller updateChannel removeChannel
/**             一个EventLoop一个Poller,多个Channel
 *              EventLoop
 *    ChannelList         Poller
 *                        ChannelMap<fd,Channel*>
*/
void EPollPoller::updateChannel(Channel* channel){
    const int index = channel->index();
    LOG_INFO("func = %s => fd=%d events=%d index=%d \n",__FUNCTION__,channel->fd(),channel->events(),index);
    if(index==kNew||index==kDeleted){       // 没有被添加过或者删除了（总之Poller没有监听）
        int fd = channel->fd();
        if(index == kNew){
            // using ChannelMap = std::unordered_map<int, Channel*>;
            channelmaps_[fd] = channel;    // channelmaps_是EpollPoller从基类Poller继承来的一个ChannelMap
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD,channel);
    }
    else{   // channel已经在Poller上注册过了(Poller上正在监听)
        int fd = channel->fd();
        if(channel->isNoneEvent()){      // 如果监听的fd什么事件都不感兴趣直接删除
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);
        }
        else{
            update(EPOLL_CTL_MOD,channel);
        }
    }
}

// 从Poller中删除channel
void EPollPoller::removeChannel(Channel* channel){
    int fd = channel->fd();
    // using ChannelMap = std::unordered_map<int, Channel*>;
    channelmaps_.erase(fd);
    LOG_INFO("func = %s => fd=%d \n",__FUNCTION__,fd);
    int index = channel->index();
    if(index==kAdded){      // 如果poller在监听再利用epoll_ctl将其设置为EPOLL_CTL_DEL
        update(EPOLL_CTL_DEL,channel);
    }
    channel->set_index(kNew);   // 设置为没有添加过（移除了）
}


/**
    typedef union epoll_data
    {
    void *ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
    } epoll_data_t;

    struct epoll_event
    {
    uint32_t events;
    epoll_data_t data;
    } __EPOLL_PACKED;
 */
// 填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents,ChannelList* activeChannels) const{
    for(int i =0;i<numEvents;i++){  
        Channel* channel = static_cast<Channel*>(eventlist_[i].data.ptr);   // eventlist_[i].data.ptr  epoll_event（中的指针）
        channel->set_revents(eventlist_[i].events);     // 设置感兴趣的事件
        activeChannels->push_back(channel);     // EventLoop就拿到了它的poller给它返回的所有发生事件的Channel列表了
    }
}

// 更新Channel通道 epoll_ctl add/mod/del
void EPollPoller::update(int operation,Channel* channel){
    epoll_event event;
    bzero(&event,sizeof(event));
    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if(::epoll_ctl(epollfd_,operation,fd,&event)<0){
        if(operation == EPOLL_CTL_DEL){
            LOG_ERROR("epoll_ctl del error:%d\n",errno);
        }
        else{
            LOG_FATAL("epoll_ctl add/mod error:%d\n",errno);
        }
    }
}
