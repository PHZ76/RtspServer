#include "RtspPusher.h"
#include "RtspConnection.h"
#include "net/Logger.h"
#include "net/TcpSocket.h"
#include "net/Timestamp.h"

using namespace xop;

RtspPusher::RtspPusher(xop::EventLoop *eventLoop)
	: _eventLoop(eventLoop)
{

}

RtspPusher::~RtspPusher()
{
	
}

void RtspPusher::addMeidaSession(MediaSession* session)
{
    std::lock_guard<std::mutex> locker(_mutex);
    _mediaSessionPtr.reset(session);
}

void RtspPusher::removeMeidaSession(MediaSessionId sessionId)
{
    std::lock_guard<std::mutex> locker(_mutex);
    _mediaSessionPtr = nullptr;
}

MediaSessionPtr RtspPusher::lookMediaSession(MediaSessionId sessionId)
{
    return _mediaSessionPtr;
}

int RtspPusher::openUrl(std::string url, int msec)
{
    std::lock_guard<std::mutex> lock(_mutex);

	static xop::Timestamp tp;
	int timeout = msec;
	if (timeout <= 0)
	{
		timeout = 10000;
	}

	tp.reset();

    if (!this->parseRtspUrl(url))
    {
        LOG_ERROR("rtsp url(%s) was illegal.\n", url.c_str());
        return -1;
    }

	if (_rtspConn != nullptr)
	{
		std::shared_ptr<RtspConnection> rtspConn = _rtspConn;
		SOCKET sockfd = rtspConn->fd();
		_taskScheduler->addTriggerEvent([sockfd, rtspConn]() {
			rtspConn->disconnect();
		});
		_rtspConn = nullptr;
	}

    TcpSocket tcpSocket;
    tcpSocket.create();
    if (!tcpSocket.connect(_rtspUrlInfo.ip, _rtspUrlInfo.port, timeout)) 
    {
        tcpSocket.close();
        return -1;
    }

	_taskScheduler = _eventLoop->getTaskScheduler().get();
	_rtspConn.reset(new RtspConnection((RtspPusher*)this, _taskScheduler, tcpSocket.fd()));
    _eventLoop->addTriggerEvent([this]() {
		_rtspConn->sendOptions(RtspConnection::RTSP_PUSHER);
    });

	timeout -= (int)tp.elapsed();
	if (timeout < 0)
	{
		timeout = 1000;
	}

	do
	{
		xop::Timer::sleep(100);
		timeout -= 100;
	} while (!_rtspConn->isRecord() && timeout > 0);

	if (!_rtspConn->isRecord())
	{
		std::shared_ptr<RtspConnection> rtspConn = _rtspConn;
		SOCKET sockfd = rtspConn->fd();
		_taskScheduler->addTriggerEvent([sockfd, rtspConn]() {
			rtspConn->disconnect();
		});
		_rtspConn = nullptr;
		return -1;
	}

    return 0;
}

void RtspPusher::close()
{
	std::lock_guard<std::mutex> lock(_mutex);

	if (_rtspConn != nullptr)
	{
		std::shared_ptr<RtspConnection> rtspConn = _rtspConn;
		SOCKET sockfd = rtspConn->fd();
		_taskScheduler->addTriggerEvent([sockfd, rtspConn]() {
			rtspConn->disconnect();
		});
		_rtspConn = nullptr;
	}
}

bool RtspPusher::isConnected()
{
	std::lock_guard<std::mutex> lock(_mutex);
	if (_rtspConn != nullptr)
	{
		return (!_rtspConn->isClosed());
	}
	return false;
}

bool RtspPusher::pushFrame(MediaChannelId channelId, AVFrame frame)
{
    std::lock_guard<std::mutex> locker(_mutex);
    if (_mediaSessionPtr==nullptr || _rtspConn==nullptr)
    {
        return false;
    }

    return _mediaSessionPtr->handleFrame(channelId, frame);
}