// PHZ
// 2018-5-15

#ifndef XOP_BUFFER_WRITER_H
#define XOP_BUFFER_WRITER_H

#include <cstdint>
#include <memory>
#include <queue>
#include <string>
#include "Socket.h"

namespace xop
{

void writeUint32BE(char* p, uint32_t value);
void writeUint32LE(char* p, uint32_t value);
void writeUint24BE(char* p, uint32_t value);
void writeUint24LE(char* p, uint32_t value);
void writeUint16BE(char* p, uint16_t value);
void writeUint16LE(char* p, uint16_t value);
	
class BufferWriter
{
public:
    BufferWriter(int capacity=kMaxQueueLength);
    ~BufferWriter() {}

    bool append(std::shared_ptr<char> data, uint32_t size, uint32_t index=0);
    bool append(const char* data, uint32_t size, uint32_t index=0);
    int send(SOCKET sockfd, int timeout=0); // timeout: ms

    bool isEmpty() const 
    { return _buffer->empty(); }

    bool isFull() const 
    { return ((int)_buffer->size()>=_maxQueueLength?true:false); }

    uint32_t size() const 
    { return (uint32_t)_buffer->size(); }
	
private:
    typedef struct 
    {
        std::shared_ptr<char> data;
        uint32_t size;
        uint32_t writeIndex;
    } Packet;

    std::shared_ptr<std::queue<Packet>> _buffer;  		
    int _maxQueueLength = 0;
	 
    static const int kMaxQueueLength = 10000;
};

}

#endif

