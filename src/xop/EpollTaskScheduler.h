#ifndef XOP_EPOLL_TASK_SCHEDULER_H
#define XOP_EPOLL_TASK_SCHEDULER_H

#include "TaskScheduler.h"
#include <mutex>
#include <unordered_map>

namespace xop
{	
class EpollTaskScheduler : public TaskScheduler
{
public:
    EpollTaskScheduler();
    virtual ~EpollTaskScheduler();

    void updateChannel(ChannelPtr channel);
    void removeChannel(ChannelPtr& channel);

    // timeout: ms
    bool handleEvent(int timeout);

private:
    void update(int operation, ChannelPtr& channel);

    int _epollfd = -1;
    std::mutex _mutex;
    std::unordered_map<int, ChannelPtr> _channels;
};

}

#endif

/*
example:

#include <string>
#include <iostream>
#include "EpollTaskScheduler.h"
#include "SelectTaskScheduler.h"

using namespace std;
using namespace xop;

void test()
{
	string input;
	cin >> input;
	cout << input << endl;
}

int main()
{
	// TaskScheduler *taskScheduler = new EpollTaskScheduler();
	TaskScheduler *taskScheduler = new SelectTaskScheduler();
	std::shared_ptr<Channel> chn(new Channel(0)); // 0 -- stdin
	
	chn->setEvents(EVENT_IN);
	chn->setReadCallback(std::bind(test));
	taskScheduler->updateChannel(chn);
	
	while(1)
	{
		taskScheduler->handleEvent(1000);
	}
	
	return 0;
}

*/