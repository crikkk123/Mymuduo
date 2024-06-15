#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop* loop):ownerLoop_(loop){

}

bool Poller::hasChannel(Channel* channel) const{
    auto it = channelmaps_.find(channel->fd());
    return it!=channelmaps_.end() && it->second == channel;        // 疑问？？？
}
