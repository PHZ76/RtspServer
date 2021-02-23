// PHZ
// 2018-11-10

#ifndef XOP_TCPSERVER_H
#define XOP_TCPSERVER_H

#include <memory>
#include <string>
#include <mutex>
#include <unordered_map>
#include "Acceptor.h"
#include "TcpConnection.h"

namespace xop
{

class EventLoop;

class TcpServer
{
public:	
	TcpServer(EventLoop* event_loop);
	virtual ~TcpServer();  

	virtual bool Start(std::string ip, uint16_t port);
	virtual void Stop();

	const std::string& GetIPAddress() const
	{ return ip_; }

	uint16_t GetPort() const 
	{ return port_; }

protected:
	virtual TcpConnection::Ptr OnConnect(SOCKET sockfd, std::string ip, int port);
	virtual void AddConnection(SOCKET sockfd, TcpConnection::Ptr tcpConn);
	virtual void RemoveConnection(SOCKET sockfd);

	EventLoop* event_loop_;
	uint16_t port_;
	std::string ip_;
	Acceptor acceptor_; 
	bool is_started_;
	std::mutex mutex_;
	std::unordered_map<SOCKET, std::shared_ptr<TcpConnection>> connections_;
};

}

#endif 
