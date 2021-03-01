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

	Channel(SOCKET sockfd) 
		: sockfd_(sockfd)
	{

	}

	virtual ~Channel() {};
    
	void SetReadCallback(const EventCallback& cb)
	{ read_callback_ = cb; }

	void SetWriteCallback(const EventCallback& cb)
	{ write_callback_ = cb; }

	void SetCloseCallback(const EventCallback& cb)
	{ close_callback_ = cb; }

	void SetErrorCallback(const EventCallback& cb)
	{ error_callback_ = cb; }

	SOCKET GetSocket() const { return sockfd_; }

	int  GetEvents() const { return events_; }
	void SetEvents(int events) { events_ = events; }
    
	void EnableReading() 
	{ events_ |= EVENT_IN; }

	void EnableWriting() 
	{ events_ |= EVENT_OUT; }
    
	void DisableReading() 
	{ events_ &= ~EVENT_IN; }
    
	void DisableWriting() 
	{ events_ &= ~EVENT_OUT; }
       
	bool IsNoneEvent() const { return events_ == EVENT_NONE; }
	bool IsWriting() const { return (events_ & EVENT_OUT) != 0; }
	bool IsReading() const { return (events_ & EVENT_IN) != 0; }
    
	void HandleEvent(int events)
	{	
		if (events & (EVENT_PRI | EVENT_IN)) {
			read_callback_();
		}

		if (events & EVENT_OUT) {
			write_callback_();
		}
        
		if (events & EVENT_HUP) {
			close_callback_();
			return ;
		}

		if (events & (EVENT_ERR)) {
			error_callback_();
		}
	}

private:
	EventCallback read_callback_  = []{};
	EventCallback write_callback_ = []{};
	EventCallback close_callback_ = []{};
	EventCallback error_callback_ = []{};
    
	SOCKET sockfd_ = 0;
	int events_ = 0;    
};

typedef std::shared_ptr<Channel> ChannelPtr;

}

#endif  

