// PHZ
// 2018-6-8

#ifndef XOP_RTSP_SERVER_H
#define XOP_RTSP_SERVER_H

#include <memory>
#include <unordered_map>
#include <string>
#include <mutex>
#include "rtsp.h"

namespace xop
{

class RtspConnection;

class RtspServer : public RTSP
{
public:	
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
    void removeConnection(SOCKET sockfd);

    xop::EventLoop *_loop = nullptr; 
    std::shared_ptr<xop::Acceptor> _acceptor; 
    std::unordered_map<SOCKET, std::shared_ptr<RtspConnection>> _connections;

    mutable std::mutex _mutex;
    std::unordered_map<MediaSessionId, std::shared_ptr<MediaSession>> _mediaSessions; 
    std::unordered_map<std::string, MediaSessionId> _rtspSuffixMap;

    TimerId _timerId;  
};

}

#endif


