#ifndef _QUEUE_H
#define _QUEUE_H

#include <mutex>
#include <queue>
#include <condition_variable>

template<typename T>
class threadsafe_queue
{
public:
    threadsafe_queue()
    {
        
    }
    threadsafe_queue(threadsafe_queue const& other)
    {
        std::lock_guard<std::mutex> lock(other._mutex);
        _dataQueue = other._dataQueue;
    }

    ~threadsafe_queue()
    {
        clear();
    }
            
    void push(T new_value)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _dataQueue.push(new_value);
        _dataCond.notify_one();
    }
        
    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _dataCond.wait(lock,[this]{return !_dataQueue.empty();});
        value = _dataQueue.front();
        _dataQueue.pop();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _dataCond.wait(lock,[this]{return !_dataQueue.empty();});
        std::shared_ptr<T> res(std::make_shared<T>(_dataQueue.front()));
        _dataQueue.pop();
        return res;
    }

    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if(_dataQueue.empty())
            return false;
        
        value = _dataQueue.front();
        _dataQueue.pop();
        
        return true;
    }

    std::shared_ptr<T> try_pop()
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
        std::swap(empty, _dataQueue);
    } 
    
private:	
    mutable std::mutex _mutex;
    std::queue<T> _dataQueue;
    std::condition_variable _dataCond;    
};

#endif 

