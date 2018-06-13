// PHZ
// 2018-6-8

#ifndef XOP_MEDIA_SOURCE_H
#define XOP_MEDIA_SOURCE_H

#include "media.h"
#include "rtp.h"
#include <string>
#include <memory>
#include <cstdint>
#include <functional>
#include <map>
#include "xop/Socket.h"

namespace xop
{

class RtpConnection;

class MediaSource
{
public:
    typedef std::function<void (MediaChannelId channelId, uint8_t frameType, RtpPacketPtr& rtpPkt, uint32_t pktSize, uint8_t last, uint32_t ts)> SendFrameCallback;

    MediaSource() {}
    virtual ~MediaSource() {}

    virtual MediaType getMediaType() const
    { return _mediaType; }

    // SDP媒体描述 m=
    virtual std::string getMediaDescription(uint16_t port=0) = 0;

    // SDP媒体属性 a=
    virtual std::string getAttribute()  = 0;

    virtual bool handleFrame(MediaChannelId channelId, AVFrame& frame) = 0;
    virtual void setSendFrameCallback(const SendFrameCallback& cb)
    { _sendFrameCallback = cb; }

    virtual uint32_t getPayloadType() const
    { return _payload; }

    virtual uint32_t getClockRate() const
    { return _clockRate; }

protected:
    MediaType _mediaType = NONE;
    uint32_t _payload = 0;
    uint32_t _clockRate = 0;
    SendFrameCallback _sendFrameCallback;
};

}

#endif
