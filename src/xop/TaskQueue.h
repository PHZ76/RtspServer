#ifndef XOP_TASK_QUEUE_H
#define XOP_TASK_QUEUE_H

#include <cstdint>
#include <mutex>
#include <queue>
#include <condition_variable>

namespace xop
{

template<typename T>
class TaskQueue
{
public:
    TaskQueue()
    {
		
	}
    
    TaskQueue(TaskQueue const& other)
    {
        std::lock_guard<std::mutex> lock(other._mutex);
        _queue = other._queue;
    }
	
	~TaskQueue()
    {
		clear();
	}
	        
    void push(T task)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(task);
        _cond.notify_one();
    }
        
    bool wait(T& value, uint32_t millsecond)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if(_queue.size() == 0)
        {
            _cond.wait_for(lock, std::chrono::milliseconds(millsecond));
            if(_queue.size() == 0)
            {
                return false;
            }
        }
        
        value = _queue.front();
        _queue.pop();
        return true;
    }
    
    bool pop(T& value)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if(_queue.empty())
            return false;
		
        value = _queue.front();
        _queue.pop();
		
		return true;
    }

	size_t size() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }
	
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.empty();
    }
	
 	void clear()
	{		
        T value;
		while(this->pop(value));
	} 
    
private:	
    mutable std::mutex _mutex;
    std::queue<T> _queue;
    std::condition_variable _cond;    
};
 
}

#endif
