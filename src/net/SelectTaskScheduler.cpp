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
	FD_ZERO(&fd_read_backup_);
	FD_ZERO(&fd_write_backup_);
	FD_ZERO(&fd_exp_backup_);

	this->UpdateChannel(wakeup_channel_);
}

SelectTaskScheduler::~SelectTaskScheduler()
{
	
}

void SelectTaskScheduler::UpdateChannel(ChannelPtr channel)
{
	std::lock_guard<std::mutex> lock(mutex_);

	SOCKET socket = channel->GetSocket();

	if(channels_.find(socket) != channels_.end()) {
		if(channel->IsNoneEvent()) {
			is_fd_read_reset_ = true;
			is_fd_write_reset_ = true;
			is_fd_exp_reset_ = true;
			channels_.erase(socket);
		}
		else {
			//is_fd_read_reset_ = true;
			is_fd_write_reset_ = true;
		}
	}
	else {
		if(!channel->IsNoneEvent()) {
			channels_.emplace(socket, channel);
			is_fd_read_reset_ = true;
			is_fd_write_reset_ = true;
			is_fd_exp_reset_ = true;
		}	
	}	
}

void SelectTaskScheduler::RemoveChannel(ChannelPtr& channel)
{
	std::lock_guard<std::mutex> lock(mutex_);

	SOCKET fd = channel->GetSocket();

	if(channels_.find(fd) != channels_.end()) {
		is_fd_read_reset_ = true;
		is_fd_write_reset_ = true;
		is_fd_exp_reset_ = true;
		channels_.erase(fd);
	}
}

bool SelectTaskScheduler::HandleEvent(int timeout)
{	
	if(channels_.empty()) {
		if (timeout <= 0) {
			timeout = 10;
		}
         
		Timer::Sleep(timeout);
		return true;
	}

	fd_set fd_read;
	fd_set fd_write;
	fd_set fd_exp;
	FD_ZERO(&fd_read);
	FD_ZERO(&fd_write);
	FD_ZERO(&fd_exp);
	bool fd_read_reset = false;
	bool fd_write_reset = false;
	bool fd_exp_reset = false;

	if(is_fd_read_reset_ || is_fd_write_reset_ || is_fd_exp_reset_) {
		if (is_fd_exp_reset_) {
			maxfd_ = 0;
		}
          
		std::lock_guard<std::mutex> lock(mutex_);
		for(auto iter : channels_) {
			int events = iter.second->GetEvents();
			SOCKET fd = iter.second->GetSocket();

			if (is_fd_read_reset_ && (events&EVENT_IN)) {
				FD_SET(fd, &fd_read);
			}

			if(is_fd_write_reset_ && (events&EVENT_OUT)) {
				FD_SET(fd, &fd_write);
			}

			if(is_fd_exp_reset_) {
				FD_SET(fd, &fd_exp);
				if(fd > maxfd_) {
					maxfd_ = fd;
				}
			}		
		}
        
		fd_read_reset = is_fd_read_reset_;
		fd_write_reset = is_fd_write_reset_;
		fd_exp_reset = is_fd_exp_reset_;
		is_fd_read_reset_ = false;
		is_fd_write_reset_ = false;
		is_fd_exp_reset_ = false;
	}
	
	if(fd_read_reset) {
		FD_ZERO(&fd_read_backup_);
		memcpy(&fd_read_backup_, &fd_read, sizeof(fd_set)); 
	}
	else {
		memcpy(&fd_read, &fd_read_backup_, sizeof(fd_set));
	}
       

	if(fd_write_reset) {
		FD_ZERO(&fd_write_backup_);
		memcpy(&fd_write_backup_, &fd_write, sizeof(fd_set));
	}
	else {
		memcpy(&fd_write, &fd_write_backup_, sizeof(fd_set));
	}
     

	if(fd_exp_reset) {
		FD_ZERO(&fd_exp_backup_);
		memcpy(&fd_exp_backup_, &fd_exp, sizeof(fd_set));
	}
	else {
		memcpy(&fd_exp, &fd_exp_backup_, sizeof(fd_set));
	}

	if(timeout <= 0) {
		timeout = 10;
	}

	struct timeval tv = { timeout/1000, timeout%1000*1000 };
	int ret = select((int)maxfd_+1, &fd_read, &fd_write, &fd_exp, &tv); 	
	if (ret < 0) {
#if defined(__linux) || defined(__linux__) 
	if(errno == EINTR) {
		return true;
	}					
#endif 
		return false;
	}

	std::forward_list<std::pair<ChannelPtr, int>> event_list;
	if(ret > 0) {
		std::lock_guard<std::mutex> lock(mutex_);
		for(auto iter : channels_) {
			int events = 0;
			SOCKET socket = iter.second->GetSocket();

			if (FD_ISSET(socket, &fd_read)) {
				events |= EVENT_IN;
			}

			if (FD_ISSET(socket, &fd_write)) {
				events |= EVENT_OUT;
			}

			if (FD_ISSET(socket, &fd_exp)) {
				events |= (EVENT_HUP); // close
			}

			if(events != 0) {
				event_list.emplace_front(iter.second, events);
			}
		}
	}	

	for(auto& iter: event_list) {
		iter.first->HandleEvent(iter.second);
	}

	return true;
}



