#include "Acceptor.h"
#include "EventLoop.h"
#include "SocketUtil.h"
#include "log.h"

using namespace xop;

Acceptor::Acceptor(EventLoop* eventLoop, std::string ip, uint16_t port)
	: _eventLoop(eventLoop)
{	
	_tcpSocket.reset(new TcpSocket(socket(AF_INET, SOCK_STREAM, 0)));
	_acceptChannel.reset(new Channel(_tcpSocket->fd()));
	SocketUtil::setReuseAddr(_tcpSocket->fd());
	SocketUtil::setReusePort(_tcpSocket->fd());
	//SocketUtil::setNonBlock(_tcpSocket->fd());
	_tcpSocket->bind(ip, port);
}

Acceptor::~Acceptor()
{
	_eventLoop->removeChannel(_acceptChannel);
	_tcpSocket->close();
}

void Acceptor::listen()
{
	_tcpSocket->listen(1024);
	
	_acceptChannel->setEvents(EVENT_IN);
	_acceptChannel->setReadCallback(std::bind(&Acceptor::handleAccept, this));
	_eventLoop->updateChannel(_acceptChannel);
}

void Acceptor::handleAccept()
{
	int connfd = _tcpSocket->accept();
	if (connfd > 0)
	{
		if (_newConnectionCallback)		
		{
			_newConnectionCallback(connfd);
		}
	}
}

