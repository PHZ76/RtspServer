#include "RtspServer.h"
#include "RtspConnection.h"
#include "xop/log.h"
#include "xop/SocketUtil.h"

using namespace xop;
using namespace std;

RtspServer::RtspServer(EventLoop* loop, std::string ip, uint16_t port)
    : _loop(loop)
    , _acceptor(new Acceptor(loop, ip, port))
    , _lastMediaSessionId(1)
{
    _acceptor->setNewConnectionCallback(std::bind(&RtspServer::newConnection, this, placeholders::_1));
    _acceptor->listen();
}

RtspServer::~RtspServer()
{
	
}

MediaSessionId RtspServer::addMeidaSession(MediaSession* session)
{
    std::lock_guard<std::mutex> locker(_mutex);

    if(_rtspSuffixMap.find(session->getRtspUrlSuffix()) != _rtspSuffixMap.end())
        return 0;

    MediaSessionId sessionId = _lastMediaSessionId++;
    while(_mediaSessions.find(sessionId) != _mediaSessions.end())
    {
        sessionId = _lastMediaSessionId++;
    }

    std::shared_ptr<MediaSession> mediaSession(session); 
    mediaSession->setMediaSessionId(sessionId);
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
        //if(sessionPtr->saveFrame(channelId, frame))
        {
            return _loop->addTriggerEvent([sessionPtr, channelId, frame]() 
            { 
                sessionPtr->handleFrame(channelId, frame); 
            }); 
        }		
    }

    return false;
}

void RtspServer::newConnection(SOCKET sockfd)
{
    std::shared_ptr<RtspConnection> conn(new RtspConnection(this, sockfd));
    _connections[sockfd] = conn;
    conn->setCloseCallback(std::bind(&RtspServer::removeConnection, this, placeholders::_1));
}

void RtspServer::removeConnection(std::shared_ptr<RtspConnection> rtspConnPtr)
{
    _loop->addTriggerEvent([rtspConnPtr, this]() 
    {
        SOCKET sockfd = rtspConnPtr->fd();
        if(rtspConnPtr->getMediaSessionId() != 0)
        {
            std::lock_guard<std::mutex> locker(_mutex);
            
            //从媒体会话删除连接
            auto iter = _mediaSessions.find(rtspConnPtr->getMediaSessionId());
            if(iter != _mediaSessions.end())
            {
                iter->second->removeClient(sockfd);
            }
        }

        _connections.erase(sockfd);
        SocketUtil::close(sockfd);
    });
} 

