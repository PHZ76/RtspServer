// PHZ
// 2018-6-8

#ifndef XOP_RTP_H
#define XOP_RTP_H

#include <memory>
#include <cstdint>

#define RTP_HEADER_SIZE   	   12
#define MAX_RTP_PAYLOAD_SIZE   1420//1460  1500-20-12-8
#define RTP_VERSION			   2
#define RTP_TCP_HEAD_SIZE	   4

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
	/* ´ó¶ËÐò
	uint8_t version : 2;
	uint8_t padding : 1;
	uint8_t extension : 1;
	uint8_t csrc : 4;
	uint8_t marker : 1;
	uint8_t payload : 7;*/

	uint8_t csrc : 4;
	uint8_t extension : 1;
	uint8_t padding : 1;
	uint8_t version : 2;
	uint8_t payload : 7;
	uint8_t marker : 1;

	uint16_t seq;
	uint32_t ts;
	uint32_t ssrc;
} RtpHeader;

struct MediaChannelInfo
{
	RtpHeader rtpHeader;

    // tcp
    uint16_t rtpChannel;
    uint16_t rtcpChannel;

    // udp
    uint16_t rtpPort;
    uint16_t rtcpPort;

    uint16_t packetSeq;
    //uint64_t packetCount;
    //uint64_t octetCount;
    //uint64_t lastRtcpNtpTime;
    uint32_t clockRate;

	bool isSetup;
	bool isPlay;
	bool isRecord;
};

typedef std::shared_ptr<char> RtpPacketPtr;

}

#endif
