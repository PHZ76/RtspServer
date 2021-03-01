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
		: event_callback_(event)
		, interval_(msec)
	{
		if (interval_ == 0) {
			interval_ = 1;
		}
	}

	static void Sleep(uint32_t msec)
	{ 
		std::this_thread::sleep_for(std::chrono::milliseconds(msec));
	}

	void SetEventCallback(const TimerEvent& event)
	{
		event_callback_ = event;
	}

	void Start(int64_t microseconds, bool repeat = false)
	{
		is_repeat_ = repeat;
		auto time_begin = std::chrono::high_resolution_clock::now();
		int64_t elapsed = 0;

		do
		{
			std::this_thread::sleep_for(std::chrono::microseconds(microseconds - elapsed));
			time_begin = std::chrono::high_resolution_clock::now();
			if (event_callback_) {
				event_callback_();
			}			
			elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - time_begin).count();
			if (elapsed < 0) {
				elapsed = 0;
			}
            
		} while (is_repeat_);
	}

	void Stop()
	{
		is_repeat_ = false;
	}	

private:
	friend class TimerQueue;

	void SetNextTimeout(int64_t time_point)
	{
		next_timeout_ = time_point + interval_;
	}

	int64_t getNextTimeout() const
	{
		return next_timeout_;
	}

	bool is_repeat_ = false;
	TimerEvent event_callback_ = [] { return false; };
	uint32_t interval_ = 0;
	int64_t  next_timeout_ = 0;
};

class TimerQueue
{
public:
	TimerId AddTimer(const TimerEvent& event, uint32_t msec);
	void RemoveTimer(TimerId timerId);

	int64_t GetTimeRemaining();
	void HandleTimerEvent();

private:
	int64_t GetTimeNow();

	std::mutex mutex_;
	std::unordered_map<TimerId, std::shared_ptr<Timer>> timers_;
	std::map<std::pair<int64_t, TimerId>, std::shared_ptr<Timer>> events_;
	uint32_t last_timer_id_ = 0;
};

}

#endif 



