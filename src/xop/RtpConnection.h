// PHZ
// 2018-6-8

#ifndef XOP_RTP_CONNECTION_H
#define XOP_RTP_CONNECTION_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <random>
#include "rtp.h"
#include "media.h"
#include "net/Socket.h"
#include "net/TcpConnection.h"

namespace xop
{

class RtspConnection;

class RtpConnection
{
public:
    RtpConnection(std::weak_ptr<TcpConnection> rtsp_connection);
    virtual ~RtpConnection();

    void SetClockRate(MediaChannelId channel_id, uint32_t clock_rate)
    { media_channel_info_[channel_id].clock_rate = clock_rate; }

    void SetPayloadType(MediaChannelId channel_id, uint32_t payload)
    { media_channel_info_[channel_id].rtp_header.payload = payload; }

    bool SetupRtpOverTcp(MediaChannelId channel_id, uint16_t rtp_channel, uint16_t rtcp_channel);
    bool SetupRtpOverUdp(MediaChannelId channel_id, uint16_t rtp_port, uint16_t rtcp_port);
    bool SetupRtpOverMulticast(MediaChannelId channel_id, std::string ip, uint16_t port);

    uint32_t GetRtpSessionId() const
    { return (uint32_t)((size_t)(this)); }

    uint16_t GetRtpPort(MediaChannelId channel_id) const
    { return local_rtp_port_[channel_id]; }

    uint16_t GetRtcpPort(MediaChannelId channel_id) const
    { return local_rtcp_port_[channel_id]; }

    SOCKET GetRtpSocket(MediaChannelId channel_id) const
    { return rtpfd_[channel_id]; }

    SOCKET GetRtcpSocket(MediaChannelId channel_id) const
    { return rtcpfd_[channel_id]; }

    std::string GetIp()
    { return rtsp_ip_; }

    uint16_t GetPort()
    { return rtsp_port_; }
    
    bool IsMulticast() const
    { return is_multicast_; }

    bool IsSetup(MediaChannelId channel_id) const
    { return media_channel_info_[channel_id].is_setup; }

    std::string GetMulticastIp(MediaChannelId channel_id) const;

    void Play();
    void Record();
    void Teardown();

    std::string GetRtpInfo(const std::string& rtsp_url);
    int SendRtpPacket(MediaChannelId channel_id, RtpPacket pkt);

    bool IsClosed() const
    { return is_closed_; }

    int GetId() const;

    bool HasKeyFrame() const
    { return has_key_frame_; }

private:
    friend class RtspConnection;
    friend class MediaSession;
    void SetFrameType(uint8_t frameType = 0);
    void SetRtpHeader(MediaChannelId channel_id, RtpPacket pkt);
    int  SendRtpOverTcp(MediaChannelId channel_id, RtpPacket pkt);
    int  SendRtpOverUdp(MediaChannelId channel_id, RtpPacket pkt);

	std::weak_ptr<TcpConnection> rtsp_connection_;
    std::string rtsp_ip_;
    uint16_t rtsp_port_;

    TransportMode transport_mode_;
    bool is_multicast_ = false;

	bool is_closed_ = false;
	bool has_key_frame_ = false;

    uint8_t  frame_type_ = 0;
    uint16_t local_rtp_port_[MAX_MEDIA_CHANNEL];
    uint16_t local_rtcp_port_[MAX_MEDIA_CHANNEL];
    SOCKET rtpfd_[MAX_MEDIA_CHANNEL];
    SOCKET rtcpfd_[MAX_MEDIA_CHANNEL];

    struct sockaddr_in peer_addr_;
    struct sockaddr_in peer_rtp_addr_[MAX_MEDIA_CHANNEL];
    struct sockaddr_in peer_rtcp_sddr_[MAX_MEDIA_CHANNEL];
    MediaChannelInfo media_channel_info_[MAX_MEDIA_CHANNEL];
};

}

#endif
