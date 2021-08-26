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

AACSource::AACSource(uint32_t samplerate, uint32_t channels, bool has_adts)
	: samplerate_(samplerate)
	, channels_(channels)
	, has_adts_(has_adts)
{
	payload_    = 97;
	media_type_ = AAC;
	clock_rate_ = samplerate;
}

AACSource* AACSource::CreateNew(uint32_t samplerate, uint32_t channels, bool has_adts)
{
    return new AACSource(samplerate, channels, has_adts);
}

AACSource::~AACSource()
{

}

string AACSource::GetMediaDescription(uint16_t port)
{
	char buf[100] = { 0 };
	sprintf(buf, "m=audio %hu RTP/AVP 97", port); // \r\nb=AS:64
	return string(buf);
}

static uint32_t AACSampleRate[16] =
{
	96000, 88200, 64000, 48000,
	44100, 32000, 24000, 22050,
	16000, 12000, 11025, 8000,
	7350, 0, 0, 0 /*reserved */
};

string AACSource::GetAttribute()  // RFC 3640
{
	char buf[500] = { 0 };
	sprintf(buf, "a=rtpmap:97 MPEG4-GENERIC/%u/%u\r\n", samplerate_, channels_);

	uint8_t index = 0;
	for (index = 0; index < 16; index++) {
		if (AACSampleRate[index] == samplerate_) {
			break;
		}        
	}

	if (index == 16) {
		return ""; // error
	}
     
	uint8_t profile = 1;
	char config[10] = {0};

	sprintf(config, "%02x%02x", (uint8_t)((profile+1) << 3)|(index >> 1), (uint8_t)((index << 7)|(channels_<< 3)));
	sprintf(buf+strlen(buf),
			"a=fmtp:97 profile-level-id=1;"
			"mode=AAC-hbr;"
			"sizelength=13;indexlength=3;indexdeltalength=3;"
			"config=%04u",
			atoi(config));

	return string(buf);
}



bool AACSource::HandleFrame(MediaChannelId channel_id, AVFrame frame)
{
	if (frame.size > (MAX_RTP_PAYLOAD_SIZE-AU_SIZE)) {
		return false;
	}

	int adts_size = 0;
	if (has_adts_) {
		adts_size = ADTS_SIZE;
	}

	uint8_t *frame_buf = frame.buffer.get() + adts_size; 
	uint32_t frame_size = frame.size - adts_size;

	char AU[AU_SIZE] = { 0 };
	AU[0] = 0x00;
	AU[1] = 0x10;
	AU[2] = (frame_size & 0x1fe0) >> 5;
	AU[3] = (frame_size & 0x1f) << 3;

	RtpPacket rtp_pkt;
	rtp_pkt.type = frame.type;
	rtp_pkt.timestamp = frame.timestamp;
	rtp_pkt.size = frame_size + RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + AU_SIZE;
	rtp_pkt.last = 1;

	rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 0] = AU[0];
	rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 1] = AU[1];
	rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 2] = AU[2];
	rtp_pkt.data.get()[RTP_TCP_HEAD_SIZE + RTP_HEADER_SIZE + 3] = AU[3];

	memcpy(rtp_pkt.data.get()+RTP_TCP_HEAD_SIZE+RTP_HEADER_SIZE+AU_SIZE, frame_buf, frame_size);

	if (send_frame_callback_) {
		send_frame_callback_(channel_id, rtp_pkt);
	}

	return true;
}

uint32_t AACSource::GetTimestamp(uint32_t sampleRate)
{
	//auto time_point = chrono::time_point_cast<chrono::milliseconds>(chrono::high_resolution_clock::now());
	//return (uint32_t)(time_point.time_since_epoch().count() * sampleRate / 1000);

	auto time_point = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now());
	return (uint32_t)((time_point.time_since_epoch().count()+500) / 1000 * sampleRate / 1000);
}
