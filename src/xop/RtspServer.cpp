// PHZ
// 2018-5-16

#include "RtspServer.h"
#include "RtspConnection.h"
#include "net/SocketUtil.h"
#include "net/Logger.h"

#define KEEP_ALIVE_ON 0

using namespace xop;
using namespace std;

RtspServer::RtspServer(EventLoop* loop, std::string ip, uint16_t port)
	: TcpServer(loop, ip, port)
{
#if KEEP_ALIVE_ON    
    loop->addTimer([this]() {
        std::lock_guard<std::mutex> locker(_conn_mutex);
        for (auto iter : _connections)
        {
            auto rtspConn = dynamic_pointer_cast<RtspConnection>(iter.second);
            if (!rtspConn->isAlive())
            {
                rtspConn->handleClose();
                continue;
            }
            rtspConn->resetAliveCount();
        }
        return true;
    }, 30 * 1000);
#endif

    if (this->start() != 0)
    {
        LOG_INFO("RTSP Server listening on %d failed.", port);
    }
}

RtspServer::~RtspServer()
{
	
}

MediaSessionId RtspServer::addMeidaSession(MediaSession* session)
{
    std::lock_guard<std::mutex> locker(_mtxSessionMap);

    if (_rtspSuffixMap.find(session->getRtspUrlSuffix()) != _rtspSuffixMap.end())
    {
        return 0;
    }

    std::shared_ptr<MediaSession> mediaSession(session); 
    MediaSessionId sessionId = mediaSession->getMediaSessionId();
    _rtspSuffixMap.emplace(std::move(mediaSession->getRtspUrlSuffix()), sessionId);
    _mediaSessions.emplace(sessionId, std::move(mediaSession));

    return sessionId;
}

void RtspServer::removeMeidaSession(MediaSessionId sessionId)
{
    std::lock_guard<std::mutex> locker(_mtxSessionMap);

    auto iter = _mediaSessions.find(sessionId);
    if(iter != _mediaSessions.end())
    {
        _rtspSuffixMap.erase(iter->second->getRtspUrlSuffix());
        _mediaSessions.erase(sessionId);
    }
}

MediaSessionPtr RtspServer::lookMediaSession(const std::string& suffix)
{
    std::lock_guard<std::mutex> locker(_mtxSessionMap);

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
    std::lock_guard<std::mutex> locker(_mtxSessionMap);

    auto iter = _mediaSessions.find(sessionId);
    if(iter != _mediaSessions.end())
    {
        return iter->second;
    }

    return nullptr;
}

bool RtspServer::pushFrame(MediaSessionId sessionId, MediaChannelId channelId, AVFrame frame)
{
    std::shared_ptr<MediaSession> sessionPtr = nullptr;

    {
        std::lock_guard<std::mutex> locker(_mtxSessionMap);
        auto iter = _mediaSessions.find(sessionId);
        if (iter != _mediaSessions.end())
        {
            sessionPtr = iter->second;
        }
        else
        {
            return false;
        }
    }

    if (sessionPtr!=nullptr && sessionPtr->getNumClient()!=0)
    {
        return sessionPtr->handleFrame(channelId, frame);
    }

    return false;
}

TcpConnection::Ptr RtspServer::newConnection(SOCKET sockfd)
{	
	return std::make_shared<RtspConnection>(this, _eventLoop->getTaskScheduler().get(), sockfd);
}

