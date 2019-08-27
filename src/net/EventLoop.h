// PHZ
// 2018-5-15

#ifndef XOP_EVENT_LOOP_H
#define XOP_EVENT_LOOP_H

#include <memory>
#include <atomic>
#include <unordered_map>
#include <functional>
#include <queue>
#include <thread>
#include <mutex>

#include "SelectTaskScheduler.h"
#include "EpollTaskScheduler.h"
#include "Pipe.h"
#include "Timer.h"
#include "RingBuffer.h"

namespace xop
{
	
class EventLoop 
{
    public:
    EventLoop(int nThreads=1); //std::thread::hardware_concurrency()
    virtual ~EventLoop();

    void loop();
    void quit();
    std::shared_ptr<TaskScheduler> getTaskScheduler();

    bool addTriggerEvent(TriggerEvent callback);
    TimerId addTimer(TimerEvent timerEvent, uint32_t msec);
    void removeTimer(TimerId timerId);	
    void updateChannel(ChannelPtr channel);
    void removeChannel(ChannelPtr& channel);
	
private:
    std::mutex _mutex;
    uint32_t _index = 1;
    std::vector<std::shared_ptr<TaskScheduler>> _taskSchedulers;
    std::vector<std::shared_ptr<std::thread>> _threads;
};

}

#endif

