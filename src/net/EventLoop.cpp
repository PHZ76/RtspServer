// PHZ
// 2018-5-24

#include "EventLoop.h"

#if defined(WIN32) || defined(_WIN32) 
#include<windows.h>
#endif

#if defined(WIN32) || defined(_WIN32) 
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib,"Iphlpapi.lib")
#endif 

using namespace xop;

EventLoop::EventLoop(int nThreads)
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
	if (nThreads <= 0)
	{
		nThreads = 1;
	}

	for (int n=0; n<nThreads; n++)
	{	
#if defined(__linux) || defined(__linux__) 
		std::shared_ptr<TaskScheduler> taskSchedulerPtr(new EpollTaskScheduler(n));
#elif defined(WIN32) || defined(_WIN32) 
		std::shared_ptr<TaskScheduler> taskSchedulerPtr(new SelectTaskScheduler(n));
#endif
		_taskSchedulers.push_back(taskSchedulerPtr);
		if (n != 0)
		{
			std::shared_ptr<std::thread> t(new std::thread(&TaskScheduler::start, taskSchedulerPtr.get()));
			_threads.push_back(t);
		}
	}
}

EventLoop::~EventLoop()
{
    for (auto iter : _taskSchedulers)
    {
        iter->stop();
    }

    for (auto iter : _threads)
    {
        iter->join();
    }
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
	_taskSchedulers[0]->start();
}

void EventLoop::quit()
{
	_taskSchedulers[0]->stop();
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
