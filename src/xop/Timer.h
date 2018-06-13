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
    
typedef std::function<void(void)> TimerEvent;
typedef std::pair<int64_t, uint32_t> TimerId;

class Timer
{
public:
    Timer(const TimerEvent& event, uint32_t ms, bool repeat)
        : eventCallback(event)
        , _interval(ms)
        , _isRepeat(repeat)
    {
        if (_interval == 0)
            _interval = 1;
    }

	Timer() { }

    bool isRepeat() const 
    { 
        return _isRepeat;
    }

    static void sleep(unsigned ms) 
    { 
        std::this_thread::sleep_for(std::chrono::milliseconds(ms)); 
    }

	void setEventCallback(const TimerEvent& event)
	{
		eventCallback = event;
	}

	void start(int64_t microseconds, bool repeat=false)
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

	TimerEvent eventCallback = [] {};
    bool _isRepeat = false;
    uint32_t _interval = 0;
    int64_t _nextTimeout = 0;
};

class TimerQueue
{
public:
    TimerId addTimer(const TimerEvent& event, uint32_t ms, bool repeat);
    void removeTimer(TimerId timerId);

    // 返回最近一次超时的时间, 没有定时器任务返回-1
    int64_t getTimeRemaining();
    void handleTimerEvent();

private:
    int64_t getTimeNow();

    std::mutex _mutex;
    std::map<TimerId, std::shared_ptr<Timer>> _timers;
    std::unordered_map<uint32_t, std::shared_ptr<Timer>> _repeatTimers;
    uint32_t _lastTimerId = 0;
    uint32_t _timeRemaining = 0;
};
}

#endif 



