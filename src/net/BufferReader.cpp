// PHZ
// 2018-5-15

#include "BufferReader.h"
#include "Socket.h"
#include <cstring>
 
using namespace xop;
uint32_t xop::ReadUint32BE(char* data)
{
	uint8_t* p = (uint8_t*)data;
	uint32_t value = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	return value;
}

uint32_t xop::ReadUint32LE(char* data)
{
	uint8_t* p = (uint8_t*)data;
	uint32_t value = (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
	return value;
}

uint32_t xop::ReadUint24BE(char* data)
{
	uint8_t* p = (uint8_t*)data;
	uint32_t value = (p[0] << 16) | (p[1] << 8) | p[2];
	return value;
}

uint32_t xop::ReadUint24LE(char* data)
{
	uint8_t* p = (uint8_t*)data;
	uint32_t value = (p[2] << 16) | (p[1] << 8) | p[0];
	return value;
}

uint16_t xop::ReadUint16BE(char* data)
{
	uint8_t* p = (uint8_t*)data;
	uint16_t value = (p[0] << 8) | p[1];
	return value; 
}

uint16_t xop::ReadUint16LE(char* data)
{
	uint8_t* p = (uint8_t*)data;
	uint16_t value = (p[1] << 8) | p[0];
	return value; 
}

const char BufferReader::kCRLF[] = "\r\n";

BufferReader::BufferReader(uint32_t initial_size)
{
	buffer_.resize(initial_size);
}	

BufferReader::~BufferReader()
{
	
}

int BufferReader::Read(SOCKET sockfd)
{	
	uint32_t size = WritableBytes();
	if(size < MAX_BYTES_PER_READ) {
		uint32_t bufferReaderSize = (uint32_t)buffer_.size();
		if(bufferReaderSize > MAX_BUFFER_SIZE) {
			return 0; 
		}
        
		buffer_.resize(bufferReaderSize + MAX_BYTES_PER_READ);
	}

	int bytes_read = ::recv(sockfd, beginWrite(), MAX_BYTES_PER_READ, 0);
	if(bytes_read > 0) {
		writer_index_ += bytes_read;
	}

	return bytes_read;
}


uint32_t BufferReader::ReadAll(std::string& data)
{
	uint32_t size = ReadableBytes();
	if(size > 0)  {
		data.assign(Peek(), size);
		writer_index_ = 0;
		reader_index_ = 0;
	}

	return size;
}

uint32_t BufferReader::ReadUntilCrlf(std::string& data)
{
	const char* crlf = FindLastCrlf();
	if(crlf == nullptr)  {
		return 0;
	}

	uint32_t size = (uint32_t)(crlf - Peek() + 2);
	data.assign(Peek(), size);
	Retrieve(size);
	return size;
}

