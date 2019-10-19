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

    void addMeidaSession(MediaSession* session);
    void removeMeidaSession(MediaSessionId sessionId);

    int openUrl(std::string url, int msec = 3000);
    void close();
	bool isConnected();

    bool pushFrame(MediaChannelId channelId, AVFrame frame);

private:
    friend class RtspConnection;
    MediaSessionPtr lookMediaSession(MediaSessionId sessionId);

    xop::EventLoop *_eventLoop = nullptr;
	xop::TaskScheduler *_taskScheduler = nullptr;
    std::mutex _mutex;
	std::shared_ptr<RtspConnection> _rtspConn;
    std::shared_ptr<MediaSession> _mediaSessionPtr;
};

}

#endif