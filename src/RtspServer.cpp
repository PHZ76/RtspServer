// PHZ
// 2018-5-16

#include "RtspServer.h"
#include "RtspConnection.h"
#include "xop/log.h"
#include "xop/SocketUtil.h"

using namespace xop;
using namespace std;

RtspServer::RtspServer(EventLoop* loop, std::string ip, uint16_t port)
    : _loop(loop)
    , _acceptor(new Acceptor(loop, ip, port))
{
    _acceptor->setNewConnectionCallback([this](SOCKET sockfd) { this->newConnection(sockfd); });
    _acceptor->listen();
    
    if(_loop)
    {
        // RTSP 心跳检测
        _timerId = _loop->addTimer([this]() 
        {
            for(auto iter : _connections)
            {
                if(!iter.second->isAlive())
                {
                    iter.second->handleClose();
                    continue;
                }
                iter.second->resetAliveCount();          
            }
        }, 60*1000, true); 
    }   
}

RtspServer::~RtspServer()
{
	_loop->removeTimer(_timerId);
}

MediaSessionId RtspServer::addMeidaSession(MediaSession* session)
{
    std::lock_guard<std::mutex> locker(_mutex);

    if(_rtspSuffixMap.find(session->getRtspUrlSuffix()) != _rtspSuffixMap.end())
        return 0;

    std::shared_ptr<MediaSession> mediaSession(session); 
    MediaSessionId sessionId = mediaSession->getMediaSessionId();
    _rtspSuffixMap.emplace(std::move(mediaSession->getRtspUrlSuffix()), sessionId);
    _mediaSessions.emplace(sessionId, std::move(mediaSession));

    return sessionId;
}

void RtspServer::removeMeidaSession(MediaSessionId sessionId)
{
    std::lock_guard<std::mutex> locker(_mutex);

    auto iter = _mediaSessions.find(sessionId);
    if(iter != _mediaSessions.end())
    {
        _rtspSuffixMap.erase(iter->second->getRtspUrlSuffix());
        _mediaSessions.erase(sessionId);
    }
}

MediaSessionPtr RtspServer::lookMediaSession(const std::string& suffix)
{
    std::lock_guard<std::mutex> locker(_mutex);

    auto iter = _rtspSuffixMap.find(suffix);
    if(iter != _rtspSuffixMap.end())
    {
        MediaSessionId id = iter->second;
        return _mediaSessions[id];
    }

    return nullptr;
}

MediaSessionPtr RtspServer::lookMediaSession(MediaSessionId sessionId)
{
    std::lock_guard<std::mutex> locker(_mutex);

    auto iter = _mediaSessions.find(sessionId);
    if(iter != _mediaSessions.end())
    {
        return iter->second;
    }

    return nullptr;
}

bool RtspServer::pushFrame(MediaSessionId sessionId, MediaChannelId channelId, AVFrame frame)
{
    std::lock_guard<std::mutex> locker(_mutex);
    auto iter = _mediaSessions.find(sessionId);
    if(iter == _mediaSessions.end())
    {
        return false;
    }

    if(iter->second->getClientNum() != 0)
    {
        auto sessionPtr = iter->second;
        return _loop->addTriggerEvent([sessionPtr, channelId, frame]() { sessionPtr->handleFrame(channelId, frame); }); 		
    }

    return false;
}

void RtspServer::newConnection(SOCKET sockfd)
{
    std::shared_ptr<RtspConnection> rtspConn(new RtspConnection(this, _loop, sockfd));
    rtspConn->setCloseCallback([this](SOCKET sockfd) { this->removeConnection(sockfd); });
    _connections.emplace(sockfd, rtspConn);   
}

void RtspServer::removeConnection(SOCKET sockfd)
{
    if(!_loop->addTriggerEvent([sockfd, this]() { _connections.erase(sockfd); }))
    {
        _loop->addTimer([sockfd, this]() { _connections.erase(sockfd); }, 1, false);
    }
} 

