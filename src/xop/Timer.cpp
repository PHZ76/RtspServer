#include "Timer.h"
#include <iostream>

using namespace xop;
using namespace std;
using namespace std::chrono;

TimerId TimerQueue::addTimer(const TimerEvent& event, uint32_t ms, bool repeat)
{
    std::lock_guard<std::mutex> locker(_mutex);

    int64_t timeoutPoint = getTimeNow();
    TimerId timerId = {timeoutPoint+ms, ++_lastTimerId};
    auto timer = make_shared<Timer>(event, ms, repeat);	

    timer->setNextTimeout(timeoutPoint);

    if(repeat)
    {
        _repeatTimers.emplace(timerId.second, timer);
    }

    _timers.emplace(timerId, std::move(timer)); 

    return timerId;
}

void TimerQueue::removeTimer(TimerId timerId)
{
    std::lock_guard<std::mutex> locker(_mutex);

    auto iter = _repeatTimers.find(timerId.second);	
    if(iter != _repeatTimers.end())
    {
        TimerId t = {iter->second->getNextTimeout(), timerId.second};
        _repeatTimers.erase(iter);
        _timers.erase(t);		
    }
    else
    {
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

    if(_timers.empty())
     return -1;

    int64_t ms = _timers.begin()->first.first - getTimeNow();
    if(ms <= 0)
        return 0; 

    return ms;
}

void TimerQueue::handleTimerEvent()
{
    if(!_timers.empty() || !_repeatTimers.empty())
    {
        std::lock_guard<std::mutex> locker(_mutex);

        int64_t timePoint = getTimeNow();
        while(!_timers.empty() && _timers.begin()->first.first<=timePoint)
        {	
            _timers.begin()->second->eventCallback();				
            if(_timers.begin()->second->isRepeat())
            {
                _timers.begin()->second->setNextTimeout(timePoint);
                TimerId t = {_timers.begin()->second->getNextTimeout(), _timers.begin()->first.second};		
                auto timerPtr = std::move(_timers.begin()->second);			
                _timers.erase(_timers.begin());
                _timers.emplace(t, std::move(timerPtr));
            }
            else		
            {
                _timers.erase(_timers.begin());
            }
        }	
    }
}

