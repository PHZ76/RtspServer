// PHZ
// 2018-5-16

#if defined(WIN32) || defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "RtspMessage.h"
#include "media.h"
#include "xop/log.h"

using namespace std;
using namespace xop;

bool RtspRequest::parseRequest(BufferReader *buffer)
{
    bool ret = true;
    while(1)
    {
        if(_state == kParseRequestLine)
        {
            const char* firstCrlf = buffer->findFirstCrlf();
            if(firstCrlf != nullptr)
            {
                ret = parseRequestLine(buffer->peek(), firstCrlf);
                buffer->retrieveUntil(firstCrlf + 2);
            }

            if(_state == kParseHeadersLine)
                continue;
            else
                break;
        }
        else if(_state == kParseHeadersLine)
        {
            const char* lastCrlf = buffer->findLastCrlf();
            if(lastCrlf != nullptr)
            {
                ret = parseHeadersLine(buffer->peek(), lastCrlf);
                buffer->retrieveUntil(lastCrlf + 2);
            }
            break;
        }
        else if(_state == kGotAll)
        {
            return true;
        }
    }

    return ret;
}

bool RtspRequest::parseRequestLine(const char* begin, const char* end)
{
    string message(begin, end);
    char method[64] = {0};
    char url[512] = {0};
    char version[64] = {0};

    if(sscanf(message.c_str(), "%s %s %s", method, url, version) != 3)
    {
        return true; //
    }

    string methodString(method);
    if(methodString == "OPTIONS")
    {
        _method = OPTIONS;
    }
    else if(methodString == "DESCRIBE")
    {
        _method = DESCRIBE;
    }
    else if(methodString == "SETUP")
    {
        _method = SETUP;
    }
    else if(methodString == "PLAY")
    {
        _method = PLAY;
    }
    else if(methodString == "TEARDOWN")
    {
        _method = TEARDOWN;
    }
    else if(methodString == "GET_PARAMETER")
    {
        _method = GET_PARAMETER;
    }
    else
    {
        _method = NONE;
        return false;
    }

    if(strncmp(url, "rtsp://", 7) != 0)
    {
        return false;
    }

    // parse url
    uint16_t port = 0;
    char ip[64] = {0};
    char suffix[64] = {0};

    if(sscanf(url+7, "%[^:]:%hu/%s", ip, &port, suffix) == 3)
    {

    }
    else if(sscanf(url+7, "%[^/]/%s", ip, suffix) == 2)
    {
        port = 554;
    }
    else
    {
        return false;
    }

    _requestLineParam.emplace("url", make_pair(string(url), 0));
    _requestLineParam.emplace("url_ip", make_pair(string(ip), 0));
    _requestLineParam.emplace("url_port", make_pair("", (uint32_t)port));
    _requestLineParam.emplace("url_suffix", make_pair(string(suffix), 0));
    _requestLineParam.emplace("version", make_pair(string(version), 0));
    _requestLineParam.emplace("method", make_pair(move(methodString), 0));

    _state = kParseHeadersLine;

    return true;
}

bool RtspRequest::parseHeadersLine(const char* begin, const char* end)
{
    string message(begin, end);
    if(!parseCSeq(message))
    {
        if(_headerLineParam.find("cseq") == _headerLineParam.end())
            return false;
    }

    if(_method == OPTIONS)
    {
        _state = kGotAll;
        return true;
    }

    if(_method == DESCRIBE)
    {
        if(parseAccept(message))
        {
            _state = kGotAll;
        }
        return true;
    }

    if(_method == SETUP)
    {
        if(parseTransport(message))
        {
            parseMediaChannel(message);
            _state = kGotAll;
        }

        return true;
    }

    if(_method == PLAY)
    {
        if(parseSessionId(message))
        {
            _state = kGotAll;
        }
        return true;
    }

    if(_method == TEARDOWN)
    {
        _state = kGotAll;
        return true;
    }

    if(_method == GET_PARAMETER)
    {
        _state = kGotAll;
        return true;
    }

    return true;
}

bool RtspRequest::parseCSeq(std::string& message)
{
    std::size_t pos = message.find("CSeq");
    if (pos != std::string::npos)
    {
        uint32_t cseq = 0;
        sscanf(message.c_str()+pos, "%*[^:]: %u", &cseq);
        _headerLineParam.emplace("cseq", make_pair("", cseq));
        return true;
    }

    return false;
}

bool RtspRequest::parseAccept(std::string& message)
{
    // 只支持sdp
    if ((message.rfind("Accept")==std::string::npos)
        || (message.rfind("sdp")==std::string::npos))
    {
        return false;
    }

    return true;
}

bool RtspRequest::parseTransport(std::string& message)
{
    // 解析 传输方式
    std::size_t pos = message.find("Transport");
    if(pos != std::string::npos)
    {
        if((pos=message.find("RTP/AVP/TCP")) != std::string::npos)
        {
            _transport = RTP_OVER_TCP;
            uint16_t rtpChannel = 0, rtcpChannel = 0;
            if(sscanf(message.c_str()+pos, "%*[^;];%*[^;];%*[^=]=%hu-%hu",
                     &rtpChannel, &rtcpChannel) != 2)
                return false;
            _headerLineParam.emplace("rtp_channel", make_pair("", rtpChannel));
            _headerLineParam.emplace("rtcp_channel", make_pair("", rtcpChannel));
        }
        else if((pos=message.find("RTP/AVP")) != std::string::npos)
        {
            uint16_t rtpPort = 0, rtcpPort = 0;
            if(((message.find("unicast", pos)) != std::string::npos))
            {
                _transport = RTP_OVER_UDP;
                if(sscanf(message.c_str()+pos, "%*[^;];%*[^;];%*[^=]=%hu-%hu",
                     &rtpPort, &rtcpPort) != 2)
                {
                    return false;
                }

            }
            else if((message.find("multicast", pos)) != std::string::npos)
            {
                _transport = RTP_OVER_MULTICAST;
            }
            else
                return false;

            _headerLineParam.emplace("rtp_port", make_pair("", rtpPort));
            _headerLineParam.emplace("rtcp_port", make_pair("", rtcpPort));
        }
        else
        {
            return false;
        }

        return true;
    }

    return false;
}

bool RtspRequest::parseSessionId(std::string& message)
{
    std::size_t pos = message.find("Session");
    if (pos != std::string::npos)
    {
        uint32_t sessionId = 0;
        if(sscanf(message.c_str()+pos, "%*[^:]: %u", &sessionId) != 1)
            return false;
        return true;
    }

    return false;
}

bool RtspRequest::parseMediaChannel(std::string& message)
{
    _channelId = channel_0;

    auto iter = _requestLineParam.find("url");
    if(iter != _requestLineParam.end())
    {
        std::size_t pos = iter->second.first.find("track1");
        if(pos != std::string::npos)
            _channelId = channel_1;
    }

    return true;
}

uint32_t RtspRequest::getCSeq() const
{
    uint32_t cseq = 0;
    auto iter = _headerLineParam.find("cseq");
    if(iter != _headerLineParam.end())
    {
        cseq = iter->second.second;
    }

    return cseq;
}

std::string RtspRequest::getIp() const
{
    auto iter = _requestLineParam.find("url_ip");
    if(iter != _requestLineParam.end())
    {
        return iter->second.first;
    }

    return "";
}

std::string RtspRequest::getRtspUrl() const
{
    auto iter = _requestLineParam.find("url");
    if(iter != _requestLineParam.end())
    {
        return iter->second.first;
    }

    return "";
}

std::string RtspRequest::getRtspUrlSuffix() const
{
    auto iter = _requestLineParam.find("url_suffix");
    if(iter != _requestLineParam.end())
    {
        return iter->second.first;
    }

    return "";
}

uint8_t RtspRequest::getRtpChannel() const
{
    auto iter = _headerLineParam.find("rtp_channel");
    if(iter != _headerLineParam.end())
    {
        return iter->second.second;
    }

    return 0;
}

uint8_t RtspRequest::getRtcpChannel() const
{
    auto iter = _headerLineParam.find("rtcp_channel");
    if(iter != _headerLineParam.end())
    {
        return iter->second.second;
    }

    return 0;
}

uint16_t RtspRequest::getRtpPort() const
{
    auto iter = _headerLineParam.find("rtp_port");
    if(iter != _headerLineParam.end())
    {
        return iter->second.second;
    }

    return 0;
}

uint16_t RtspRequest::getRtcpPort() const
{
    auto iter = _headerLineParam.find("rtcp_port");
    if(iter != _headerLineParam.end())
    {
        return iter->second.second;
    }

    return 0;
}

bool RtspResponse::parseResponse(xop::BufferReader *buffer)
{
    // 暂时不做解析, 只判断报文是否完整
    if (strstr(buffer->peek(), "\r\n\r\n") != NULL)
    {
        if (strstr(buffer->peek(), "OK") == NULL)
        {
            return false;
        }

        char* ptr = strstr(buffer->peek(), "Session");
        if (ptr != NULL)
        {
            char sessionId[50] = {0};
            if (sscanf(ptr, "%*[^:]: %s", sessionId) == 1)
                _session = sessionId;
        }

        _cseq++;
        buffer->retrieveUntil("\r\n\r\n");
    }

    return true;
}
