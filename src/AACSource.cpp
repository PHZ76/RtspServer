#include "AACSource.h"
#include "RtpConnection.h"
#include <cstdio>
#include <chrono>
#if defined(__linux) || defined(__linux__) 
#include <sys/time.h>
#endif 

using namespace xop;
using namespace std;

AACSource::AACSource(uint32_t sampleRate, uint32_t channels)
	: _sampleRate(sampleRate)
	, _channels(channels)
{
	_payload = 96;
	_mediaType = AAC;
	_clockRate = sampleRate;
}

AACSource* AACSource::createNew(uint32_t sampleRate, uint32_t channels)
{
	return new AACSource(sampleRate, channels);
}

AACSource::~AACSource()
{
	
}

string AACSource::getMediaDescription(uint16_t port)
{
	char buf[100] = { 0 };
	sprintf(buf, "m=audio %hu RTP/AVP 96", port);
	
	return string(buf);
}

static uint32_t AACSampleRate[16] = 
{
	96000, 88200, 64000, 48000,
	44100, 32000, 24000, 22050,
	16000, 12000, 11025, 8000,
	0, 0, 0, 0 /*reserved */
};

string AACSource::getAttribute()  // RFC 3640
{
	char buf[500] = { 0 };
	sprintf(buf, "a=rtpmap:96 MPEG4-GENERIC/%u/%u\r\n", _sampleRate, _channels);

	int index = 0;
	for (index = 0; index < 16; index++)
	{
		if (AACSampleRate[index] == _sampleRate)
			break;
	}
	if (index == 16)
		return ""; // error

	int profile = 1;
	sprintf(buf+strlen(buf), 
		"a=fmtp:96 profile-level-id=1;"
		//"streamtype=5;"
		"mode=AAC-hbr;"
		"sizelength=13;indexlength=3;indexdeltalength=3;"
		"config=%02X%02x",
		((profile+1) << 3) | (index >> 1),
		(index << 7) | (_channels<< 3));

	return string(buf);
}

#define ADTS_SIZE 7
#define AU_SIZE 4
bool AACSource::handleFrame(MediaChannelId channelId, AVFrame& frame)
{
	if (frame.size > (MAX_RTP_PAYLOAD_SIZE-AU_SIZE))
	{
		return false;
	}

	char *frameBuf  = frame.buffer.get() + ADTS_SIZE;
	uint32_t frameSize = frame.size - ADTS_SIZE;
	
	char AU[AU_SIZE] = { 0 };
	AU[0] = 0x00;
	AU[1] = 0x10;
	AU[2] = (frameSize & 0x1fe0) >> 5;
	AU[3] = (frameSize & 0x1f) << 3;

	RtpPacketPtr rtpPkt(new char[1500]);
	rtpPkt.get()[4 + RTP_HEADER_SIZE + 0] = AU[0];
	rtpPkt.get()[4 + RTP_HEADER_SIZE + 1] = AU[1];
	rtpPkt.get()[4 + RTP_HEADER_SIZE + 2] = AU[2];
	rtpPkt.get()[4 + RTP_HEADER_SIZE + 3] = AU[3];

	memcpy(rtpPkt.get()+4+RTP_HEADER_SIZE+AU_SIZE, frameBuf, frameSize);

	if(_sendFrameCallback)
		_sendFrameCallback(channelId, rtpPkt, frameSize+4+RTP_HEADER_SIZE+AU_SIZE, 1, frame.timestamp);
	
	return true;
}

uint32_t AACSource::getTimeStamp(uint32_t sampleRate)
{
	auto timePoint = chrono::time_point_cast<chrono::milliseconds>(chrono::high_resolution_clock::now());
	return (uint32_t)(timePoint.time_since_epoch().count()*sampleRate/1000);
}


