// PHZ
// 2018-5-15

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
    _acceptor->setNewConnectionCallback([this](SOCKET sockfd) { this->newConnection(sockfd); });
    _acceptor->listen();
}

TcpServer::~TcpServer()
{
	
}

void TcpServer::newConnection(SOCKET sockfd)
{
    TcpConnectionPtr conn(new TcpConnection(_eventLoop, sockfd));
    _connections[sockfd] = conn;
    conn->setMessageCallback(_messageCallback);
    conn->setCloseCallback([this](TcpConnectionPtr& conn) { this->removeConnection(conn); });
}

void TcpServer::removeConnection(TcpConnectionPtr& conn)
{
    SOCKET sockfd = conn->fd();
    _eventLoop->addTriggerEvent([sockfd, this]()
    {
        this->_connections.erase(sockfd);
    });
}




