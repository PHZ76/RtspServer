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

#define TASK_SCHEDULER_PRIORITY_LOW       0
#define TASK_SCHEDULER_PRIORITY_NORMAL    1
#define TASK_SCHEDULER_PRIORITYO_HIGH     2 
#define TASK_SCHEDULER_PRIORITY_HIGHEST   3
#define TASK_SCHEDULER_PRIORITY_REALTIME  4

namespace xop
{

class EventLoop 
{
public:
	EventLoop(const EventLoop&) = delete;
	EventLoop &operator = (const EventLoop&) = delete; 
	EventLoop(uint32_t nThreads=1); //std::thread::hardware_concurrency()
	virtual ~EventLoop();

	std::shared_ptr<TaskScheduler> getTaskScheduler();

	bool addTriggerEvent(TriggerEvent callback);
	TimerId addTimer(TimerEvent timerEvent, uint32_t msec);
	void removeTimer(TimerId timerId);	
	void updateChannel(ChannelPtr channel);
	void removeChannel(ChannelPtr& channel);
	
private:
	void loop();
	void quit();

	std::mutex _mutex;
	uint32_t _nThreads = 1;
	uint32_t _index = 1;
	std::vector<std::shared_ptr<TaskScheduler>> _taskSchedulers;
	std::vector<std::shared_ptr<std::thread>> _threads;
};

}

#endif

