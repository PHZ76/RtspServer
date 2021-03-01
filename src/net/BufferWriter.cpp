// PHZ
// 2018-5-15

#include "BufferWriter.h"
#include "Socket.h"
#include "SocketUtil.h"

using namespace xop;

void xop::WriteUint32BE(char* p, uint32_t value)
{
	p[0] = value >> 24;
	p[1] = value >> 16;
	p[2] = value >> 8;
	p[3] = value & 0xff;
}

void xop::WriteUint32LE(char* p, uint32_t value)
{
	p[0] = value & 0xff;
	p[1] = value >> 8;
	p[2] = value >> 16;
	p[3] = value >> 24;
}

void xop::WriteUint24BE(char* p, uint32_t value)
{
	p[0] = value >> 16;
	p[1] = value >> 8;
	p[2] = value & 0xff;
}

void xop::WriteUint24LE(char* p, uint32_t value)
{
	p[0] = value & 0xff;
	p[1] = value >> 8;
	p[2] = value >> 16;
}

void xop::WriteUint16BE(char* p, uint16_t value)
{
	p[0] = value >> 8;
	p[1] = value & 0xff;
}

void xop::WriteUint16LE(char* p, uint16_t value)
{
	p[0] = value & 0xff;
	p[1] = value >> 8;
}

BufferWriter::BufferWriter(int capacity) 
	: max_queue_length_(capacity)
{
	
}	

bool BufferWriter::Append(std::shared_ptr<char> data, uint32_t size, uint32_t index)
{
	if (size <= index) {
		return false;
	}
   
	if ((int)buffer_.size() >= max_queue_length_) {
		return false;
	}
     
	Packet pkt = { data, size, index };
	buffer_.emplace(std::move(pkt));
	return true;
}

bool BufferWriter::Append(const char* data, uint32_t size, uint32_t index)
{
	if (size <= index) {
		return false;
	}
     
	if ((int)buffer_.size() >= max_queue_length_) {
		return false;
	}
     
	Packet pkt;
	pkt.data.reset(new char[size+512]);
	memcpy(pkt.data.get(), data, size);
	pkt.size = size;
	pkt.writeIndex = index;
	buffer_.emplace(std::move(pkt));
	return true;
}

int BufferWriter::Send(SOCKET sockfd, int timeout)
{		
	if (timeout > 0) {
		SocketUtil::SetBlock(sockfd, timeout); 
	}
      
	int ret = 0;
	int count = 1;

	do
	{
		if (buffer_.empty()) {
			return 0;
		}
		
		count -= 1;
		Packet &pkt = buffer_.front();
		ret = ::send(sockfd, pkt.data.get() + pkt.writeIndex, pkt.size - pkt.writeIndex, 0);
		if (ret > 0) {
			pkt.writeIndex += ret;
			if (pkt.size == pkt.writeIndex) {
				count += 1;
				buffer_.pop();
			}
		}
		else if (ret < 0) {
#if defined(__linux) || defined(__linux__)
		if (errno == EINTR || errno == EAGAIN) 
#elif defined(WIN32) || defined(_WIN32)
			int error = WSAGetLastError();
			if (error == WSAEWOULDBLOCK || error == WSAEINPROGRESS || error == 0)
#endif
			{
				ret = 0;
			}
		}
	} while (count > 0);

	if (timeout > 0) {
		SocketUtil::SetNonBlock(sockfd);
	}
    
	return ret;
}


