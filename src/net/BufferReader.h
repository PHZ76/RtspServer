// PHZ
// 2018-5-15

#ifndef XOP_BUFFER_READER_H
#define XOP_BUFFER_READER_H

#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>  
#include <memory>  
#include "Socket.h"

namespace xop
{

uint32_t readUint32BE(char* data);
uint32_t readUint32LE(char* data);
uint32_t readUint24BE(char* data);
uint32_t readUint24LE(char* data);
uint16_t readUint16BE(char* data);
uint16_t readUint16LE(char* data);
    
class BufferReader
{
public:	
	static const uint32_t kInitialSize = 2048;
    BufferReader(uint32_t initialSize = kInitialSize);
    ~BufferReader();

    uint32_t readableBytes() const
    { return (uint32_t)(_writerIndex - _readerIndex); }

    uint32_t writableBytes() const
    {  return (uint32_t)(_buffer->size() - _writerIndex); }

    char* peek() 
    { return begin() + _readerIndex; }

    const char* peek() const
    { return begin() + _readerIndex; }

    const char* findFirstCrlf() const
    {    
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    const char* findLastCrlf() const
    {    
        const char* crlf = std::find_end(peek(), beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

	const char* findLastCrlfCrlf() const
	{
		char crlfCrlf[] = "\r\n\r\n";
		const char* crlf = std::find_end(peek(), beginWrite(), crlfCrlf, crlfCrlf + 4);
		return crlf == beginWrite() ? nullptr : crlf;
	}

    void retrieveAll() 
    { 
        _writerIndex = 0; 
        _readerIndex = 0; 
    }

    void retrieve(size_t len)
    {
        if (len <= readableBytes())
        {
            _readerIndex += len;
            if(_readerIndex == _writerIndex)
            {
                _readerIndex = 0;
                _writerIndex = 0;
            }
        }
        else
        {
            retrieveAll();
        }
    }

    void retrieveUntil(const char* end)
    { retrieve(end - peek()); }

    int readFd(SOCKET sockfd);
    uint32_t readAll(std::string& data);
    uint32_t readUntilCrlf(std::string& data);

    uint32_t bufferSize() const 
    { return (uint32_t)_buffer->size(); }

private:
    char* begin()
    { return &*_buffer->begin(); }

    const char* begin() const
    { return &*_buffer->begin(); }

    char* beginWrite()
    { return begin() + _writerIndex; }

    const char* beginWrite() const
    { return begin() + _writerIndex; }

    std::shared_ptr<std::vector<char>> _buffer;
    size_t _readerIndex = 0;
    size_t _writerIndex = 0;

    static const char kCRLF[];
	static const uint32_t MAX_BYTES_PER_READ = 4096;
	static const uint32_t MAX_BUFFER_SIZE = 1024 * 100000;
};

}

#endif


