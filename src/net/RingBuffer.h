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
    RingBuffer(unsigned capacity=60)
        : _buffer(capacity)
        , _capacity(capacity)
        , _numDatas(0)
    { }
	
    ~RingBuffer() {	}

    bool push(const T& data) { return pushData(std::forward<T>(data)); } 	
    bool push(T&& data) { return pushData(data); } 
        
    bool pop(T& data)
    {
        if(_numDatas > 0)
        {
            data = std::move(_buffer[_getPos]);
            add(_getPos);
            _numDatas--;
            return true;
        }

        return false;
    }	

    bool isFull()  const { return ((_numDatas==_capacity)?true:false); }	
    bool isEmpty() const { return ((_numDatas==0)?true:false); }
    int size() const { return _numDatas; }
	
private:		
	template <typename F>
	bool pushData(F&& data)
	{
        if (_numDatas < _capacity)
        {
        _buffer[_putPos] = std::forward<F>(data);
        add(_putPos);
        _numDatas++;
        return true;
        }

        return false;
	}

    void add(int& pos)
    {	
        pos = (((pos+1)==_capacity) ? 0 : (pos+1));
    }

    int _capacity = 0;	
    int _putPos   = 0;  					
    int _getPos   = 0;  		

    std::atomic_int _numDatas;     			
    std::vector<T> _buffer;	
};

}

#endif


