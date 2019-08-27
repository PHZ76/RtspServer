// PHZ
// 2018-5-15

#ifndef _XOP_TIMER_H
#define _XOP_TIMER_H

#include <map>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <cstdint>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

namespace xop
{
    
typedef std::function<bool(void)> TimerEvent;
typedef uint32_t TimerId;

class Timer
{
public:
    Timer(const TimerEvent& event, uint32_t msec)
        : eventCallback(event)
        , _interval(msec)
    {
        if (_interval == 0)
            _interval = 1;
    }

    Timer() { }

    static void sleep(uint32_t msec)
    { 
        std::this_thread::sleep_for(std::chrono::milliseconds(msec));
    }

    void setEventCallback(const TimerEvent& event)
    {
        eventCallback = event;
    }

    void start(int64_t microseconds, bool repeat = false)
    {
        _isRepeat = repeat;
        auto timeBegin = std::chrono::high_resolution_clock::now();
        int64_t elapsed = 0;

        do
        {
            std::this_thread::sleep_for(std::chrono::microseconds(microseconds - elapsed));
            timeBegin = std::chrono::high_resolution_clock::now();
            eventCallback();
            elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - timeBegin).count();
            if (elapsed < 0)
                elapsed = 0;
        } while (_isRepeat);
    }

    void stop()
    {
        _isRepeat = false;
    }	

private:
    friend class TimerQueue;

    void setNextTimeout(int64_t currentTimePoint)
    {
        _nextTimeout = currentTimePoint + _interval;
    }

    int64_t getNextTimeout() const
    {
        return _nextTimeout;
    }

    bool _isRepeat = false;
    TimerEvent eventCallback = [] { return false; };
    uint32_t _interval = 0;
    int64_t _nextTimeout = 0;
};

class TimerQueue
{
public:
    TimerId addTimer(const TimerEvent& event, uint32_t msec);
    void removeTimer(TimerId timerId);

    int64_t getTimeRemaining();
    void handleTimerEvent();

private:
    int64_t getTimeNow();

    std::mutex _mutex;
    std::unordered_map<TimerId, std::shared_ptr<Timer>> _timers;
    std::map<std::pair<int64_t, TimerId>, std::shared_ptr<Timer>> _events;
    uint32_t _lastTimerId = 0;
    uint32_t _timeRemaining = 0;
};

}

#endif 



