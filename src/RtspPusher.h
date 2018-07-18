// PHZ
// 2018-6-27

#ifndef _RTSP_PUSHER_H
#define _RTSP_PUSHER_H

#include <mutex>
#include <map>
#include <cstdint>
#include "rtsp.h"

namespace xop
{

struct RtspInfo
{
    std::string url;
    std::string ip;
    uint16_t port;
    std::string suffix;
};

class RtspConnection;
class RtspPusher : public RTSP
{
public:
    RtspPusher(EventLoop* loop);
    ~RtspPusher();

    MediaSessionId addMeidaSession(MediaSession* session);
    void removeMeidaSession(MediaSessionId sessionId=0);

    bool openUrl(std::string url);
    void close();

    bool pushFrame(MediaSessionId sessionId, MediaChannelId channelId, AVFrame frame);

    bool isConnected() const
    { return (_connections.size() > 0); }

    std::string getRtspUrl() 
    { return _rtspInfo.url; }

private:
    friend class RtspConnection;

    bool parseRtspUrl(std::string& url);
    virtual MediaSessionPtr lookMediaSession(const std::string& suffix);
    virtual MediaSessionPtr lookMediaSession(MediaSessionId sessionId);

    std::shared_ptr<RtspConnection> newConnection(SOCKET sockfd);
    void removeConnection(SOCKET sockfd);

    xop::EventLoop* _loop = nullptr;
    std::mutex _mutex;
    std::shared_ptr<MediaSession> _mediaSessions;

    RtspInfo _rtspInfo;
    std::map<SOCKET, std::shared_ptr<RtspConnection>> _connections;
};

}

#endif

