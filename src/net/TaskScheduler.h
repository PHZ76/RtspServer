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

	void Start();
	void Stop();
	TimerId AddTimer(TimerEvent timerEvent, uint32_t msec);
	void RemoveTimer(TimerId timerId);
	bool AddTriggerEvent(TriggerEvent callback);

	virtual void UpdateChannel(ChannelPtr channel) { };
	virtual void RemoveChannel(ChannelPtr& channel) { };
	virtual bool HandleEvent(int timeout) { return false; };

	int GetId() const 
	{ return id_; }

protected:
	void Wake();
	void HandleTriggerEvent();

	int id_ = 0;
	std::atomic_bool is_shutdown_;
	std::unique_ptr<Pipe> wakeup_pipe_;
	std::shared_ptr<Channel> wakeup_channel_;
	std::unique_ptr<xop::RingBuffer<TriggerEvent>> trigger_events_;

	std::mutex mutex_;
	TimerQueue timer_queue_;

	static const char kTriggetEvent = 1;
	static const char kTimerEvent = 2;
	static const int  kMaxTriggetEvents = 50000;
};

}
#endif  

