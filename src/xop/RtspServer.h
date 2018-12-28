// PHZ
// 2018-6-8

#ifndef XOP_RTSP_SERVER_H
#define XOP_RTSP_SERVER_H

#include <memory>
#include <unordered_map>
#include <string>
#include <mutex>
#include "net/TcpServer.h"
#include "rtsp.h"

namespace xop
{

class RtspConnection;

class RtspServer : public Rtsp, public TcpServer
{
public:
    RtspServer(xop::EventLoop *loop, std::string ip, uint16_t port = 554);
    ~RtspServer();

    MediaSessionId addMeidaSession(MediaSession* session);
    void removeMeidaSession(MediaSessionId sessionId);

    bool pushFrame(MediaSessionId sessionId, MediaChannelId channelId, AVFrame frame);

private:
    friend class RtspConnection;
    MediaSessionPtr lookMediaSession(const std::string& suffix);
    MediaSessionPtr lookMediaSession(MediaSessionId sessionId);

    virtual TcpConnection::Ptr newConnection(SOCKET sockfd);

    std::mutex _mtxSessionMap;
    std::unordered_map<MediaSessionId, std::shared_ptr<MediaSession>> _mediaSessions;
    std::unordered_map<std::string, MediaSessionId> _rtspSuffixMap;
};

}

#endif

