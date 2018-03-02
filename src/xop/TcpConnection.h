#ifndef XOP_TCP_CONNECTION_H
#define XOP_TCP_CONNECTION_H

#include <iostream>
#include <functional>
#include <memory>
#include "BufferReader.h"
#include "BufferWriter.h"
#include "Channel.h"

namespace xop
{

class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::function<void (const TcpConnectionPtr&)> MessageCallback;
typedef std::function<void (TcpConnectionPtr& conn)> CloseCallback;

class EventLoop;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
	TcpConnection()	= delete;
	TcpConnection(EventLoop *eventLoop, int sockfd);
	virtual ~TcpConnection();
	
	int fd() const { return _channel->fd(); }
	
	void setCloseCallback(const CloseCallback& cb)
	{ _closeCallback = cb; }
	
	void setMessageCallback(const MessageCallback& cb)
	{ _messageCallback = cb; }
	
	void send(const void* message, int len);
	size_t readableBytes() const;
	size_t read(std::string& data);
	
	void shutdown();
	
private:
	void handleRead();
	void handleWrite();
	void handleClose();
	void handleError();
	
	EventLoop *_eventLoop = nullptr;
	std::shared_ptr<Channel> _channel;
	
	enum State { kDisconnected, kConnected };
	State _state = kConnected;
	
	CloseCallback _closeCallback;
	MessageCallback _messageCallback;
	
	std::shared_ptr<BufferReader> _readBuf;
	std::shared_ptr<BufferWriter> _writeBuf;
};
	
}

#endif

