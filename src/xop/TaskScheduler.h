#ifndef XOP_TASK_SCHEDULER_H
#define XOP_TASK_SCHEDULER_H

#include "Channel.h"

namespace xop
{

class TaskScheduler 
{
public:
	virtual ~TaskScheduler() {}

	virtual void updateChannel(ChannelPtr channel) = 0;
	virtual void removeChannel(ChannelPtr& channel) = 0;
	virtual bool handleEvent(int timeout) = 0;

protected:
	
};

}
#endif  
