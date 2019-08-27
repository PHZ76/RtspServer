// PHZ
// 2018-5-15

#ifndef XOP_EPOLL_TASK_SCHEDULER_H
#define XOP_EPOLL_TASK_SCHEDULER_H

#include "TaskScheduler.h"
#include <mutex>
#include <unordered_map>

namespace xop
{	
class EpollTaskScheduler : public TaskScheduler
{
public:
    EpollTaskScheduler(int id);
    virtual ~EpollTaskScheduler();

    void updateChannel(ChannelPtr channel);
    void removeChannel(ChannelPtr& channel);

    // timeout: ms
    bool handleEvent(int timeout);

private:
    void update(int operation, ChannelPtr& channel);

    int _epollfd = -1;
    std::mutex _mutex;
    std::unordered_map<int, ChannelPtr> _channels;
};

}

#endif
