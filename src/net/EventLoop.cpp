// PHZ
// 2019-10-18

#include "EventLoop.h"

#if defined(WIN32) || defined(_WIN32) 
#include<windows.h>
#endif

#if defined(WIN32) || defined(_WIN32) 
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib,"Iphlpapi.lib")
#endif 

using namespace xop;

EventLoop::EventLoop(uint32_t nThreads)
	: _index(1)
{
    static std::once_flag oc_init;
	std::call_once(oc_init, [] {
#if defined(WIN32) || defined(_WIN32) 
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData))
		{
			WSACleanup();
		}
#endif
	});

	_nThreads = 1;
	if (nThreads > 0)
	{
		_nThreads = nThreads;
	}

	this->loop();
}

EventLoop::~EventLoop()
{
	this->quit();
}

std::shared_ptr<TaskScheduler> EventLoop::getTaskScheduler()
{
    std::lock_guard<std::mutex> locker(_mutex);
    if (_taskSchedulers.size() == 1)
    {
        return _taskSchedulers.at(0);
    }
    else
    {
        auto taskSchedulers = _taskSchedulers.at(_index);
        _index++;
        if (_index >= _taskSchedulers.size())
        {
            _index = 1;
        }		
        return taskSchedulers;
    }

    return nullptr;
}

void EventLoop::loop()
{
	std::lock_guard<std::mutex> locker(_mutex);

	for (uint32_t n = 0; n < _nThreads; n++)
	{
#if defined(__linux) || defined(__linux__) 
		std::shared_ptr<TaskScheduler> taskSchedulerPtr(new EpollTaskScheduler(n));
#elif defined(WIN32) || defined(_WIN32) 
		std::shared_ptr<TaskScheduler> taskSchedulerPtr(new SelectTaskScheduler(n));
#endif
		_taskSchedulers.push_back(taskSchedulerPtr);
		std::shared_ptr<std::thread> t(new std::thread(&TaskScheduler::start, taskSchedulerPtr.get()));
		t->native_handle();
		_threads.push_back(t);
	}

	int priority = TASK_SCHEDULER_PRIORITY_REALTIME;

	for (auto iter : _threads)
	{
#if defined(__linux) || defined(__linux__) 

#elif defined(WIN32) || defined(_WIN32) 
		switch (priority) 
		{
		case TASK_SCHEDULER_PRIORITY_LOW:
			SetThreadPriority(iter->native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
			break;
		case TASK_SCHEDULER_PRIORITY_NORMAL:
			SetThreadPriority(iter->native_handle(), THREAD_PRIORITY_NORMAL);
			break;
		case TASK_SCHEDULER_PRIORITYO_HIGH:
			SetThreadPriority(iter->native_handle(), THREAD_PRIORITY_ABOVE_NORMAL);
			break;
		case TASK_SCHEDULER_PRIORITY_HIGHEST:
			SetThreadPriority(iter->native_handle(), THREAD_PRIORITY_HIGHEST);
			break;
		case TASK_SCHEDULER_PRIORITY_REALTIME:
			SetThreadPriority(iter->native_handle(), THREAD_PRIORITY_TIME_CRITICAL);
			break;
		}
#endif
	}
}

void EventLoop::quit()
{
	std::lock_guard<std::mutex> locker(_mutex);
	for (auto iter : _taskSchedulers)
	{
		iter->stop();
	}

	for (auto iter : _threads)
	{
		iter->join();
	}
}
	
void EventLoop::updateChannel(ChannelPtr channel)
{
	_taskSchedulers[0]->updateChannel(channel);
}

void EventLoop::removeChannel(ChannelPtr& channel)
{
	_taskSchedulers[0]->removeChannel(channel);
}

TimerId EventLoop::addTimer(TimerEvent timerEvent, uint32_t msec)
{
	return _taskSchedulers[0]->addTimer(timerEvent, msec);
}

void EventLoop::removeTimer(TimerId timerId)
{
	return _taskSchedulers[0]->removeTimer(timerId);
}

bool EventLoop::addTriggerEvent(TriggerEvent callback)
{   
	return _taskSchedulers[0]->addTriggerEvent(callback);
}
