#ifndef XOP_RTSP_SERVER_H
#define XOP_RTSP_SERVER_H

#include <memory>
#include <unordered_map>
#include <string>
#include <mutex>
#include <atomic>
#include "xop/Acceptor.h"
#include "xop/EventLoop.h"
#include "xop/Socket.h"
#include "MediaSession.h"
#include "media.h"

namespace xop
{

class RtspConnection;

class RtspServer
{
public:	
    RtspServer() = delete;	
    RtspServer(xop::EventLoop *loop, std::string ip, uint16_t port=554);
    ~RtspServer();  

    MediaSessionId addMeidaSession(MediaSession* session);
    void removeMeidaSession(MediaSessionId sessionId);	

    bool pushFrame(MediaSessionId sessionId, MediaChannelId channelId, AVFrame frame);
	
private:
    friend class RtspConnection;

    MediaSessionPtr lookMediaSession(const std::string& suffix);
    MediaSessionPtr lookMediaSession(MediaSessionId sessionId);

    void newConnection(SOCKET sockfd);
    void removeConnection(std::shared_ptr<RtspConnection> rtspConnPtr);

    xop::EventLoop *_loop; 
    std::shared_ptr<xop::Acceptor> _acceptor; 
    std::unordered_map<SOCKET, std::shared_ptr<RtspConnection>> _connections;

    mutable std::mutex _mutex;
    //MediaSessionId _lastMediaSessionId = 1;
    std::atomic_uint _lastMediaSessionId;
    std::unordered_map<MediaSessionId, std::shared_ptr<MediaSession>> _mediaSessions; 
    std::unordered_map<std::string, MediaSessionId> _rtspSuffixMap;
};

}

#endif


