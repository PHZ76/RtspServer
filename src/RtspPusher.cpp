#if defined(WIN32) || defined(_WIN32) 
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "RtspPusher.h"
#include "RtspConnection.h"
#include "xop/TcpSocket.h"
#include "xop/log.h"

using namespace xop;

RtspPusher::RtspPusher(EventLoop* loop)
	: _loop(loop)
{

}

RtspPusher::~RtspPusher()
{
	
}

MediaSessionId RtspPusher::addMeidaSession(MediaSession* session)
{
	std::lock_guard<std::mutex> locker(_mutex);

	if (_mediaSessions)
		return 0;

	_mediaSessions.reset(session);

	return _mediaSessions->getMediaSessionId();
}

void RtspPusher::removeMeidaSession(MediaSessionId sessionId)
{
	std::lock_guard<std::mutex> locker(_mutex);
	_mediaSessions = nullptr;
}

bool RtspPusher::openUrl(std::string url)
{
	std::lock_guard<std::mutex> locker(_mutex);
	if (parseRtspUrl(url))
	{
		if (_rtspConnection)
		{
			return false;
		}

		TcpSocket tcpSocket;
		tcpSocket.create();
		if (!tcpSocket.connect(_rtspInfo.ip, _rtspInfo.port))
		{
			tcpSocket.close();
			return false;
		}

		newConnection(tcpSocket.fd());		
		_rtspConnection->sendOptions();

		return true; 
	}

	return false;
}

void RtspPusher::close()
{
	std::lock_guard<std::mutex> locker(_mutex);
	if (_rtspConnection)
	{
		removeConnection(_rtspConnection->fd());
	}
}

bool RtspPusher::isConnected()
{
	return (_rtspConnection && _rtspConnection->isAlive());
}

bool RtspPusher::pushFrame(MediaSessionId sessionId, MediaChannelId channelId, AVFrame frame)
{
	std::lock_guard<std::mutex> locker(_mutex);
	
	if (_rtspConnection && _mediaSessions)
	{
		auto sessionPtr = _mediaSessions;
		return _loop->addTriggerEvent([sessionPtr, channelId, frame]() { sessionPtr->handleFrame(channelId, frame); });
	}
	
	return false;
}

MediaSessionPtr RtspPusher::lookMediaSession(const std::string& suffix)
{
	std::lock_guard<std::mutex> locker(_mutex);
	return _mediaSessions;
}

MediaSessionPtr RtspPusher::lookMediaSession(MediaSessionId sessionId)
{
	std::lock_guard<std::mutex> locker(_mutex);
	return _mediaSessions;
}

bool RtspPusher::parseRtspUrl(std::string& url)
{
	char ip[100] = { 0 };
	char suffix[100] = { 0 };
	uint16_t port = 0;

	if (sscanf(url.c_str() + 7, "%[^:]:%hu/%s", ip, &port, suffix) == 3)
	{
		_rtspInfo.port = port;
	}
	else if (sscanf(url.c_str() + 7, "%[^/]/%s", ip, suffix) == 2)
	{
		_rtspInfo.port = 554;
	}
	else
	{
		LOG("%s was illegal.\n", url.c_str());
		return false;
	}

	_rtspInfo.ip = ip;
	_rtspInfo.suffix = suffix;
	_rtspInfo.url = url;

	return true;
}

void RtspPusher::newConnection(SOCKET sockfd)
{
	_rtspConnection.reset(new RtspConnection(this, _loop, sockfd));		
	_rtspConnection->setCloseCallback([this](SOCKET sockfd) { this->removeConnection(sockfd); });
}

void RtspPusher::removeConnection(SOCKET sockfd)
{
	if (!_loop->addTriggerEvent([sockfd, this]() { if(this->_rtspConnection && this->_rtspConnection->fd() == sockfd) this->_rtspConnection = nullptr; }))
	{
		_loop->addTimer([sockfd, this]() { if (this->_rtspConnection && this->_rtspConnection->fd() == sockfd) this->_rtspConnection = nullptr; }, 1, false);
	}
}