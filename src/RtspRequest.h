//
// Author: PHZ
// Date: 2018-1-15
//

#ifndef XOP_RTSP_REQUEST_H
#define XOP_RTSP_REQUEST_H

#include <utility> 
#include <unordered_map>
#include <string>
#include <cstring>

#include "rtp.h"

#include "media.h"
#include "xop/BufferReader.h"

namespace xop
{

class RtspRequest
{
public:
    enum RtspRequestParseState
    {
        kParseRequestLine, // 请求行
        kParseHeadersLine, // 首部行, 可能有多行
        //kParseBody,	// 主体
        kGotAll,
    };

    enum Method 
    {
        OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN,
        NONE, // 自定义
    };

    bool parseRequest(xop::BufferReader *buffer);

    bool gotAll() const
    { return _state == kGotAll; }

    void reset()
    {
        _state = kParseRequestLine;
        _requestLineParma.clear();
        _headerLineParma.clear();
    }

    Method getMethod() const
    { return _method; }

    uint32_t getCSeq() const;

    std::string getRtspUrl() const;

    std::string getRtspUrlSuffix() const;

    std::string getIp() const;

    TransportMode getTransportMode() const
    { return _transport; }

    MediaChannelId getChannelId() const
    { return _channelId; }

    uint8_t getRtpChannel() const;
    uint8_t getRtcpChannel() const;
    uint16_t getRtpPort() const;
    uint16_t getRtcpPort() const;
	
private:
    bool parseRequestLine(const char* begin, const char* end);
    bool parseHeadersLine(const char* begin, const char* end);
    bool parseCSeq(std::string& message);
    bool parseAccept(std::string& message);
    bool parseTransport(std::string& message);
    bool parseSessionId(std::string& message);	 
    bool parseMediaChannel(std::string& message);

    Method _method;
    MediaChannelId _channelId;
    TransportMode _transport;
    std::unordered_map<std::string, std::pair<std::string, uint32_t>> _requestLineParma;
    std::unordered_map<std::string, std::pair<std::string, uint32_t>> _headerLineParma;

    RtspRequestParseState _state = kParseRequestLine;
};	
	
}

#endif

