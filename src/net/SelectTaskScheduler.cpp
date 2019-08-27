// PHZ
// 2018-5-15

#include "SelectTaskScheduler.h"
#include "Logger.h"
#include "Timer.h"
#include <cstring>
#include <forward_list>

using namespace xop;

#define SELECT_CTL_ADD	0
#define SELECT_CTL_MOD  1
#define SELECT_CTL_DEL	2

SelectTaskScheduler::SelectTaskScheduler(int id)
	: TaskScheduler(id)
{
    FD_ZERO(&_fdReadBackup);
    FD_ZERO(&_fdWriteBackup);
    FD_ZERO(&_fdExpBackup);

    this->updateChannel(_wakeupChannel);
}

SelectTaskScheduler::~SelectTaskScheduler()
{
	
}

void SelectTaskScheduler::updateChannel(ChannelPtr channel)
{
    std::lock_guard<std::mutex> lock(_mutex);

    SOCKET fd = channel->fd();

    if(_channels.find(fd) != _channels.end())		// 
    {
        if(channel->isNoneEvent())
        {
            _isfdReadReset = true;
            _isfdWriteReset = true;
            _isfdExpReset = true;
            _channels.erase(fd);
        }
        else
        {
            //_isfdReadReset = true;
            _isfdWriteReset = true;
        }
    }
    else
    {
        if(!channel->isNoneEvent())
        {
            _channels.emplace(fd, channel);
            _isfdReadReset = true;
            _isfdWriteReset = true;
            _isfdExpReset = true;
        }	
    }	
}

void SelectTaskScheduler::removeChannel(ChannelPtr& channel)
{
    std::lock_guard<std::mutex> lock(_mutex);

    SOCKET fd = channel->fd();

    if(_channels.find(fd) != _channels.end())	
    {
        _isfdReadReset = true;
        _isfdWriteReset = true;
        _isfdExpReset = true;
        _channels.erase(fd);
    }
}

bool SelectTaskScheduler::handleEvent(int timeout)
{	
    if(_channels.empty())
    {
        if(timeout <= 0)
            timeout = 100;
        Timer::sleep(timeout);
        return true;
    }

    fd_set fdRead;
    fd_set fdWrite;
    fd_set fdExp;
    FD_ZERO(&fdRead);
    FD_ZERO(&fdWrite);
    FD_ZERO(&fdExp);
    bool fdReadReset = false, fdWriteReset = false, fdExpReset = false;
    if(_isfdReadReset || _isfdWriteReset || _isfdExpReset)
    {
        if(_isfdExpReset)
            _maxfd = 0;
        std::lock_guard<std::mutex> lock(_mutex);
        for(auto iter : _channels)
        {
            int events = iter.second->events();
			SOCKET fd = iter.second->fd();

            if (_isfdReadReset && (events&EVENT_IN))
            {
                FD_SET(fd, &fdRead);
            }

            if(_isfdWriteReset && (events&EVENT_OUT))
            {
                FD_SET(fd, &fdWrite);
            }

            if(_isfdExpReset)
            {
                FD_SET(fd, &fdExp);
                if(fd > _maxfd)
                {
                    _maxfd = fd;
                }
            }		
        }
        
        fdReadReset = _isfdReadReset;
        fdWriteReset = _isfdWriteReset;
        fdExpReset = _isfdExpReset;
        _isfdReadReset = false;
        _isfdWriteReset = false;
        _isfdExpReset = false;
    }
	
    if(fdReadReset)
    {
        FD_ZERO(&_fdReadBackup);
        memcpy(&_fdReadBackup, &fdRead, sizeof(fd_set)); //备份
    }
    else
        memcpy(&fdRead, &_fdReadBackup, sizeof(fd_set));

    if(fdWriteReset)
    {
        FD_ZERO(&_fdWriteBackup);
        memcpy(&_fdWriteBackup, &fdWrite, sizeof(fd_set));
    }
    else
        memcpy(&fdWrite, &_fdWriteBackup, sizeof(fd_set));

    if(fdExpReset)
    {
        FD_ZERO(&_fdExpBackup);
        memcpy(&_fdExpBackup, &fdExp, sizeof(fd_set));
    }
    else
    {
        memcpy(&fdExp, &_fdExpBackup, sizeof(fd_set));
    }

    if(timeout < 0)
    {
        timeout = 100;
    }

    struct timeval tv = { timeout/1000, timeout%1000*1000 };
    int ret = select((int)_maxfd+1, &fdRead, &fdWrite, &fdExp, &tv); 	
    if (ret < 0)
    {
#if defined(__linux) || defined(__linux__) 
    if(errno == EINTR)
    {
        return true;
    }					
#endif 
        return false;
    }

    std::forward_list<std::pair<ChannelPtr, int>> eventList;
    if(ret > 0)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for(auto iter : _channels)
        {
            int events = 0;
            {
                if (FD_ISSET(iter.second->fd(), &fdRead))
                {
                    events |= EVENT_IN;
                }

                if (FD_ISSET(iter.second->fd(), &fdWrite))
                {
                    events |= EVENT_OUT;
                }

                if (FD_ISSET(iter.second->fd(), &fdExp))
                {
                    events |= (EVENT_HUP); // close
                }
            }
            
            if(events != 0)
            {
                eventList.emplace_front(iter.second, events);
            }
        }
    }	

    for(auto& iter: eventList)
    {
        iter.first->handleEvent(iter.second);
    }

    return true;
}



