#include "TcpServer.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include <cstdio>  

using namespace xop;
using namespace std;

TcpServer::TcpServer(EventLoop* eventLoop, std::string ip, uint16_t port)
	: _eventLoop(eventLoop),
	  _acceptor(new Acceptor(eventLoop, ip, port))
{
	_acceptor->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1));
	_acceptor->listen();
}

TcpServer::~TcpServer()
{
	
}

void TcpServer::newConnection(int sockfd)
{
	TcpConnectionPtr conn(new TcpConnection(_eventLoop, sockfd));
	_connections[sockfd] = conn;
	conn->setMessageCallback(_messageCallback);
	conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
}

void TcpServer::removeConnection(TcpConnectionPtr& conn)
{
	_eventLoop->addTriggerEvent(std::bind(&TcpServer::disconnect, this, conn->fd()));
}

void TcpServer::disconnect(int sockfd)
{
	_connections.erase(sockfd);
}



