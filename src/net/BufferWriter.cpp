// PHZ
// 2018-5-15

#include "BufferWriter.h"
#include "Socket.h"
#include "SocketUtil.h"

using namespace xop;

BufferWriter::BufferWriter(int capacity) 
    : _maxQueueLength(capacity)
	, _buffer(new std::queue<Packet>)
{
	
}	

bool BufferWriter::append(std::shared_ptr<char> data, uint32_t size, uint32_t index)
{
    if(size <= index)
        return false;

    if((int)_buffer->size() >= _maxQueueLength)
        return false;		

    Packet pkt = {data, size, index};
    _buffer->emplace(std::move(pkt));

    return true;
}

bool BufferWriter::append(const char* data, uint32_t size, uint32_t index)
{
    if(size <= index)
        return false;

    if((int)_buffer->size() >= _maxQueueLength)
        return false;		

    Packet pkt;
    pkt.data.reset(new char[size+512]);
    memcpy(pkt.data.get(), data, size);
    pkt.size = size;
    pkt.writeIndex = index;

    _buffer->emplace(std::move(pkt));

    return true;
}

int BufferWriter::send(int sockfd, int timeout)
{		
    if(timeout > 0)
        SocketUtil::setBlock(sockfd, timeout); // 超时返回-1

	int ret = 0;
	int count = 1;
	do
	{
		if (_buffer->empty())
			return 0;

		count -= 1;
		Packet &pkt = _buffer->front();
		ret = ::send(sockfd, pkt.data.get() + pkt.writeIndex, pkt.size - pkt.writeIndex, 0);
		if (ret > 0)
		{
			pkt.writeIndex += ret;
			if (pkt.size == pkt.writeIndex)
			{
				count += 1;
				_buffer->pop();
			}
		}
		else if (ret < 0)
		{
#if defined(__linux) || defined(__linux__)
			if (errno == EINTR || errno == EAGAIN)
#elif defined(WIN32) || defined(_WIN32)
			int error = WSAGetLastError();
			if (error == WSAEWOULDBLOCK || error == WSAEINPROGRESS || error == 0)
#endif
				ret = 0;
		}
	} while (count>0);

    if(timeout > 0)
        SocketUtil::setNonBlock(sockfd);

    return ret;
}


