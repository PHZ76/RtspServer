#include "TaskScheduler.h"
#if defined(__linux) || defined(__linux__) 
#include <signal.h>
#endif

using namespace xop;

TaskScheduler::TaskScheduler(int id)
	: _id(id)
	, _shutdown(false)
	, _wakeupPipe(std::make_shared<Pipe>())
	, _triggerEvents(new TriggerEventQueue(kMaxTriggetEvents))
{
    if (_wakeupPipe->create())
    {
        _wakeupChannel.reset(new Channel(_wakeupPipe->readfd()));
        _wakeupChannel->enableReading();
        _wakeupChannel->setReadCallback([this]() { this->wake(); });		
    }        
}

TaskScheduler::~TaskScheduler()
{

}

void TaskScheduler::start()
{
#if defined(__linux) || defined(__linux__) 
	signal(SIGPIPE, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGKILL, SIG_IGN);
#endif     
	_shutdown = false;
	while (!_shutdown)
	{
		this->handleTriggerEvent();
		this->_timerQueue.handleTimerEvent();
		int64_t timeout = this->_timerQueue.getTimeRemaining();
		this->handleEvent((int)timeout);
	}
}

void TaskScheduler::stop()
{
	_shutdown = true;
	char event = kTriggetEvent;
	_wakeupPipe->write(&event, 1);
}

TimerId TaskScheduler::addTimer(TimerEvent timerEvent, uint32_t msec)
{
	TimerId id = _timerQueue.addTimer(timerEvent, msec);
	return id;
}

void TaskScheduler::removeTimer(TimerId timerId)
{
	_timerQueue.removeTimer(timerId);
}

bool TaskScheduler::addTriggerEvent(TriggerEvent callback)
{
	if (_triggerEvents->size() < kMaxTriggetEvents)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		char event = kTriggetEvent;
		_triggerEvents->push(std::move(callback));
		_wakeupPipe->write(&event, 1);
		return true;
	}

	return false;
}

void TaskScheduler::wake()
{
	char event[10] = { 0 };
	while (_wakeupPipe->read(event, 10) > 0);

	return;
}

void TaskScheduler::handleTriggerEvent()
{
	do
	{
		TriggerEvent callback;
		if (_triggerEvents->pop(callback))
		{
			callback();
		}
	} while (_triggerEvents->size() > 0);
}