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
        TcpSocket tcpSocket;
        tcpSocket.create();
        if (!tcpSocket.connect(_rtspInfo.ip, _rtspInfo.port, 3000)) // 3s timeout
        {
            tcpSocket.close();
            return false;
        }

        SOCKET sockfd = tcpSocket.fd();
        _loop->addTriggerEvent([sockfd, this]() 
        { 
            std::shared_ptr<RtspConnection> rtspConn = this->newConnection(sockfd);		
            rtspConn->sendOptions(); 
        });                

        return true; 
    }

    return false;
}

void RtspPusher::close()
{
    std::lock_guard<std::mutex> locker(_mutex);
    for(auto iter : _connections)
    {
        auto ptr = iter.second;
        _loop->addTriggerEvent([ptr]() { ptr->handleClose(); });
    }
}

bool RtspPusher::pushFrame(MediaSessionId sessionId, MediaChannelId channelId, AVFrame frame)
{
    std::lock_guard<std::mutex> locker(_mutex);
    if (_connections.size()>0 && _mediaSessions)
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

std::shared_ptr<RtspConnection> RtspPusher::newConnection(SOCKET sockfd)
{
    std::shared_ptr<RtspConnection> rtspConn(new RtspConnection(this, _loop, sockfd));
    rtspConn->setCloseCallback([this](SOCKET sockfd) { this->removeConnection(sockfd); });
    _connections.emplace(sockfd, rtspConn); 
    return rtspConn;
}

void RtspPusher::removeConnection(SOCKET sockfd)
{
    if(!_loop->addTriggerEvent([sockfd, this]() { _connections.erase(sockfd); }))
    {
        _loop->addTimer([sockfd, this]() { _connections.erase(sockfd); }, 1, false);
    }
}
