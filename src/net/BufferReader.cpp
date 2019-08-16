// PHZ
// 2018-5-15

#include "BufferReader.h"
#include "Socket.h"
#include <cstring>
 
using namespace xop;
uint32_t xop::readUint32BE(char* data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    return value;
}

uint32_t xop::readUint32LE(char* data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
    return value;
}

uint32_t xop::readUint24BE(char* data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = (p[0] << 16) | (p[1] << 8) | p[2];
    return value;
}

uint32_t xop::readUint24LE(char* data)
{
    uint8_t* p = (uint8_t*)data;
    uint32_t value = (p[2] << 16) | (p[1] << 8) | p[0];
    return value;
}

uint16_t xop::readUint16BE(char* data)
{
    uint8_t* p = (uint8_t*)data;
    uint16_t value = (p[0] << 8) | p[1];
    return value; 
}

uint16_t xop::readUint16LE(char* data)
{
    uint8_t* p = (uint8_t*)data;
    uint16_t value = (p[1] << 8) | p[0];
    return value; 
}

const char BufferReader::kCRLF[] = "\r\n";

BufferReader::BufferReader(uint32_t initialSize)
    : _buffer(new std::vector<char>(initialSize))
{
	_buffer->resize(initialSize);
}	

BufferReader::~BufferReader()
{
	
}

int BufferReader::readFd(SOCKET sockfd)
{	
    uint32_t size = writableBytes();
    if(size < MAX_BYTES_PER_READ) // 重新调整BufferReader大小
    {
        uint32_t bufferReaderSize = (uint32_t)_buffer->size();
        if(bufferReaderSize > MAX_BUFFER_SIZE)
        {
            return 0; // close
        }
        
        _buffer->resize(bufferReaderSize + MAX_BYTES_PER_READ);
    }

    int bytesRead = ::recv(sockfd, beginWrite(), MAX_BYTES_PER_READ, 0);
    if(bytesRead > 0)
    {
        _writerIndex += bytesRead;
    }

    return bytesRead;
}


uint32_t BufferReader::readAll(std::string& data)
{
    uint32_t size = readableBytes();
    if(size > 0)
    {
        data.assign(peek(), size);
        _writerIndex = 0;
        _readerIndex = 0;
    }

    return size;
}

uint32_t BufferReader::readUntilCrlf(std::string& data)
{
    const char* crlf = findLastCrlf();
    if(crlf == nullptr)
    {
        return 0;
    }

    uint32_t size = (uint32_t)(crlf - peek() + 2);
    data.assign(peek(), size);
    retrieve(size);

    return size;
}

