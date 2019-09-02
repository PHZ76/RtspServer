#ifndef XOP_TCP_CONNECTION_H
#define XOP_TCP_CONNECTION_H

#include <atomic>
#include <mutex>
#include "TaskScheduler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "Channel.h"

namespace xop
{

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    using Ptr = std::shared_ptr<TcpConnection>;
    using DisconnectCallback = std::function<void(std::shared_ptr<TcpConnection> conn)> ;
    using CloseCallback = std::function<void(std::shared_ptr<TcpConnection> conn)>;
    using ReadCallback = std::function<bool(std::shared_ptr<TcpConnection> conn, xop::BufferReader& buffer)>;

    TcpConnection(TaskScheduler *taskScheduler, SOCKET sockfd);
    virtual ~TcpConnection();

    TaskScheduler* getTaskScheduler() const 
    { return _taskScheduler; }

    void setReadCallback(const ReadCallback& cb)
    { _readCB = cb; }

    void setCloseCallback(const CloseCallback& cb)
    { _closeCB = cb; }

    void send(std::shared_ptr<char> data, uint32_t size);
    void send(const char *data, uint32_t size);
    
	void disconnect();

    bool isClosed() const 
    { return _isClosed; }

    SOCKET fd() const 
    { return _channelPtr->fd(); }

protected:
    friend class TcpServer;

    virtual void handleRead();
    virtual void handleWrite();
    virtual void handleClose();
    virtual void handleError();	

    void setDisconnectCallback(const DisconnectCallback& cb)
    { _disconnectCB = cb; }

	TaskScheduler *_taskScheduler;
	std::shared_ptr<xop::BufferReader> _readBufferPtr;
	std::shared_ptr<xop::BufferWriter> _writeBufferPtr;
	std::atomic_bool _isClosed;

private:
	void close();

    std::shared_ptr<xop::Channel> _channelPtr;
    std::mutex _mutex;
    DisconnectCallback _disconnectCB ;
    CloseCallback _closeCB;
    ReadCallback _readCB;
};

}

#endif 
