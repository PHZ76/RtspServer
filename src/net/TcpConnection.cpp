#include "TcpConnection.h"
#include "SocketUtil.h"

using namespace xop;

TcpConnection::TcpConnection(TaskScheduler *taskScheduler, SOCKET sockfd)
	: _taskScheduler(taskScheduler)
	, _readBufferPtr(new BufferReader)
	, _writeBufferPtr(new BufferWriter(500))
	, _channelPtr(new Channel(sockfd))
{
    _isClosed = false;

    _channelPtr->setReadCallback([this]() { this->handleRead(); });
    _channelPtr->setWriteCallback([this]() { this->handleWrite(); });
    _channelPtr->setCloseCallback([this]() { this->handleClose(); });
    _channelPtr->setErrorCallback([this]() { this->handleError(); });

    SocketUtil::setNonBlock(sockfd);
    SocketUtil::setSendBufSize(sockfd, 100 * 1024);
    SocketUtil::setKeepAlive(sockfd);

    _channelPtr->enableReading();
    _taskScheduler->updateChannel(_channelPtr);
}

TcpConnection::~TcpConnection()
{
	SOCKET fd = _channelPtr->fd();
    if (fd > 0)
    {
        SocketUtil::close(fd);
    }
}

void TcpConnection::send(std::shared_ptr<char> data, uint32_t size)
{
	if (_isClosed)
		return;

	{
		std::lock_guard<std::mutex> lock(_mutex);
		_writeBufferPtr->append(data, size);
	}

    this->handleWrite();
    return;
}

void TcpConnection::send(const char *data, uint32_t size)
{
	if (_isClosed)
		return;

	{
		std::lock_guard<std::mutex> lock(_mutex);
		_writeBufferPtr->append(data, size);
	}

    this->handleWrite();
    return;
}

void TcpConnection::disconnect()
{
	std::lock_guard<std::mutex> lock(_mutex);
	this->close();
}

void TcpConnection::handleRead()
{
	{
		std::lock_guard<std::mutex> lock(_mutex);

		if (_isClosed)
			return;

		int ret = _readBufferPtr->readFd(_channelPtr->fd());
		if (ret <= 0)
		{
			this->close();
			return;
		}
	}

    if (_readCB)
    {
        bool ret = _readCB(shared_from_this(), *_readBufferPtr);
        if (false == ret)
        {
			std::lock_guard<std::mutex> lock(_mutex);
			this->close();
        }
    }
}

void TcpConnection::handleWrite()
{
	if (_isClosed)
		return;

	std::lock_guard<std::mutex> lock(_mutex);

    int ret = 0;
    bool empty = false;
    do
    {
        ret = _writeBufferPtr->send(_channelPtr->fd());
        if (ret < 0)
        {
			this->close();
            return;
        }
        empty = _writeBufferPtr->isEmpty();
    } while (0);

    if (empty)
    {
        if (_channelPtr->isWriting())
        {
            _channelPtr->disableWriting();
            _taskScheduler->updateChannel(_channelPtr);
        }
    }
    else if(!_channelPtr->isWriting())
    {
        _channelPtr->enableWriting();
        _taskScheduler->updateChannel(_channelPtr);
    }
}

void TcpConnection::close()
{
	if (!_isClosed)
	{
		_isClosed = true;
		_taskScheduler->removeChannel(_channelPtr);

		if (_closeCB)
			_closeCB(shared_from_this());

		if (_disconnectCB)
			_disconnectCB(shared_from_this());
	}
}

void TcpConnection::handleClose()
{
    std::lock_guard<std::mutex> lock(_mutex);
	this->close();
}

void TcpConnection::handleError()
{
	std::lock_guard<std::mutex> lock(_mutex);
	this->close();
}
