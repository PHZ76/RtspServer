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
		kParseRequestLine, /* 请求行 */
		kParseHeadersLine, /* 首部行, 可能有多行 */
		//kParseBody,	/* 主体 */
		kGotAll,
	};

    bool parseRequest(xop::BufferReader *buffer);

    bool gotAll() const
    { return _state == kGotAll; }

    void reset()
    {
        _state = kParseRequestLine;
        _requestLineParam.clear();
        _headerLineParam.clear();
    }

    Method getMethod() const
    { return _method; }

    uint32_t getCSeq() const;

    std::string getRtspUrl() const;

    std::string getRtspUrlSuffix() const;

    std::string getIp() const;

	std::string getAuthResponse() const;

    TransportMode getTransportMode() const
    { return _transport; }

    MediaChannelId getChannelId() const
    { return _channelId; }

    uint8_t getRtpChannel() const;
    uint8_t getRtcpChannel() const;
    uint16_t getRtpPort() const;
    uint16_t getRtcpPort() const;

    int buildOptionRes(const char* buf, int bufSize);
    int buildDescribeRes(const char* buf, int bufSize, const char* strSdp);
    int buildSetupMulticastRes(const char* buf, int bufSize, const char* strMulticastIp, uint16_t port, uint32_t sessionId);
    int buildSetupTcpRes(const char* buf, int bufSize, uint16_t rtpChn, uint16_t rtcpChn, uint32_t sessionId);
    int buildSetupUdpRes(const char* buf, int bufSize, uint16_t serRtpChn, uint16_t serRtcpChn, uint32_t sessionId);
    int buildPlayRes(const char* buf, int bufSize, const char* rtpInfo, uint32_t sessionId);
    int buildTeardownRes(const char* buf, int bufSize, uint32_t sessionId);
    int buildGetParamterRes(const char* buf, int bufSize, uint32_t sessionId);
    int buildNotFoundRes(const char* buf, int bufSize);
    int buildServerErrorRes(const char* buf, int bufSize);
    int buildUnsupportedRes(const char* buf, int bufSize);
	int buildUnauthorizedRes(const char* buf, int bufSize, const char* realm, const char* nonce);

private:
    bool parseRequestLine(const char* begin, const char* end);
    bool parseHeadersLine(const char* begin, const char* end);
    bool parseCSeq(std::string& message);
    bool parseAccept(std::string& message);
    bool parseTransport(std::string& message);
    bool parseSessionId(std::string& message);
    bool parseMediaChannel(std::string& message);
	bool parseAuthorization(std::string& message);

    Method _method;
    MediaChannelId _channelId;
    TransportMode _transport;
	std::string _authResponse;
    std::unordered_map<std::string, std::pair<std::string, uint32_t>> _requestLineParam;
    std::unordered_map<std::string, std::pair<std::string, uint32_t>> _headerLineParam;

    RtspRequestParseState _state = kParseRequestLine;
};

class RtspResponse
{
public:
    enum Method
    {
        OPTIONS=0, DESCRIBE, ANNOUNCE, SETUP, RECORD, RTCP,
        NONE, 
    };

    bool parseResponse(xop::BufferReader *buffer);

    Method getMethod() const
    { return _method; }

    uint32_t getCSeq() const
    { return _cseq;  }

    std::string getSession() const
    { return _session; }

    void setUserAgent(const char *userAgent) 
    { _userAgent = std::string(userAgent); }

    void setRtspUrl(const char *url)
    { _rtspUrl = std::string(url); }

    int buildOptionReq(const char* buf, int bufSize);
    int buildDescribeReq(const char* buf, int bufSize);
    int buildAnnounceReq(const char* buf, int bufSize, const char *strSdp);
    int buildSetupTcpReq(const char* buf, int bufSize, int channel);
    int buildRecordReq(const char* buf, int bufSize);

private:
    Method _method;
    uint32_t _cseq = 0;
    std::string _userAgent;
    std::string _rtspUrl;
    std::string _session;
};

}

#endif
