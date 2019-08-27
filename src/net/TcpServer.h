// PHZ
// 2018-11-10

#ifndef XOP_TCPSERVER_H
#define XOP_TCPSERVER_H

#include <memory>
#include <string>
#include <mutex>
#include <unordered_map>
#include "Socket.h"
#include "TcpConnection.h"

namespace xop
{

class Acceptor;
class EventLoop;

class TcpServer 
{
public:	
    TcpServer(EventLoop* loop, std::string ip, uint16_t port);
    virtual ~TcpServer();  
    int start();

    std::string getIPAddress() const
    { return _ip; }

    uint16_t getPort() const 
    { return _port; }

protected:
    virtual TcpConnection::Ptr newConnection(SOCKET sockfd);
    void addConnection(SOCKET sockfd, TcpConnection::Ptr tcpConn);
    void removeConnection(SOCKET sockfd);

    EventLoop* _eventLoop; 
    uint16_t _port;
    std::string _ip;
    std::shared_ptr<Acceptor> _acceptor; 
    std::mutex _conn_mutex;
    std::unordered_map<SOCKET, std::shared_ptr<TcpConnection>> _connections;
};

}

#endif 
