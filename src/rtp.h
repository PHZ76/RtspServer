// PHZ
// 2018-6-8

#ifndef XOP_RTP_H
#define XOP_RTP_H

#include <memory>

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
#ifdef BIGENDIAN//defined(sun) || defined(__BIG_ENDIAN) || defined(NET_ENDIAN)
    unsigned char version:2;
    unsigned char padding:1;
    unsigned char extension:1;
    unsigned char csrc:4;
    unsigned char marker:1;
    unsigned char payload:7;
#else
    unsigned char csrc:4;		
    unsigned char extension:1;		
    unsigned char padding:1;		
    unsigned char version:2;		
    unsigned char payload:7;		
    unsigned char marker:1;		
#endif
    unsigned short seq;
    unsigned int   ts;
    unsigned int   ssrc;	        				
} RtpHeader; 

struct MediaChannelInfo
{
    bool isSetup;
    bool isPlay;   //客户端拉流
	bool isRecord; //推流到服务器

    // tcp
	uint16_t rtpChannel;
	uint16_t rtcpChannel;

    // udp
    uint16_t rtpPort;
    uint16_t rtcpPort;

    uint16_t packetSeq;
    uint64_t packetCount;
    uint64_t octetCount;
    uint64_t lastRtcpNtpTime;

    uint32_t clockRate;

    RtpHeader rtpHeader;
};

typedef std::shared_ptr<char> RtpPacketPtr;
	
}

#endif
