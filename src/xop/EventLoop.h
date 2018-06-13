// PHZ
// 2018-5-15

#ifndef XOP_EVENT_LOOP_H
#define XOP_EVENT_LOOP_H

#include <memory>
#include <atomic>
#include <unordered_map>
#include <functional>
#include <queue>
#include <mutex>

#include "SelectTaskScheduler.h"
#include "EpollTaskScheduler.h"
#include "Pipe.h"
#include "Timer.h"
#include "RingBuffer.h"

namespace xop
{
	
typedef std::function<void(void)> TriggerEvent; 
	
class EventLoop 
{
public:
    EventLoop(TaskScheduler* taskScheduler=nullptr);
    virtual ~EventLoop();

    void loop();
    void quit();

    bool addTriggerEvent(TriggerEvent callback);

    TimerId addTimer(TimerEvent timerEvent, uint32_t ms, bool repeat);
    void removeTimer(TimerId timerId);	

    void updateChannel(ChannelPtr channel);
    void removeChannel(ChannelPtr& channel);
	
private:
    void wake();
    void handleTriggerEvent();

    TaskScheduler* _taskScheduler;
    std::atomic_bool _shutdown;

    std::shared_ptr<Pipe> _wakeupPipe;
    std::shared_ptr<Channel> _wakeupChannel;

    //typedef std::queue<TriggerEvent> TriggerEventQueue;
    typedef RingBuffer<TriggerEvent> TriggerEventQueue;
    std::shared_ptr<TriggerEventQueue> _triggerEvents;
    std::mutex _mutex;

    TimerQueue _timerQueue;

    static const char kTriggetEvent = 1;
    static const char kTimerEvent = 2;
    static const int kMaxTriggetEvents = 1024;
};

}

#endif

