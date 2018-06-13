// PHZ
// 2018-6-8
// RTSP模板, 用于实现RtspServer和RtspPusher

#ifndef XOP_RTSP_H
#define XOP_RTSP_H

#include <string>
#include "MediaSession.h"
#include "xop/Acceptor.h"
#include "xop/EventLoop.h"
#include "xop/Socket.h"
#include "xop/Timer.h"

namespace xop
{

class RTSP
{
public:
    virtual ~RTSP() {};

    virtual MediaSessionId addMeidaSession(MediaSession* session) = 0;
    virtual void removeMeidaSession(MediaSessionId sessionId) = 0;

    virtual bool pushFrame(MediaSessionId sessionId, MediaChannelId channelId, AVFrame frame) = 0;

    virtual void setVersion(std::string version) // SDP Session Name
    { _version = std::move(version); }

    virtual std::string getVersion()
    { return _version; }

    virtual std::string getRtspUrl()
    { return _rtspUrl; }

protected:
    friend class RtspConnection;
    virtual MediaSessionPtr lookMediaSession(const std::string& suffix) = 0;
    virtual MediaSessionPtr lookMediaSession(MediaSessionId sessionId) = 0;

    std::string _version;
    std::string _rtspUrl;
};

}

#endif


