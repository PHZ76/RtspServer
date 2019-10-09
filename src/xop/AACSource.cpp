// PHZ
// 2018-5-16

#if defined(WIN32) || defined(_WIN32)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif
#include "AACSource.h"
#include <stdlib.h>
#include <cstdio>
#include <chrono>
#if defined(__linux) || defined(__linux__)
#include <sys/time.h>
#endif

using namespace xop;
using namespace std;

AACSource::AACSource(uint32_t sampleRate, uint32_t channels, bool hasADTS)
    : _sampleRate(sampleRate)
    , _channels(channels)
	, _hasADTS(hasADTS)
{
    _payload = 97;
    _mediaType = AAC;
    _clockRate = sampleRate;
}

AACSource* AACSource::createNew(uint32_t sampleRate, uint32_t channels, bool hasADTS)
{
    return new AACSource(sampleRate, channels, hasADTS);
}

AACSource::~AACSource()
{

}

string AACSource::getMediaDescription(uint16_t port)
{
    char buf[100] = { 0 };
    sprintf(buf, "m=audio %hu RTP/AVP 97", port); // \r\nb=AS:64

    return string(buf);
}

static uint32_t AACSampleRate[16] =
{
    97000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,
    7350, 0, 0, 0 /*reserved */
};

string AACSource::getAttribute()  // RFC 3640
{
    char buf[500] = { 0 };
    sprintf(buf, "a=rtpmap:97 MPEG4-GENERIC/%u/%u\r\n", _sampleRate, _channels);

    uint8_t index = 0;
    for (index = 0; index < 16; index++)
    {
        if (AACSampleRate[index] == _sampleRate)
            break;
    }
    if (index == 16)
        return ""; // error

    uint8_t profile = 1;
    char configStr[10] = {0};
    sprintf(configStr, "%02x%02x", (uint8_t)((profile+1) << 3)|(index >> 1), (uint8_t)((index << 7)|(_channels<< 3)));

    sprintf(buf+strlen(buf),
            "a=fmtp:97 profile-level-id=1;"
            "mode=AAC-hbr;"
            "sizelength=13;indexlength=3;indexdeltalength=3;"
            "config=%04u",
             atoi(configStr));

    return string(buf);
}

#define ADTS_SIZE 7
#define AU_SIZE 4
bool AACSource::handleFrame(MediaChannelId channelId, AVFrame frame)
{
    if (frame.size > (MAX_RTP_PAYLOAD_SIZE-AU_SIZE))
    {
        return false;
    }

    int adtsSize = 0;
    if (_hasADTS)
    {
        adtsSize = ADTS_SIZE;
    }

    uint8_t *frameBuf = frame.buffer.get() + adtsSize; /* 打包RTP去掉ADTS头 */
    uint32_t frameSize = frame.size - adtsSize;

    char AU[AU_SIZE] = { 0 };
    AU[0] = 0x00;
    AU[1] = 0x10;
    AU[2] = (frameSize & 0x1fe0) >> 5;
    AU[3] = (frameSize & 0x1f) << 3;

    RtpPacket rtpPkt;
    rtpPkt.type = frame.type;
    rtpPkt.timestamp = frame.timestamp;
    rtpPkt.size = frameSize + 4 + RTP_HEADER_SIZE + AU_SIZE;
    rtpPkt.last = 1;

    rtpPkt.data.get()[4 + RTP_HEADER_SIZE + 0] = AU[0];
    rtpPkt.data.get()[4 + RTP_HEADER_SIZE + 1] = AU[1];
    rtpPkt.data.get()[4 + RTP_HEADER_SIZE + 2] = AU[2];
    rtpPkt.data.get()[4 + RTP_HEADER_SIZE + 3] = AU[3];

    memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE+AU_SIZE, frameBuf, frameSize);

    if(_sendFrameCallback)
        _sendFrameCallback(channelId, rtpPkt);

    return true;
}

uint32_t AACSource::getTimeStamp(uint32_t sampleRate)
{
    //auto timePoint = chrono::time_point_cast<chrono::milliseconds>(chrono::high_resolution_clock::now());
    //return (uint32_t)(timePoint.time_since_epoch().count() * sampleRate / 1000);

    auto timePoint = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now());
    return (uint32_t)((timePoint.time_since_epoch().count()+500) / 1000 * sampleRate / 1000);
}
