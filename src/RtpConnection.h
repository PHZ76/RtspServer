#ifndef XOP_RTP_CONNECTION_H
#define XOP_RTP_CONNECTION_H

#include <cstdint>
#include <vector>
#include <string>
#include <random>
#include "rtp.h"
#include "media.h"
#include "xop/Socket.h"

namespace xop
{
	
class RtspConnection;

class RtpConnection
{
public:
	RtpConnection(RtspConnection *rtspConn);
	virtual ~RtpConnection();
	
	void setClockRate(MediaChannelId channelId, uint32_t clockRate)
	{ _mediaChannelInfo[channelId].clockRate = clockRate; }
	
	void setPayloadType(MediaChannelId channelId, uint32_t payload) 	
	{ _mediaChannelInfo[channelId].rtpHeader.payload = payload; }
	
	bool setupRtpOverTcp(MediaChannelId channelId, uint16_t rtpChannel, uint16_t rtcpChannel);
	bool setupRtpOverUdp(MediaChannelId channelId, uint16_t rtpPort, uint16_t rtcpPort);
	bool setupRtpOverMulticast(MediaChannelId channelId, int sockfd, std::string ip, uint16_t port);
	
	uint32_t getRtpSessionId() const
	{ return (uint32_t)((size_t)(this)); }
	
	uint16_t getRtpPort(MediaChannelId channelId) const
	{ return _localRtpPort[channelId]; }

	uint16_t getRtcpPort(MediaChannelId channelId) const
	{ return _localRtcpPort[channelId]; }
	
	std::string getMulticastIp(MediaChannelId channelId) const;
	
	void play();
	void teardown();
	
	std::string getRtpInfo(const std::string& rtspUrl);
	
	void setRtpHeader(MediaChannelId channelId, RtpPacketPtr& rtpPkt, uint8_t last, uint32_t ts);
	void sendRtpPacket(MediaChannelId channelId, RtpPacketPtr& rtpPkt, uint32_t pktSize);
	
private:
	friend class RtspConnection;
	friend class MediaSession;
	
	int sendRtpOverTcp(MediaChannelId channelId, RtpPacketPtr& rtpPkt, uint32_t pktSize);
	int sendRtpOverUdp(MediaChannelId channelId, RtpPacketPtr& rtpPkt, uint32_t pktSize);
	
	RtspConnection* _rtspConnection;
	
	TransportMode _transportMode;
	
	// server
	bool _isClosed = false;
	uint16_t _localRtpPort[2];	
	uint16_t _localRtcpPort[2];		
	SOCKET _rtpfd[2], _rtcpfd[2];
	
	// client
	struct sockaddr_in _peerAddr;	
	struct sockaddr_in _peerRtpAddr[2];
	struct sockaddr_in _peerRtcpAddr[2];
	MediaChannelInfo _mediaChannelInfo[2];
};	
	
}

#endif
