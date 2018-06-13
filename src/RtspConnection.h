// PHZ
// 2018-6-8

#ifndef _RTSP_CONNECTION_H
#define _RTSP_CONNECTION_H

#include <iostream>
#include <functional>
#include <memory>
#include <vector>
#include <cstdint>
#include "RtpConnection.h"
#include "RtspMessage.h"
#include "xop/BufferReader.h"
#include "xop/BufferWriter.h"
#include "xop/EventLoop.h"
#include "xop/Channel.h"
#include "rtsp.h"

namespace xop
{

class RtspServer;
class MediaSession;

class RtspConnection : public std::enable_shared_from_this<RtspConnection>
{
public:
    typedef std::function<void (SOCKET sockfd)> CloseCallback;

	enum ConnectionMode
	{
		RTSP_SERVER, // RTSP转发
		RTSP_CLIENT, // RTSP推流
	};

    RtspConnection() = delete;
    RtspConnection(RTSP* rtsp, EventLoop* loop, int sockfd);
    ~RtspConnection();

    int fd() const
    { return _sockfd; }

    MediaSessionId getMediaSessionId()
    { return _sessionId; }

    void setCloseCallback(const CloseCallback& cb)
    { _closeCallback = cb; }

    void keepAlive()
    { _aliveCount++; }

    bool isAlive() const
    {
		if (_isClosed)
		{
			return false;
		}

        if(_rtpConnection)
        {
            // 组播暂时不加入心跳检测
            if(_rtpConnection->isMulticast())
                return true;
        }

        return (_aliveCount > 0);
    }

    void resetAliveCount()
    { _aliveCount = 0; }

private:
    friend class RtpConnection;
    friend class MediaSession;
    friend class RtspServer;
    friend class RtspPusher;

    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();
    void handleRtcp(SOCKET sockfd);

    void sendMessage(std::shared_ptr<char> buf, uint32_t size);
    bool handleRtspRequest();
    bool handleRtspResponse();

    void handleCmdOption();
    void handleCmdDescribe();
    void handleCmdSetup();
    void handleCmdPlay();
    void handleCmdTeardown();
    void handleCmdGetParamter();

    void sendOptions();
    void sendAnnounce();
    void sendSetup();
    void handleRecord();

    int _sockfd = -1;
    int _aliveCount = 0;
    bool _isClosed=false;

    RTSP* _rtsp;
    xop::EventLoop* _loop;
    enum ConnectionMode _connMode = RTSP_SERVER;
    MediaSessionId _sessionId = 0;

    std::shared_ptr<xop::Channel> _channel;
    std::shared_ptr<RtspRequest> _rtspRequest;
    std::shared_ptr<RtspResponse> _rtspResponse;
    std::shared_ptr<xop::BufferReader> _readBuffer;
    std::shared_ptr<xop::BufferWriter> _writeBuffer;
    std::shared_ptr<RtpConnection> _rtpConnection;
    std::shared_ptr<xop::Channel> _rtcpChannel[MAX_MEDIA_CHANNEL];

    CloseCallback _closeCallback;
};

}

#endif
