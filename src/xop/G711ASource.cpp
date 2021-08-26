// PHZ
// 2018-5-16

#if defined(WIN32) || defined(_WIN32) 
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif
#include "G711ASource.h"
#include <cstdio>
#include <chrono>
#if defined(__linux) || defined(__linux__) 
#include <sys/time.h>
#endif 

using namespace xop;
using namespace std;

G711ASource::G711ASource()
{
	payload_    = 8;
	media_type_ = PCMA;
	clock_rate_ = 8000;
}

G711ASource* G711ASource::CreateNew()
{
    return new G711ASource();
}

G711ASource::~G711ASource()
{
	
}

string G711ASource::GetMediaDescription(uint16_t port)
{
	char buf[100] = {0};
	sprintf(buf, "m=audio %hu RTP/AVP 8", port);
	return string(buf);
}
	
string G711ASource::GetAttribute()
{
    return string("a=rtpmap:8 PCMA/8000/1");
}

bool G711ASource::HandleFrame(MediaChannelId channel_id, AVFrame frame)
{
	if (frame.size > MAX_RTP_PAYLOAD_SIZE) {
		return false;
	}

	uint8_t *frame_buf  = frame.buffer.get();
	uint32_t frame_size = frame.size;

	RtpPacket rtp_pkt;
	rtp_pkt.type = frame.type;
	rtp_pkt.timestamp = frame.timestamp;
	rtp_pkt.size = frame_size + RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE;
	rtp_pkt.last = 1;

	memcpy(rtp_pkt.data.get()+RTP_TCP_HEAD_SIZE+RTP_HEADER_SIZE, frame_buf, frame_size);

	if (send_frame_callback_) {
		send_frame_callback_(channel_id, rtp_pkt);
	}

	return true;
}

uint32_t G711ASource::GetTimestamp()
{
	auto time_point = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now());
	return (uint32_t)((time_point.time_since_epoch().count()+500)/1000*8);
}

