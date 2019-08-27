// PHZ
// 2018-5-15

#include "EpollTaskScheduler.h"

#if defined(__linux) || defined(__linux__) 
#include <sys/epoll.h>
#include <errno.h>
#endif

using namespace xop;

EpollTaskScheduler::EpollTaskScheduler(int id)
	: TaskScheduler(id)
{
#if defined(__linux) || defined(__linux__) 
    _epollfd = epoll_create1(0);
 #endif
    this->updateChannel(_wakeupChannel);
}

EpollTaskScheduler::~EpollTaskScheduler()
{
	
}

void EpollTaskScheduler::updateChannel(ChannelPtr channel)
{
	std::lock_guard<std::mutex> lock(_mutex);
#if defined(__linux) || defined(__linux__) 
    int fd = channel->fd();
    if(_channels.find(fd) != _channels.end())		
    {
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            _channels.erase(fd);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
    else
    {
        if(!channel->isNoneEvent())
        {
            _channels.emplace(fd, channel);
            update(EPOLL_CTL_ADD, channel);
        }	
    }	
#endif
}

void EpollTaskScheduler::update(int operation, ChannelPtr& channel)
{
#if defined(__linux) || defined(__linux__) 
    struct epoll_event event = {0};

    if(operation != EPOLL_CTL_DEL)
    {
        event.data.ptr = channel.get();
        event.events = channel->events();
    }

    if(::epoll_ctl(_epollfd, operation, channel->fd(), &event) < 0)
    {

    }
#endif
}

void EpollTaskScheduler::removeChannel(ChannelPtr& channel)
{
    std::lock_guard<std::mutex> lock(_mutex);
#if defined(__linux) || defined(__linux__) 
    int fd = channel->fd();

    if(_channels.find(fd) != _channels.end())	
    {
        update(EPOLL_CTL_DEL, channel);
        _channels.erase(fd);
    }
#endif
}

bool EpollTaskScheduler::handleEvent(int timeout)
{
#if defined(__linux) || defined(__linux__) 
    struct epoll_event events[512] = {0};
    int numEvents = -1;

    numEvents = epoll_wait(_epollfd, events, 512, timeout);
    if(numEvents < 0)  // 
    {
        if(errno != EINTR)
        {
            return false;
        }								
    }

    for(int n=0; n<numEvents; n++)
    {
        if(events[n].data.ptr)
        {        
            ((Channel *)events[n].data.ptr)->handleEvent(events[n].events);
        }
    }		
    return true;
#else
    return false;
#endif
}


