#include "BufferReader.h"
#include "Socket.h"
#include <cstring>
 
using namespace xop;

const char BufferReader::kCRLF[] = "\r\n";

BufferReader::BufferReader(uint32_t initialSize)
	: _buffer(initialSize)
{
	
}	

BufferReader::~BufferReader()
{
	
}

int BufferReader::readFd(int sockfd)
{
	uint32_t size = writableBytes();
	if(size < MAX_BYTES_PER_READ) // 重新调整BufferReader大小
	{
		uint32_t BufferReaderSize = _buffer.size();
		if(BufferReaderSize > MAX_BUFFER_SIZE)
		{
			return 0; // close
		}
		_buffer.resize(BufferReaderSize + MAX_BYTES_PER_READ);
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
	
	uint32_t size = crlf - peek() + 2;
	data.assign(peek(), size);
	retrieve(size);
	
	return size;
}





