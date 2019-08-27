// PHZ
// 2018-5-15
    
#ifndef XOP_CHANNEL_H
#define XOP_CHANNEL_H

#include <functional>
#include <memory>
#include "Socket.h"

namespace xop
{
    
enum EventType
{
    EVENT_NONE   = 0,
    EVENT_IN     = 1,
    EVENT_PRI    = 2,		
    EVENT_OUT    = 4,
    EVENT_ERR    = 8,
    EVENT_HUP    = 16,
    EVENT_RDHUP  = 8192
};

class Channel 
{
public:
    typedef std::function<void()> EventCallback;
    
    Channel() = delete;
    Channel(SOCKET fd) : _fd(fd) {};
    ~Channel() {};
    
    void setReadCallback(const EventCallback& cb)
    { _readCallback = cb; }
    void setWriteCallback(const EventCallback& cb)
    { _writeCallback = cb; }
    void setCloseCallback(const EventCallback& cb)
    { _closeCallback = cb; }
    void setErrorCallback(const EventCallback& cb)
    { _errorCallback = cb; }

    void setReadCallback(EventCallback&& cb)
    { _readCallback = std::move(cb); }
    void setWriteCallback(EventCallback&& cb)
    { _writeCallback = std::move(cb); }
    void setCloseCallback(EventCallback&& cb)
    { _closeCallback = std::move(cb); }
    void setErrorCallback(EventCallback&& cb)
    { _errorCallback = std::move(cb); } 

	SOCKET fd() const { return _fd; }
    int events() const { return _events; }
    void setEvents(int events) { _events = events; }
    
    void enableReading() 
    { _events |= EVENT_IN; }

    void enableWriting() 
    { _events |= EVENT_OUT; }
    
    void disableReading() 
    { _events &= ~EVENT_IN; }
    
    void disableWriting() 
    { _events &= ~EVENT_OUT; }
       
    bool isNoneEvent() const { return _events == EVENT_NONE; }
    bool isWriting() const { return (_events & EVENT_OUT)!=0; }
    bool isReading() const { return (_events & EVENT_IN)!=0; }
    
    void handleEvent(int events)
    {	
        if (events & (EVENT_PRI | EVENT_IN))
        {
            _readCallback();
        }

        if (events & EVENT_OUT)
        {
            _writeCallback();
        }
        
        if (events & EVENT_HUP)
        {
            _closeCallback();
            return ;
        }

        if (events & (EVENT_ERR))
        {
            _errorCallback();	
        }
    }

private:
    EventCallback _readCallback  = []{};
    EventCallback _writeCallback = []{};
    EventCallback _closeCallback = []{};
    EventCallback _errorCallback = []{};
    
	SOCKET _fd = 0;
    int _events = 0;    
};

typedef std::shared_ptr<Channel> ChannelPtr;

}

#endif  

