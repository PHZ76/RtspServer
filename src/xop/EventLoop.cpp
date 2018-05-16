// PHZ
// 2018-5-15

#include "EventLoop.h"
#include "Signal.h"

using namespace xop;

EventLoop::EventLoop(TaskScheduler* taskScheduler)
    : _shutdown(false)
    , _taskScheduler(taskScheduler)
    , _wakeupPipe(std::make_shared<Pipe>())
    , _triggerEvents(new TriggerEventQueue(kMaxTriggetEvents))
{
    if(!_taskScheduler)
    {
#if defined(__linux) || defined(__linux__) 
        _taskScheduler = new EpollTaskScheduler(); // SelectTaskScheduler(); 
#else
        _taskScheduler = new SelectTaskScheduler(); 
#endif
	}

    if(_wakeupPipe->create())
    {
        _wakeupChannel.reset(new Channel(_wakeupPipe->readfd()));
        _wakeupChannel->setEvents(EVENT_IN);
        _wakeupChannel->setReadCallback([this]() {this->wake();});
        _taskScheduler->updateChannel(_wakeupChannel);
    }
}

EventLoop::~EventLoop()
{
    delete _taskScheduler;
}

void EventLoop::loop()
{
    Signal::ignoreSignal();

    while(!_shutdown)
    {
        handleTriggerEvent();
        _timerQueue.handleTimerEvent();        
        int64_t timeout = _timerQueue.getTimeRemaining();
        _taskScheduler->handleEvent(timeout);        
    }
}

void EventLoop::quit()
{
    _shutdown = true;
}
	
void EventLoop::updateChannel(ChannelPtr channel)
{
    _taskScheduler->updateChannel(channel);
}

void EventLoop::removeChannel(ChannelPtr& channel)
{
    _taskScheduler->removeChannel(channel);
}

TimerId EventLoop::addTimer(TimerEvent timerEvent, uint32_t ms, bool repeat)
{
    TimerId id = _timerQueue.addTimer(timerEvent, ms, repeat);		
    return id;
}

void EventLoop::removeTimer(TimerId timerId)
{
    _timerQueue.removeTimer(timerId);
}

bool EventLoop::addTriggerEvent(TriggerEvent callback)
{   
    if(_triggerEvents->size() < kMaxTriggetEvents)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        char event = kTriggetEvent;
        _triggerEvents->push(std::move(callback));
        _wakeupPipe->write(&event, 1);
        return true;
    }

    return false;
}

void EventLoop::wake()
{
    char event[10] = {0};
    while(_wakeupPipe->read(event, 10) > 0);
    
    return ;
}

void EventLoop::handleTriggerEvent()
{
    do
    {
        TriggerEvent callback;
        if(_triggerEvents->pop(callback))
        {
            callback();	
        }           
    } while(_triggerEvents->size() > 0);
}

