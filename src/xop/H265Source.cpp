// PHZ
// 2018-6-7

#if defined(WIN32) || defined(_WIN32) 
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "H265Source.h"
#include <cstdio>
#include <chrono>
#if defined(__linux) || defined(__linux__) 
#include <sys/time.h>
#endif

using namespace xop;
using namespace std;

H265Source::H265Source(uint32_t frameRate)
	: _frameRate(frameRate)
{
    _payload = 96;
    _mediaType = H265;
    _clockRate = 90000;
}

H265Source* H265Source::createNew(uint32_t frameRate)
{
    return new H265Source(frameRate);
}

H265Source::~H265Source()
{
	
}

string H265Source::getMediaDescription(uint16_t port)
{
    char buf[100] = {0};
    sprintf(buf, "m=video %hu RTP/AVP 96", port);

    return string(buf);
}
	
string H265Source::getAttribute()
{
    return string("a=rtpmap:96 H265/90000");
}

bool H265Source::handleFrame(MediaChannelId channelId, AVFrame frame)
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

        memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE, frameBuf, frameSize); /* 预留 4字节TCP Header, 12字节 RTP Header */
        
        if (_sendFrameCallback)
        {
            if (!_sendFrameCallback(channelId, rtpPkt))
                return false;
        }
    }	
    else
    {	
        /* 参考live555 */
        char FU[3] = {0};	
        char nalUnitType = (frameBuf[0] & 0x7E) >> 1; 
        FU[0] = (frameBuf[0] & 0x81) | (49<<1); 
        FU[1] = frameBuf[1]; 
        FU[2] = (0x80 | nalUnitType); 
        
        frameBuf  += 2;
        frameSize -= 2;
        
        while (frameSize + 3 > MAX_RTP_PAYLOAD_SIZE) 
        {
            RtpPacket rtpPkt;
            rtpPkt.type = frame.type;
            rtpPkt.timestamp = frame.timestamp;
            rtpPkt.size = 4 + RTP_HEADER_SIZE + MAX_RTP_PAYLOAD_SIZE;
            rtpPkt.last = 0;

            rtpPkt.data.get()[RTP_HEADER_SIZE+4] = FU[0];
            rtpPkt.data.get()[RTP_HEADER_SIZE+5] = FU[1];
            rtpPkt.data.get()[RTP_HEADER_SIZE+6] = FU[2];
            memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE+3, frameBuf, MAX_RTP_PAYLOAD_SIZE-3);
            
            if (_sendFrameCallback)
            {
                if (!_sendFrameCallback(channelId, rtpPkt))
                    return false;
            }
            
            frameBuf  += (MAX_RTP_PAYLOAD_SIZE - 3);
            frameSize -= (MAX_RTP_PAYLOAD_SIZE - 3);
        
            FU[2] &= ~0x80;						
        }
        
        {
            RtpPacket rtpPkt;
            rtpPkt.type = frame.type;
            rtpPkt.timestamp = frame.timestamp;
            rtpPkt.size = 4 + RTP_HEADER_SIZE + 3 + frameSize;
            rtpPkt.last = 1;

            FU[2] |= 0x40;
            rtpPkt.data.get()[RTP_HEADER_SIZE+4] = FU[0];
            rtpPkt.data.get()[RTP_HEADER_SIZE+5] = FU[1];
            rtpPkt.data.get()[RTP_HEADER_SIZE+6] = FU[2];
            memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE+3, frameBuf, frameSize);
            
            if (_sendFrameCallback)
            {
                if (!_sendFrameCallback(channelId, rtpPkt))
                    return false;
            }
        }            
    }

    return true;
}


uint32_t H265Source::getTimeStamp()
{
/* #if defined(__linux) || defined(__linux__) 
	struct timeval tv = {0};
	gettimeofday(&tv, NULL);
	uint32_t ts = ((tv.tv_sec*1000)+((tv.tv_usec+500)/1000))*90; // 90: _clockRate/1000;
	return ts;
#else */
	//auto timePoint = chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now());
	//auto timePoint = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now());
	auto timePoint = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now());
	return (uint32_t)((timePoint.time_since_epoch().count() + 500) / 1000 * 90);
//#endif 
}
