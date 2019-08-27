// PHZ
// 2018-5-15

#ifndef XOP_TASK_SCHEDULER_H
#define XOP_TASK_SCHEDULER_H

#include "Channel.h"
#include "Pipe.h"
#include "Timer.h"
#include "RingBuffer.h"

namespace xop
{

typedef std::function<void(void)> TriggerEvent;

class TaskScheduler 
{
public:
    TaskScheduler(int id=1);
    virtual ~TaskScheduler();

    void start();
    void stop();
    TimerId addTimer(TimerEvent timerEvent, uint32_t msec);
    void removeTimer(TimerId timerId);
    bool addTriggerEvent(TriggerEvent callback);

    virtual void updateChannel(ChannelPtr channel) { };
    virtual void removeChannel(ChannelPtr& channel) { };
    virtual bool handleEvent(int timeout) { return false; };

    int getId() const 
    { return _id; }

protected:
    void wake();
    void handleTriggerEvent();

    int _id = 0;
    std::atomic_bool _shutdown;
    std::shared_ptr<Pipe> _wakeupPipe;
    std::shared_ptr<Channel> _wakeupChannel;

    typedef xop::RingBuffer<TriggerEvent> TriggerEventQueue;
    std::shared_ptr<TriggerEventQueue> _triggerEvents;

    std::mutex _mutex;
    TimerQueue _timerQueue;

    static const char kTriggetEvent = 1;
    static const char kTimerEvent = 2;
    static const int kMaxTriggetEvents = 5000;
};

}
#endif  

