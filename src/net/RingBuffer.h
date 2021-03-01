// PHZ
// 2018-5-15

#ifndef XOP_RING_BUFFER_H
#define XOP_RING_BUFFER_H

#include <vector>
#include <memory>
#include <atomic>
#include <iostream>

namespace xop
{

template <typename T>
class RingBuffer
{
public:
	RingBuffer(int capacity=60)
		: capacity_(capacity)
		, num_datas_(0)
		, buffer_(capacity)
	{ }
	
	virtual ~RingBuffer() {	}

	bool Push(const T& data) 
	{ 
		return pushData(std::forward<T>(data)); 
	} 	

	bool Push(T&& data) 
	{ 
		return PushData(data); 
	} 
        
	bool Pop(T& data)
	{
		if(num_datas_ > 0) {
			data = std::move(buffer_[get_pos_]);
			Add(get_pos_);
			num_datas_--;
			return true;
		}

		return false;
	}	

	bool IsFull()  const 
	{ 
		return ((num_datas_==capacity_) ? true : false); 
	}	

	bool IsEmpty() const 
	{ 
		return ((num_datas_==0) ? true : false); 
	}

	int  Size() const 
	{ 
		return num_datas_; 
	}
	
private:		
	template <typename F>
	bool PushData(F&& data)
	{
		if (num_datas_ < capacity_)
		{
			buffer_[put_pos_] = std::forward<F>(data);
			Add(put_pos_);
			num_datas_++;
			return true;
		}

		return false;
	}

	void Add(int& pos)
	{	
		pos = (((pos+1)==capacity_) ? 0 : (pos+1));
	}

	int capacity_ = 0;
	int put_pos_ = 0;
	int get_pos_ = 0;

	std::atomic_int num_datas_;     			
	std::vector<T> buffer_;
};

}

#endif


