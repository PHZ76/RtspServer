#include "TcpConnection.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"
#include "SocketUtil.h"

using namespace xop;
using namespace std;

TcpConnection::TcpConnection(EventLoop *eventLoop, int sockfd)
	: _eventLoop(eventLoop)
	,  _channel(new Channel(sockfd))
	,  _readBuf(new BufferReader)
	, _writeBuf(new BufferWriter)
{
	_channel->setReadCallback(std::bind(&TcpConnection::handleRead, this));
	_channel->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
	_channel->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
	_channel->setErrorCallback(std::bind(&TcpConnection::handleError, this));
	
	SocketUtil::setKeepAlive(_channel->fd());
	SocketUtil::setNonBlock(_channel->fd());
	
	_channel->setEvents(EVENT_IN);
	_eventLoop->updateChannel(_channel);
}

TcpConnection::~TcpConnection()
{
	SocketUtil::close(_channel->fd());
}

void TcpConnection::handleRead()
{
	int ret = _readBuf->readFd(_channel->fd());
	if(ret > 0)
	{
		if(_messageCallback)
		{
			_messageCallback(shared_from_this());
		}
	}
	else if(ret <= 0)
	{
		handleClose();
	}
}

void TcpConnection::handleWrite()
{
 	int bytesSend = _writeBuf->send(_channel->fd());
	if(bytesSend < 0)
	{
		handleClose();
		return ;
	}
	else if(bytesSend == 0)
	{
		return ;
	}
	
	if(_writeBuf->isEmpty())
	{
		_channel->setEvents(EVENT_IN);
		_eventLoop->updateChannel(_channel);
	} 
}

void TcpConnection::handleClose()
{
	if(_state != kDisconnected)
	{
		_state = kDisconnected;
		_eventLoop->removeChannel(_channel);
		std::shared_ptr<TcpConnection> guardThis(shared_from_this());
		_closeCallback(guardThis);
	}
}

void TcpConnection::handleError()
{
	handleClose();
}

size_t TcpConnection::readableBytes() const 
{ 
	return _readBuf->readableBytes(); 
}


void TcpConnection::send(const void* message, int len)
{
	if (_state == kConnected)
	{
		int bytesSend = 0;

		if (_writeBuf->isEmpty())
		{
			int bytesSend = ::send(_channel->fd(), (const char*)message, len, 0);
			if (bytesSend == len)
				return;

			if (bytesSend > 0)
			{
				_writeBuf->append((const char*)message, len, bytesSend);
			}
			else if (bytesSend < 0)
			{
				handleClose();
			}
		}
		
		_writeBuf->append((const char*)message, len, bytesSend);

		if (!_writeBuf->isEmpty() && !_channel->isWriting())
		{
			_channel->setEvents(EVENT_IN | EVENT_OUT);
			_eventLoop->updateChannel(_channel);
		}
	}
}

void TcpConnection::shutdown()
{
	handleClose();
}

size_t TcpConnection::read(std::string& data)
{
	size_t size = 0;
	if(_readBuf->findLastCrlf() != nullptr)
	{
		size = _readBuf->readUntilCrlf(data);
	}
	else
	{
		size = _readBuf->readAll(data);
	}
	
	return size;
}


