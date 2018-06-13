// PHZ
// 2018-6-10

#include "RtspConnection.h"
#include "RtspServer.h"
#include "xop/SocketUtil.h"
#include "MediaSession.h"
#include "MediaSource.h"

#define USER_AGENT "-_-"

#define RTSP_DEBUG 0
#define MAX_RTSP_MESSAGE_SIZE 2048

using namespace xop;
using namespace std;

RtspConnection::RtspConnection(RTSP* rtsp, EventLoop* loop, int sockfd)
    : _loop(loop)
    , _rtsp(rtsp)
    , _sockfd(sockfd)
    , _channel(new Channel(sockfd))
    , _readBuffer(new BufferReader)
    , _writeBuffer(new BufferWriter(300))
    , _rtspRequest(new RtspRequest)
    , _rtspResponse(new RtspResponse)
    , _rtpConnection(new RtpConnection(this))
{
    _channel->setReadCallback([this]() { this->handleRead(); });
    _channel->setWriteCallback([this]() { this->handleWrite(); });
    _channel->setCloseCallback([this]() { this->handleClose(); });
    _channel->setErrorCallback([this]() { this->handleError(); });

    SocketUtil::setNonBlock(_sockfd);
    SocketUtil::setSendBufSize(_sockfd, 100*1024);
    SocketUtil::setKeepAlive(_sockfd);

    _channel->enableReading();
    _loop->updateChannel(_channel);

    for(int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
    {
        _rtcpChannel[chn] = nullptr;
    }
}

RtspConnection::~RtspConnection()
{
	if(_sockfd > 0)
    {
        SocketUtil::close(_sockfd);
    }
}

void RtspConnection::handleRead()
{
    if(_isClosed)
        return ;

    keepAlive(); // 心跳计数, 未加入RTCP解析

    int ret = _readBuffer->readFd(_sockfd);
    if (ret > 0 && _connMode == RTSP_SERVER)
    {
        if (!handleRtspRequest())
        {
            handleClose();
            return;
        }
    }
    else if (ret > 0 && _connMode == RTSP_CLIENT)
    {
        if (!handleRtspResponse())
        {
            handleClose();
            return;
        }
    }

    if (ret <= 0)
    {
        handleClose();
    }

    if (_readBuffer->readableBytes() > MAX_RTSP_MESSAGE_SIZE)
    {
        _readBuffer->retrieveAll(); // rtcp
        //handleClose();
    }
}

void RtspConnection::handleWrite()
{
    if(_isClosed)
       return ;

    int ret = 0;
    bool empty = false;

    do
    {
        ret = _writeBuffer->send(_sockfd);
        if(ret < 0)
        {
            handleClose();
            return ;
        }
        empty = _writeBuffer->isEmpty();
    }while(!empty && ret>0);

    if(empty)
    {
        _channel->disableWriting();
        _loop->updateChannel(_channel);
    }
}

void RtspConnection::handleClose()
{
    if(!_isClosed)
    {
        _isClosed = true;
        _loop->removeChannel(_channel);

        if(_sessionId != 0)
        {
            MediaSessionPtr mediaSessionPtr = _rtsp->lookMediaSession(_sessionId);
            if(mediaSessionPtr)
            {
               mediaSessionPtr->removeClient(_sockfd);
            }
        }

        for(int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
        {
            if(_rtcpChannel[chn] && !_rtcpChannel[chn]->isNoneEvent())
            {
                _loop->removeChannel(_rtcpChannel[chn]);
            }
        }

        if(_closeCallback)
        {
            _closeCallback(_sockfd);
        }
    }
}

void RtspConnection::handleError()
{

}

bool RtspConnection::handleRtspRequest()
{
#if RTSP_DEBUG
	cout << string(_readBuffer->peek(), _readBuffer->readableBytes()) << endl;
#endif
	if (_rtspRequest->parseRequest(_readBuffer.get()))
	{
		if (!_rtspRequest->gotAll()) //解析完成
			return true;

		RtspRequest::Method method = _rtspRequest->getMethod();
		switch (method)
		{
		case RtspRequest::OPTIONS:
			handleCmdOption();
			break;
		case RtspRequest::DESCRIBE:
			handleCmdDescribe();
			break;
		case RtspRequest::SETUP:
			handleCmdSetup();
			break;
		case RtspRequest::PLAY:
			handleCmdPlay();
			break;
		case RtspRequest::TEARDOWN:
			handleCmdTeardown();
			break;
		case RtspRequest::GET_PARAMETER:
			handleCmdGetParamter();
			break;
		default:
			break;
		}

		if (_rtspRequest->gotAll())
		{
			_rtspRequest->reset();
			_readBuffer->retrieveAll();
		}
	}
	else
	{
		return false;
	}

	return true;
}

bool RtspConnection::handleRtspResponse()
{
#if RTSP_DEBUG
	cout << string(_readBuffer->peek(), _readBuffer->readableBytes()) << endl;
#endif

	if (_rtspResponse->parseResponse(_readBuffer.get()))
	{
		RtspResponse::Method method = _rtspResponse->getMethod();
		switch (method)
		{
		case RtspResponse::OPTIONS:
			sendAnnounce();
			break;
		case RtspResponse::ANNOUNCE:
			sendSetup();
			break;
		case RtspResponse::SETUP:
			sendSetup();
			break;
		case RtspResponse::RECORD:
			handleRecord();
			break;
		default:
			// RTCP
			break;
		}
	}
	else
	{
		return false;
	}

	return true;
}

void RtspConnection::sendMessage(std::shared_ptr<char> buf, uint32_t size)
{
	if (_isClosed)
		return;

#if RTSP_DEBUG
	cout << buf.get() << endl;
#endif

	_writeBuffer->append(buf, size);
	int ret = 0;
	do
	{
		ret = _writeBuffer->send(_sockfd);
		if (ret < 0)
		{
			handleClose();
			return;
		}
	} while (ret > 0 && !_writeBuffer->isEmpty());

	if (!_writeBuffer->isEmpty() && !_channel->isWriting())
	{
		_channel->enableWriting();
		_loop->updateChannel(_channel);
	}

	return;
}

void RtspConnection::handleRtcp(SOCKET sockfd)
{
    char buf[1024] = {0};
    if(recv(sockfd, buf, 1024, 0) > 0)
    {
        keepAlive();
    }
}

void RtspConnection::handleCmdOption()
{
	std::shared_ptr<char> response(new char[2048]);
    snprintf(response.get(), 2048,
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %u\r\n"
            "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY\r\n"
            "\r\n",
            _rtspRequest->getCSeq());
	sendMessage(response, strlen(response.get()));
}

void RtspConnection::handleCmdDescribe()
{
    std::shared_ptr<char> response(new char[4096]);
    MediaSessionPtr mediaSessionPtr = _rtsp->lookMediaSession(_rtspRequest->getRtspUrlSuffix());
    if(!mediaSessionPtr)
    {
        snprintf(response.get(), 4096,
                "RTSP/1.0 404 Not Found\r\n"
                "CSeq: %u\r\n"
                "\r\n",
                _rtspRequest->getCSeq());
        sendMessage(response, strlen(response.get()));
        return ;
    }
    else
    {
        // 关联媒体会话
        _sessionId = mediaSessionPtr->getMediaSessionId();
        mediaSessionPtr->addClient(_sockfd, _rtpConnection);

        for(int chn=0; chn<2; chn++)
        {
            MediaSource* source = mediaSessionPtr->getMediaSource((MediaChannelId)chn);
            if(source != nullptr)
            {
                // 设置时钟频率
                _rtpConnection->setClockRate((MediaChannelId)chn, source->getClockRate());
                // 设置媒体负载类型
                _rtpConnection->setPayloadType((MediaChannelId)chn, source->getPayloadType());
            }
        }
    }

    std::string sdp = mediaSessionPtr->getSdpMessage(_rtsp->getVersion());
    if(sdp == "")
    {
        snprintf(response.get(), 4096,
                "RTSP/1.0 500 Internal Server Error\r\n"
                "CSeq: %u\r\n"
                "\r\n",
                _rtspRequest->getCSeq());

        sendMessage(response, strlen(response.get()));
        return ;
    }

        snprintf(response.get(), 4096,
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %u\r\n"
                "Content-Length: %lu\r\n"
                "Content-Type: application/sdp\r\n"
                "\r\n"
                "%s",
                _rtspRequest->getCSeq(), strlen(sdp.c_str()), sdp.c_str());

        sendMessage(response, strlen(response.get()));
        return ;
}

void RtspConnection::handleCmdSetup()
{
    std::shared_ptr<char> response(new char[4096]);
    MediaChannelId channelId = _rtspRequest->getChannelId();

    MediaSessionPtr mediaSessionPtr = _rtsp->lookMediaSession(_sessionId);
    if(!mediaSessionPtr)
    {
        goto server_error;
    }

    if(mediaSessionPtr->isMulticast()) //会话使用组播
    {
        if(_rtspRequest->getTransportMode() == RTP_OVER_MULTICAST)
        {

            if(!_rtpConnection->setupRtpOverMulticast(channelId,
                                                     mediaSessionPtr->getMulticastSockfd(channelId),
                                                     mediaSessionPtr->getMulticastIp().c_str(),
                                                     mediaSessionPtr->getMulticastPort(channelId)))
            {
                goto server_error;
            }

			      snprintf(response.get(), 4096,
                    "RTSP/1.0 200 OK\r\n"
                    "CSeq: %u\r\n"
                    "Transport: RTP/AVP;multicast;destination=%s;source=%s;port=%u;ttl=255\r\n"
                    "Session: %u\r\n"
                    "\r\n",
                    _rtspRequest->getCSeq(),
                    mediaSessionPtr->getMulticastIp().c_str(),
                    _rtspRequest->getIp().c_str(),
                    mediaSessionPtr->getMulticastPort(channelId),
                    _rtpConnection->getRtpSessionId());
        }
        else
        {
            // 组播会不支持单播
            goto transport_unsupport;
        }
    }
    else //会话使用单播
    {
        if(_rtspRequest->getTransportMode() == RTP_OVER_TCP)
        {
            uint16_t rtpChannel = _rtspRequest->getRtpChannel();
            uint16_t rtcpChannel = _rtspRequest->getRtcpChannel();
            _rtpConnection->setupRtpOverTcp(channelId, rtpChannel, rtcpChannel);
			      snprintf(response.get(), 4096,
                    "RTSP/1.0 200 OK\r\n"
                    "CSeq: %u\r\n"
                    "Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d\r\n"
                    "Session: %u\r\n"
                    "\r\n",
                    _rtspRequest->getCSeq(),
                    rtpChannel, rtcpChannel,
                    _rtpConnection->getRtpSessionId());
        }
        else if(_rtspRequest->getTransportMode() == RTP_OVER_UDP)
        {
            uint16_t rtpPort = _rtspRequest->getRtpPort();
            uint16_t rtcpPort = _rtspRequest->getRtcpPort();
            if(_rtspRequest->getTransportMode() == RTP_OVER_MULTICAST)
            {
                rtpPort = mediaSessionPtr->getMulticastPort(channelId);
                rtcpPort = rtpPort + 1;
            }

            if(_rtpConnection->setupRtpOverUdp(channelId, rtpPort, rtcpPort))
            {
                  // 监听rtcp作为心跳
                SOCKET rtcpfd = _rtpConnection->getRtcpfd(channelId);
                _rtcpChannel[channelId].reset(new Channel(rtcpfd));
                _rtcpChannel[channelId]->setReadCallback([rtcpfd, this]() { this->handleRtcp(rtcpfd); });
                _rtcpChannel[channelId]->enableReading();
                _loop->updateChannel(_rtcpChannel[channelId]);
            }
            else
            {
                goto server_error;
            }

			         snprintf(response.get(), 4096,
                        "RTSP/1.0 200 OK\r\n"
                        "CSeq: %u\r\n"
                        "Transport: RTP/AVP;unicast;client_port=%hu-%hu;server_port=%hu-%hu\r\n"
                        "Session: %u\r\n"
                        "\r\n",
                        _rtspRequest->getCSeq(),
                        rtpPort, rtcpPort,
                        _rtpConnection->getRtpPort(channelId),
                        _rtpConnection->getRtcpPort(channelId),
                        _rtpConnection->getRtpSessionId());
        }
        else
        {
            // 单播会话不支持组播
            goto transport_unsupport;
        }
    }

	   sendMessage(response, strlen(response.get()));
    return ;

    transport_unsupport:
    snprintf(response.get(), 4096,
            "RTSP/1.0 461 Unsupported transport\r\n"
            "CSeq: %d\r\n"
            "\r\n",
            _rtspRequest->getCSeq());

	  sendMessage(response, strlen(response.get()));
    return ;

server_error:
    snprintf(response.get(), 4096,
            "RTSP/1.0 500 Internal Server Error\r\n"
            "CSeq: %u\r\n"
            "\r\n",
            _rtspRequest->getCSeq());

    sendMessage(response, strlen(response.get()));
    return ;
}

void RtspConnection::handleCmdPlay()
{
    _rtpConnection->play();

    std::shared_ptr<char> response(new char[2048]);
    snprintf(response.get(), 2048,
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Range: npt=0.000-\r\n"
            "Session: %u; timeout=60\r\n",
            _rtspRequest->getCSeq(),
            _rtpConnection->getRtpSessionId());

#if 1//def HISI
    snprintf(response.get()+strlen(response.get()), sizeof(response)-strlen(response.get()),
            "%s\r\n",
            _rtpConnection->getRtpInfo(_rtspRequest->getRtspUrl()).c_str());
#endif

    snprintf(response.get()+strlen(response.get()), 2048 - strlen(response.get()), "\r\n");
    sendMessage(response, strlen(response.get()));
}

void RtspConnection::handleCmdTeardown()
{
    _rtpConnection->teardown();

    std::shared_ptr<char> response(new char[2048]);
    snprintf(response.get(), 2048,
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Session: %u\r\n"
            "\r\n",
            _rtspRequest->getCSeq(),
            _rtpConnection->getRtpSessionId());

    sendMessage(response, strlen(response.get()));
    handleClose();
}

void RtspConnection::handleCmdGetParamter()
{
	std::shared_ptr<char> response(new char[2048]);
	snprintf(response.get(), 2048,
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Session: %u\r\n"
            "\r\n",
            _rtspRequest->getCSeq(),
            _rtpConnection->getRtpSessionId());

	sendMessage(response, strlen(response.get()));
}


void RtspConnection::sendOptions()
{
	_connMode = RTSP_CLIENT;

	std::shared_ptr<char> buf(new char[2048]);
	snprintf(buf.get(), 2048,
			"OPTIONS %s RTSP/1.0\r\n"
			"CSeq: %u\r\n"
			"User-Agent: %s\r\n"
			"\r\n",
			_rtsp->getRtspUrl().c_str(), _rtspResponse->getCSeq()+1, USER_AGENT);

	_rtspResponse->setMethod(RtspResponse::OPTIONS);
	return sendMessage(buf, strlen(buf.get()));
}

void RtspConnection::sendAnnounce()
{
	MediaSessionPtr mediaSessionPtr = _rtsp->lookMediaSession(1);
	if (!mediaSessionPtr)
	{
		handleClose();
		return;
	}
	else
	{
		// 关联媒体会话
		_sessionId = mediaSessionPtr->getMediaSessionId();
		mediaSessionPtr->addClient(_sockfd, _rtpConnection);

		for (int chn = 0; chn<2; chn++)
		{
			MediaSource* source = mediaSessionPtr->getMediaSource((MediaChannelId)chn);
			if (source != nullptr)
			{
				// 设置时钟频率
				_rtpConnection->setClockRate((MediaChannelId)chn, source->getClockRate());
				// 设置媒体负载类型
				_rtpConnection->setPayloadType((MediaChannelId)chn, source->getPayloadType());
			}
		}
	}

	std::string sdp = mediaSessionPtr->getSdpMessage(_rtsp->getVersion());
	if (sdp == "")
	{
		handleClose();
		return;
	}

	std::shared_ptr<char> buf(new char[4096]);
	snprintf(buf.get(), 4096,
			"ANNOUNCE %s RTSP/1.0\r\n"
			"Content-Type: application/sdp\r\n"
			"CSeq: %u\r\n"
			"User-Agent: %s\r\n"
			"Session: %s\r\n"
			"Content-Length: %lu\r\n"
			"\r\n"
			"%s",
			_rtsp->getRtspUrl().c_str(),
			_rtspResponse->getCSeq() + 1, USER_AGENT,
			_rtspResponse->getSession().c_str(),
			strlen(sdp.c_str()+1) ,
			sdp.c_str());

    _rtspResponse->setMethod(RtspResponse::ANNOUNCE);
    return sendMessage(buf, strlen(buf.get()));
}

void RtspConnection::sendSetup()
{
	_rtspResponse->setMethod(RtspResponse::SETUP);
	MediaSessionPtr mediaSessionPtr = _rtsp->lookMediaSession(_sessionId);
	if (!mediaSessionPtr)
	{
		handleClose();
		return;
	}

	std::shared_ptr<char> buf(new char[2048]);
	if (mediaSessionPtr->getMediaSource(channel_0) && !_rtpConnection->isSetup(channel_0))
	{
		_rtpConnection->setupRtpOverTcp(channel_0, 0, 1);

		snprintf(buf.get(), 2048,
				"SETUP %s/track0 RTSP/1.0\r\n"
				"Transport: RTP/AVP/TCP;unicast;mode=record;interleaved=0-1\r\n"
				"CSeq: %u\r\n"
				"User-Agent: %s\r\n"
				"Session: %s\r\n"
				"\r\n",
				_rtsp->getRtspUrl().c_str(), _rtspResponse->getCSeq() + 1,
				USER_AGENT, _rtspResponse->getSession().c_str());
	}
	else if (mediaSessionPtr->getMediaSource(channel_1) && !_rtpConnection->isSetup(channel_1))
	{
		_rtpConnection->setupRtpOverTcp(channel_1, 2, 3);
		snprintf(buf.get(), 2048,
				"SETUP %s/track1 RTSP/1.0\r\n"
				"Transport: RTP/AVP/TCP;unicast;mode=record;interleaved=2-3\r\n"
				"CSeq: %u\r\n"
				"User-Agent: %s\r\n"
				"Session: %s\r\n"
				"\r\n",
				_rtsp->getRtspUrl().c_str(), _rtspResponse->getCSeq() + 1,
				USER_AGENT, _rtspResponse->getSession().c_str());
	}
	else
	{
		_rtspResponse->setMethod(RtspResponse::RECORD);
		snprintf(buf.get(), 2048,
				"RECORD %s RTSP/1.0\r\n"
				"Range: npt=0.000-\r\n"
				"CSeq: %u\r\n"
				"User-Agent: %s\r\n"
				"Session: %s\r\n"
				"\r\n",
				_rtsp->getRtspUrl().c_str(), _rtspResponse->getCSeq() + 1,
				USER_AGENT, _rtspResponse->getSession().c_str());
	}

	return sendMessage(buf, strlen(buf.get()));
}

void RtspConnection::handleRecord()
{
	_rtpConnection->record();
	_rtpConnection->play();
}
