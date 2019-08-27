#ifndef Thread_SAFE_QUEUE_H
#define Thread_SAFE_QUEUE_H

#include <mutex>
#include <queue>
#include <condition_variable>

namespace xop
{
    
template<typename T>
class ThreadSafeQueue
{
public:
    ThreadSafeQueue()
    {
        
    }
    
    ThreadSafeQueue(ThreadSafeQueue const& other)
    {
        std::lock_guard<std::mutex> lock(other._mutex);
        _dataQueue = other._dataQueue;
    }

    ~ThreadSafeQueue()
    {
       
    }
            
    void push(T value)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _dataQueue.push(value);
        _dataCond.notify_one();
    }
        
    bool waitAndPop(T& value)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _dataCond.wait(lock);
		if (_dataQueue.empty())
		{
			return false;
		}
        value = _dataQueue.front();
        _dataQueue.pop();
		return true;
    }

    std::shared_ptr<T> waitAndPop()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _dataCond.wait(lock);
		if (_dataQueue.empty())
		{
			return nullptr;
		}
        std::shared_ptr<T> res(std::make_shared<T>(_dataQueue.front()));
        _dataQueue.pop();
        return res;
    }

    bool tryPop(T& value)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if(_dataQueue.empty())
            return false;
        
        value = _dataQueue.front();
        _dataQueue.pop();
        
        return true;
    }

    std::shared_ptr<T> tryPop()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if(_dataQueue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res(std::make_shared<T>(_dataQueue.front()));
        _dataQueue.pop();
        return res;
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _dataQueue.size();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _dataQueue.empty();
    }

    void clear()
    {		
        std::lock_guard<std::mutex> lock(_mutex);
        std::queue<T> empty;
        _dataQueue.swap(empty);
    } 
    
	void wake()
	{
		_dataCond.notify_one();
	}

private:	
    mutable std::mutex _mutex;
    std::queue<T> _dataQueue;
    std::condition_variable _dataCond;    
};

}

#endif 

