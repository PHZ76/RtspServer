#ifndef XOP_RTSP_PUSHER_H
#define XOP_RTSP_PUSHER_H

#include <mutex>
#include <map>
#include "rtsp.h"

namespace xop
{

class RtspConnection;

class RtspPusher : public Rtsp
{
public:
    RtspPusher(xop::EventLoop *eventLoop);
    ~RtspPusher();

    MediaSessionId addMeidaSession(MediaSession* session);
    void removeMeidaSession(MediaSessionId sessionId);

    int openUrl(std::string url);
    void close();

    bool pushFrame(MediaSessionId sessionId, MediaChannelId channelId, AVFrame frame);

    bool isConnected() const
    { return (_connections.size() > 0); }

private:
    friend class RtspConnection;
    MediaSessionPtr lookMediaSession(MediaSessionId sessionId);

    std::shared_ptr<RtspConnection> newConnection(SOCKET sockfd);
    void removeConnection(SOCKET sockfd);

    xop::EventLoop *_eventLoop = nullptr;
    std::mutex _mutex;
    std::map<SOCKET, std::shared_ptr<RtspConnection>> _connections;

    std::shared_ptr<MediaSession> _mediaSessionPtr;
};

}

#endif