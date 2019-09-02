#include "Timer.h"
#include <iostream>

using namespace xop;
using namespace std;
using namespace std::chrono;

TimerId TimerQueue::addTimer(const TimerEvent& event, uint32_t ms)
{    
    std::lock_guard<std::mutex> locker(_mutex);
    int64_t timeout = getTimeNow();
    TimerId timerId = ++_lastTimerId;

    auto timer = make_shared<Timer>(event, ms);	
    timer->setNextTimeout(timeout);
    _timers.emplace(timerId, timer);
    _events.emplace(std::pair<int64_t, TimerId>(timeout + ms, timerId), std::move(timer));
    return timerId;
}

void TimerQueue::removeTimer(TimerId timerId)
{
    std::lock_guard<std::mutex> locker(_mutex);
    auto iter = _timers.find(timerId);
    if (iter != _timers.end())
    {
        int64_t timeout = iter->second->getNextTimeout();
        _events.erase(std::pair<int64_t, TimerId>(timeout, timerId));
        _timers.erase(timerId);
    }
}

int64_t TimerQueue::getTimeNow()
{	
    auto timePoint = steady_clock::now();	
    return duration_cast<milliseconds>(timePoint.time_since_epoch()).count();	
}

int64_t TimerQueue::getTimeRemaining()
{	
    std::lock_guard<std::mutex> locker(_mutex);

    if (_timers.empty())
    {
        return -1;
    }

    int64_t msec = _events.begin()->first.first - getTimeNow();
    if (msec <= 0)
    {
        msec = 0;
    }

    return msec;
}

void TimerQueue::handleTimerEvent()
{
    if(!_timers.empty())
    {
        std::lock_guard<std::mutex> locker(_mutex);
        int64_t timePoint = getTimeNow();
        while(!_timers.empty() && _events.begin()->first.first<=timePoint)
        {	
            TimerId timerId = _events.begin()->first.second;
            bool flag = _events.begin()->second->eventCallback();
            if(flag == true)
            {
                _events.begin()->second->setNextTimeout(timePoint);
                auto timerPtr = std::move(_events.begin()->second);
                _events.erase(_events.begin());
                _events.emplace(std::pair<int64_t, TimerId>(timerPtr->getNextTimeout(), timerId), timerPtr);
            }
            else		
            {		
				_events.erase(_events.begin());
                _timers.erase(timerId);				
            }
        }	
    }
}

