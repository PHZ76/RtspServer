// PHZ
// 2018-6-8

#ifndef XOP_RTSP_H
#define XOP_RTSP_H

#include <cstdio>
#include <string>
#include "MediaSession.h"
#include "net/Acceptor.h"
#include "net/EventLoop.h"
#include "net/Socket.h"
#include "net/Timer.h"

namespace xop
{

struct RtspUrlInfo
{
    std::string url;
    std::string ip;
    uint16_t port;
    std::string suffix;
};

class Rtsp
{
public:
	Rtsp() : _hasAuthInfo(false) {}
    virtual ~Rtsp() {}

    virtual MediaSessionId addMeidaSession(MediaSession* session)
    { return 0; }

    virtual void removeMeidaSession(MediaSessionId sessionId)
    { return; }

    virtual bool pushFrame(MediaSessionId sessionId, MediaChannelId channelId, AVFrame frame)
    { return false;}

	virtual void setAuthConfig(std::string realm, std::string username, std::string password)
	{
		_realm = realm;
		_username = username;
		_password = password;
		_hasAuthInfo = true;

		if (_realm=="" || username=="")
		{
			_hasAuthInfo = false;
		}
	}

    virtual void setVersion(std::string version) // SDP Session Name
    { _version = std::move(version); }

    virtual std::string getVersion()
    { return _version; }

    virtual std::string getRtspUrl()
    { return _rtspUrlInfo.url; }

	bool parseRtspUrl(std::string url)
	{
		char ip[100] = { 0 };
		char suffix[100] = { 0 };
		uint16_t port = 0;
#if defined(__linux) || defined(__linux__)
		if (sscanf(url.c_str() + 7, "%[^:]:%hu/%s", ip, &port, suffix) == 3)
#elif defined(WIN32) || defined(_WIN32)
		if (sscanf_s(url.c_str() + 7, "%[^:]:%hu/%s", ip, 100, &port, suffix, 100) == 3)
#endif
		{
			_rtspUrlInfo.port = port;
		}
#if defined(__linux) || defined(__linux__)
		else if (sscanf(url.c_str() + 7, "%[^/]/%s", ip, suffix) == 2)
#elif defined(WIN32) || defined(_WIN32)
		else if (sscanf_s(url.c_str() + 7, "%[^/]/%s", ip, 100, suffix, 100) == 2)
#endif
		{
			_rtspUrlInfo.port = 554;
		}
		else
		{
			//LOG("%s was illegal.\n", url.c_str());
			return false;
		}

		_rtspUrlInfo.ip = ip;
		_rtspUrlInfo.suffix = suffix;
		_rtspUrlInfo.url = url;
		return true;
	}

protected:
    friend class RtspConnection;
    virtual MediaSessionPtr lookMediaSession(const std::string& suffix)
    { return nullptr; }

    virtual MediaSessionPtr lookMediaSession(MediaSessionId sessionId)
    { return nullptr; }

	bool _hasAuthInfo = false;
	std::string _realm;
	std::string _username;
	std::string _password;
    std::string _version;
    //std::string _rtspUrl;
    struct RtspUrlInfo _rtspUrlInfo;
};

}

#endif


