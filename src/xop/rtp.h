// PHZ
// 2021-9-2

#ifndef XOP_RTP_H
#define XOP_RTP_H

#include <memory>
#include <cstdint>

#define RTP_HEADER_SIZE   	   12
#define MAX_RTP_PAYLOAD_SIZE   1420 //1460  1500-20-12-8
#define RTP_VERSION			   2
#define RTP_TCP_HEAD_SIZE	   4
#define RTP_VPX_HEAD_SIZE	   1

#define RTP_HEADER_BIG_ENDIAN  0

namespace xop
{

enum TransportMode
{
	RTP_OVER_TCP = 1,
	RTP_OVER_UDP = 2,
	RTP_OVER_MULTICAST = 3,
};

typedef struct _RTP_header
{
#if RTP_HEADER_BIG_ENDIAN
	/* 大端序 */
	unsigned char version   : 2;
	unsigned char padding   : 1;
	unsigned char extension : 1;
	unsigned char csrc      : 4;
	unsigned char marker    : 1;
	unsigned char payload   : 7;
#else
	/* 小端序 */
	unsigned char csrc      : 4;
	unsigned char extension : 1;
	unsigned char padding   : 1;
	unsigned char version   : 2;
	unsigned char payload   : 7;
	unsigned char marker    : 1;
#endif 
	unsigned short seq;
	unsigned int   ts;
	unsigned int   ssrc;
} RtpHeader;

struct MediaChannelInfo
{
	RtpHeader rtp_header;

	// tcp
	uint16_t rtp_channel;
	uint16_t rtcp_channel;

	// udp
	uint16_t rtp_port;
	uint16_t rtcp_port;
	uint16_t packet_seq;
	uint32_t clock_rate;

	// rtcp
	uint64_t packet_count;
	uint64_t octet_count;
	uint64_t last_rtcp_ntp_time;

	bool is_setup;
	bool is_play;
	bool is_record;
};

struct RtpPacket
{
	RtpPacket()
		: data(new uint8_t[1600], std::default_delete<uint8_t[]>())
	{
		type = 0;
		size = 0;
		timestamp = 0;
		last = 0;
	}

	std::shared_ptr<uint8_t> data;
	uint32_t size;
	uint32_t timestamp;
	uint8_t  type;
	uint8_t  last;
};

}

#endif
