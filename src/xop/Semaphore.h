#ifndef XOP_SEMAPHORE_H
#define XOP_SEMAPHORE_H

#include <atomic>
#include <mutex>
#include <condition_variable>

namespace xop
{

class Semaphore
{
public:
    Semaphore()
    {
        _count = 0;    
    }
    
    ~Semaphore()
    {

    }
    
    void post(unsigned int n = 1) 
    {
		_count += n;
		do
        {
			_cond.notify_one();
		}while(--n);
	}

    void wait() 
    {
        std::unique_lock<std::mutex> lock(_mutex);
        while (_count == 0) 
        {			
			_cond.wait(lock);
		}
		--_count;
	}

private:
    std::atomic_int _count;
    std::mutex _mutex;
    std::condition_variable_any _cond;
    
};

}

#endif