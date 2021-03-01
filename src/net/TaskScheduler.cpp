#include "TaskScheduler.h"
#if defined(__linux) || defined(__linux__) 
#include <signal.h>
#endif

using namespace xop;

TaskScheduler::TaskScheduler(int id)
	: id_(id)
	, is_shutdown_(false) 
	, wakeup_pipe_(new Pipe())
	, trigger_events_(new xop::RingBuffer<TriggerEvent>(kMaxTriggetEvents))
{
	static std::once_flag flag;
	std::call_once(flag, [] {
#if defined(WIN32) || defined(_WIN32) 
		WSADATA wsa_data;
		if (WSAStartup(MAKEWORD(2, 2), &wsa_data)) {
			WSACleanup();
		}
#endif
	});

	if (wakeup_pipe_->Create()) {
		wakeup_channel_.reset(new Channel(wakeup_pipe_->Read()));
		wakeup_channel_->EnableReading();
		wakeup_channel_->SetReadCallback([this]() { this->Wake(); });		
	}        
}

TaskScheduler::~TaskScheduler()
{
	
}

void TaskScheduler::Start()
{
#if defined(__linux) || defined(__linux__) 
	signal(SIGPIPE, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGKILL, SIG_IGN);
#endif     
	is_shutdown_ = false;
	while (!is_shutdown_) {
		this->HandleTriggerEvent();
		this->timer_queue_.HandleTimerEvent();
		int64_t timeout = this->timer_queue_.GetTimeRemaining();
		this->HandleEvent((int)timeout);
	}
}

void TaskScheduler::Stop()
{
	is_shutdown_ = true;
	char event = kTriggetEvent;
	wakeup_pipe_->Write(&event, 1);
}

TimerId TaskScheduler::AddTimer(TimerEvent timerEvent, uint32_t msec)
{
	TimerId id = timer_queue_.AddTimer(timerEvent, msec);
	return id;
}

void TaskScheduler::RemoveTimer(TimerId timerId)
{
	timer_queue_.RemoveTimer(timerId);
}

bool TaskScheduler::AddTriggerEvent(TriggerEvent callback)
{
	if (trigger_events_->Size() < kMaxTriggetEvents) {
		std::lock_guard<std::mutex> lock(mutex_);
		char event = kTriggetEvent;
		trigger_events_->Push(std::move(callback));
		wakeup_pipe_->Write(&event, 1);
		return true;
	}

	return false;
}

void TaskScheduler::Wake()
{
	char event[10] = { 0 };
	while (wakeup_pipe_->Read(event, 10) > 0);
}

void TaskScheduler::HandleTriggerEvent()
{
	do 
	{
		TriggerEvent callback;
		if (trigger_events_->Pop(callback)) {
			callback();
		}
	} while (trigger_events_->Size() > 0);
}
