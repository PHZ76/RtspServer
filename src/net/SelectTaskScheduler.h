// PHZ
// 2018-5-15

#ifndef XOP_SELECT_TASK_SCHEDULER_H
#define XOP_SELECT_TASK_SCHEDULER_H

#include "TaskScheduler.h"
#include "Socket.h"
#include <mutex>
#include <unordered_map>

#if defined(__linux) || defined(__linux__) 
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace xop
{	

class SelectTaskScheduler : public TaskScheduler
{
public:
    SelectTaskScheduler(int id);
    virtual ~SelectTaskScheduler();

    void updateChannel(ChannelPtr channel);
    void removeChannel(ChannelPtr& channel);
    bool handleEvent(int timeout);
	
private:
    fd_set _fdReadBackup;
    fd_set _fdWriteBackup;
    fd_set _fdExpBackup;
	SOCKET _maxfd = 0;

    bool _isfdReadReset = false;
    bool _isfdWriteReset = false;
    bool _isfdExpReset = false;

    std::mutex _mutex;
    std::unordered_map<SOCKET, ChannelPtr> _channels;
};

}

#endif
