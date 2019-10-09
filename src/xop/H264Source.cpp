// PHZ
// 2018-5-16

#if defined(WIN32) || defined(_WIN32)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "H264Source.h"
#include <cstdio>
#include <chrono>
#if defined(__linux) || defined(__linux__)
#include <sys/time.h>
#endif

using namespace xop;
using namespace std;

H264Source::H264Source(uint32_t frameRate)
	: _frameRate(frameRate)
{
    _payload = 96; 
    _mediaType = H264;
    _clockRate = 90000;
}

H264Source* H264Source::createNew(uint32_t frameRate)
{
    return new H264Source(frameRate);
}

H264Source::~H264Source()
{

}

string H264Source::getMediaDescription(uint16_t port)
{
    char buf[100] = {0};
    sprintf(buf, "m=video %hu RTP/AVP 96", port); // \r\nb=AS:2000

    return string(buf);
}

string H264Source::getAttribute()
{
    return string("a=rtpmap:96 H264/90000");
}

bool H264Source::handleFrame(MediaChannelId channelId, AVFrame frame)
{
    uint8_t *frameBuf  = frame.buffer.get();
    uint32_t frameSize = frame.size;

    if(frame.timestamp == 0)
        frame.timestamp = getTimeStamp();

    if (frameSize <= MAX_RTP_PAYLOAD_SIZE)
    {
        RtpPacket rtpPkt;
        rtpPkt.type = frame.type;
        rtpPkt.timestamp = frame.timestamp;
        rtpPkt.size = frameSize + 4 + RTP_HEADER_SIZE;
        rtpPkt.last = 1;
        memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE, frameBuf, frameSize); /* 预留12字节 rtp header */

        if (_sendFrameCallback)
        {
            if (!_sendFrameCallback(channelId, rtpPkt))
                return false;
        }
    }
    else
    {
        char FU_A[2] = {0};

        FU_A[0] = (frameBuf[0] & 0xE0) | 28;
        FU_A[1] = 0x80 | (frameBuf[0] & 0x1f);

        frameBuf  += 1;
        frameSize -= 1;

        while (frameSize + 2 > MAX_RTP_PAYLOAD_SIZE)
        {
            RtpPacket rtpPkt;
            rtpPkt.type = frame.type;
            rtpPkt.timestamp = frame.timestamp;
            rtpPkt.size = 4 + RTP_HEADER_SIZE + MAX_RTP_PAYLOAD_SIZE;
            rtpPkt.last = 0;

            rtpPkt.data.get()[RTP_HEADER_SIZE+4] = FU_A[0];
            rtpPkt.data.get()[RTP_HEADER_SIZE+5] = FU_A[1];
            memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE+2, frameBuf, MAX_RTP_PAYLOAD_SIZE-2);

            if (_sendFrameCallback)
            {
                if (!_sendFrameCallback(channelId, rtpPkt))
                    return false;
            }

            frameBuf  += MAX_RTP_PAYLOAD_SIZE - 2;
            frameSize -= MAX_RTP_PAYLOAD_SIZE - 2;

            FU_A[1] &= ~0x80;
        }

        {
            RtpPacket rtpPkt;
            rtpPkt.type = frame.type;
            rtpPkt.timestamp = frame.timestamp;
            rtpPkt.size = 4 + RTP_HEADER_SIZE + 2 + frameSize;
            rtpPkt.last = 1;

            FU_A[1] |= 0x40;
            rtpPkt.data.get()[RTP_HEADER_SIZE+4] = FU_A[0];
            rtpPkt.data.get()[RTP_HEADER_SIZE+5] = FU_A[1];
            memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE+2, frameBuf, frameSize);

            if (_sendFrameCallback)
            {
                if (!_sendFrameCallback(channelId, rtpPkt))
                    return false;
            }
        }
    }

    return true;
}

uint32_t H264Source::getTimeStamp()
{
/* #if defined(__linux) || defined(__linux__)
	struct timeval tv = {0};
	gettimeofday(&tv, NULL);
	uint32_t ts = ((tv.tv_sec*1000)+((tv.tv_usec+500)/1000))*90; // 90: _clockRate/1000;
	return ts;
#else  */
	auto timePoint = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now());
	return (uint32_t)((timePoint.time_since_epoch().count() + 500) / 1000 * 90 );
//#endif
}
