// PHZ
// 2018-6-8

#ifndef XOP_RTSP_MESSAGE_H
#define XOP_RTSP_MESSAGE_H

#include <utility>
#include <unordered_map>
#include <string>
#include <cstring>
#include "rtp.h"
#include "media.h"
#include "net/BufferReader.h"

namespace xop
{

class RtspRequest
{
public:
	enum Method
	{
		OPTIONS=0, DESCRIBE, SETUP, PLAY, TEARDOWN, GET_PARAMETER, 
		RTCP, NONE,
	};

	const char* MethodToString[8] =
	{
		"OPTIONS", "DESCRIBE", "SETUP", "PLAY", "TEARDOWN", "GET_PARAMETER",
		"RTCP", "NONE"
	};

	enum RtspRequestParseState
	{
		kParseRequestLine,
		kParseHeadersLine,
		//kParseBody,	
		kGotAll,
	};

	bool ParseRequest(xop::BufferReader *buffer);

	bool GotAll() const
	{ return state_ == kGotAll; }

	void Reset()
	{
		state_ = kParseRequestLine;
		request_line_param_.clear();
		header_line_param_.clear();
	}

	Method GetMethod() const
	{ return method_; }

	uint32_t GetCSeq() const;

	std::string GetRtspUrl() const;

	std::string GetRtspUrlSuffix() const;

	std::string GetIp() const;

	std::string GetAuthResponse() const;

	TransportMode GetTransportMode() const
	{ return transport_; }

	MediaChannelId GetChannelId() const
	{ return channel_id_; }

	uint8_t  GetRtpChannel() const;
	uint8_t  GetRtcpChannel() const;
	uint16_t GetRtpPort() const;
	uint16_t GetRtcpPort() const;

	int BuildOptionRes(const char* buf, int buf_size);
	int BuildDescribeRes(const char* buf, int buf_size, const char* sdp);
	int BuildSetupMulticastRes(const char* buf, int buf_size, const char* multicast_ip, uint16_t port, uint32_t session_id);
	int BuildSetupTcpRes(const char* buf, int buf_size, uint16_t rtp_chn, uint16_t rtcp_chn, uint32_t session_id);
	int BuildSetupUdpRes(const char* buf, int buf_size, uint16_t rtp_chn, uint16_t rtcp_chn, uint32_t session_id);
	int BuildPlayRes(const char* buf, int buf_size, const char* rtp_info, uint32_t session_id);
	int BuildTeardownRes(const char* buf, int buf_size, uint32_t session_id);
	int BuildGetParamterRes(const char* buf, int buf_size, uint32_t session_id);
	int BuildNotFoundRes(const char* buf, int buf_size);
	int BuildServerErrorRes(const char* buf, int buf_size);
	int BuildUnsupportedRes(const char* buf, int buf_size);
	int BuildUnauthorizedRes(const char* buf, int buf_size, const char* realm, const char* nonce);

private:
	bool ParseRequestLine(const char* begin, const char* end);
	bool ParseHeadersLine(const char* begin, const char* end);
	bool ParseCSeq(std::string& message);
	bool ParseAccept(std::string& message);
	bool ParseTransport(std::string& message);
	bool ParseSessionId(std::string& message);
	bool ParseMediaChannel(std::string& message);
	bool ParseAuthorization(std::string& message);

	Method method_;
	MediaChannelId channel_id_;
	TransportMode transport_;
	std::string auth_response_;
	std::unordered_map<std::string, std::pair<std::string, uint32_t>> request_line_param_;
	std::unordered_map<std::string, std::pair<std::string, uint32_t>> header_line_param_;

	RtspRequestParseState state_ = kParseRequestLine;
};

class RtspResponse
{
public:
	enum Method
	{
		OPTIONS=0, DESCRIBE, ANNOUNCE, SETUP, RECORD, RTCP,
		NONE, 
	};

	bool ParseResponse(xop::BufferReader *buffer);

	Method GetMethod() const
	{ return method_; }

	uint32_t GetCSeq() const
	{ return cseq_;  }

	std::string GetSession() const
	{ return session_; }

	void SetUserAgent(const char *user_agent) 
	{ user_agent_ = std::string(user_agent); }

	void SetRtspUrl(const char *url)
	{ rtsp_url_ = std::string(url); }

	int BuildOptionReq(const char* buf, int buf_size);
	int BuildDescribeReq(const char* buf, int buf_size);
	int BuildAnnounceReq(const char* buf, int buf_size, const char *sdp);
	int BuildSetupTcpReq(const char* buf, int buf_size, int channel);
	int BuildRecordReq(const char* buf, int buf_size);

private:
	Method method_;
	uint32_t cseq_ = 0;
	std::string user_agent_;
	std::string rtsp_url_;
	std::string session_;
};

}

#endif
