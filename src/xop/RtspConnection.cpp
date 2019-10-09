// PHZ
// 2018-6-10

#include "RtspConnection.h"
#include "RtspServer.h"
#include "MediaSession.h"
#include "MediaSource.h"
#include "net/SocketUtil.h"

#define USER_AGENT "-_-"
#define RTSP_DEBUG 0
#define MAX_RTSP_MESSAGE_SIZE 2048

using namespace xop;
using namespace std;

RtspConnection::RtspConnection(Rtsp *rtsp, TaskScheduler *taskScheduler, SOCKET sockfd)
    : TcpConnection(taskScheduler, sockfd)
	, _pTaskScheduler(taskScheduler)
    , _pRtsp(rtsp)
    , _rtpChannelPtr(new Channel(sockfd))
    , _rtspRequestPtr(new RtspRequest)
    , _rtspResponsePtr(new RtspResponse)
    , _rtpConnPtr(new RtpConnection(this))
{
    this->setReadCallback([this](std::shared_ptr<TcpConnection> conn, xop::BufferReader& buffer) {
        return this->onRead(buffer);
    });

    this->setCloseCallback([this](std::shared_ptr<TcpConnection> conn) {
        this->onClose();
    });

    _aliveCount = 1;

    _rtpChannelPtr->setReadCallback([this]() { this->handleRead(); });
    _rtpChannelPtr->setWriteCallback([this]() { this->handleWrite(); });
    _rtpChannelPtr->setCloseCallback([this]() { this->handleClose(); });
    _rtpChannelPtr->setErrorCallback([this]() { this->handleError(); });

    for(int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
    {
        _rtcpChannels[chn] = nullptr;
    }

	_hasAuth = true;
	if (rtsp->_hasAuthInfo)
	{
		_hasAuth = false;
		_authInfoPtr.reset(new DigestAuthentication(rtsp->_realm, rtsp->_username, rtsp->_password));
	}
}

RtspConnection::~RtspConnection()
{

}

bool RtspConnection::onRead(BufferReader& buffer)
{
    keepAlive();

    int size = buffer.readableBytes();
    if (size <= 0)
    {
        return false; //close
    }

    if (_connMode == RTSP_SERVER)
    {
        if (!handleRtspRequest(buffer))
        {
            return false; //close
        }
    }
    else if (_connMode == RTSP_PUSHER)
    {
        if (!handleRtspResponse(buffer))
        {           
            return false; //close
        }
    }

    if (buffer.readableBytes() > MAX_RTSP_MESSAGE_SIZE)
    {
        buffer.retrieveAll(); // rtcp
    }

    return true;
}

void RtspConnection::onClose()
{
    if(_sessionId != 0)
    {
        MediaSessionPtr mediaSessionPtr = _pRtsp->lookMediaSession(_sessionId);
        if(mediaSessionPtr)
        {
            mediaSessionPtr->removeClient(this->fd());
        }
    }

    for(int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
    {
        if(_rtcpChannels[chn] && !_rtcpChannels[chn]->isNoneEvent())
        {
            _pTaskScheduler->removeChannel(_rtcpChannels[chn]);
        }
    }
}

bool RtspConnection::handleRtspRequest(BufferReader& buffer)
{
#if RTSP_DEBUG
	string str(buffer.peek(), buffer.readableBytes());
	if (str.find("rtsp") != string::npos || str.find("RTSP") != string::npos)
		cout << str << endl;
#endif

    if (_rtspRequestPtr->parseRequest(&buffer))
    {
        RtspRequest::Method method = _rtspRequestPtr->getMethod();
        if(method == RtspRequest::RTCP)
        {
            handleRtcp(buffer);
            return true;
        }
        else if(!_rtspRequestPtr->gotAll()) 
        {
            return true;
        }
        
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

        if (_rtspRequestPtr->gotAll())
        {
            _rtspRequestPtr->reset();
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool RtspConnection::handleRtspResponse(BufferReader& buffer)
{
#if RTSP_DEBUG
	string str(buffer.peek(), buffer.readableBytes());
	if(str.find("rtsp")!=string::npos || str.find("RTSP") != string::npos)
		cout << str << endl;
#endif

    if (_rtspResponsePtr->parseResponse(&buffer))
    {
        RtspResponse::Method method = _rtspResponsePtr->getMethod();
        switch (method)
        {
        case RtspResponse::OPTIONS:
            if (_connMode == RTSP_PUSHER)
                sendAnnounce();
            break;
        case RtspResponse::ANNOUNCE:
        case RtspResponse::DESCRIBE:
            sendSetup();
            break;
        case RtspResponse::SETUP:
            sendSetup();
            break;
        case RtspResponse::RECORD:
            handleRecord();
            break;
        default:            
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
#if RTSP_DEBUG
	cout << buf.get() << endl;
#endif

    this->send(buf, size);
    return;
}

void RtspConnection::handleRtcp(BufferReader& buffer)
{    
    char *peek = buffer.peek();
    if(peek[0] == '$' &&  buffer.readableBytes() > 4)
    {
        uint32_t pktSize = peek[2]<<8 | peek[3];
        if(pktSize+4 >=  buffer.readableBytes())
        {
            buffer.retrieve(pktSize+4);  /* 忽略RTCP包 */
        }
    }
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
    std::shared_ptr<char> res(new char[2048]);
    int size = _rtspRequestPtr->buildOptionRes(res.get(), 2048);
    sendMessage(res, size);	
}

void RtspConnection::handleCmdDescribe()
{
	if (_authInfoPtr!=nullptr && !handleAuthentication())
	{
		return;
	}

    std::shared_ptr<char> res(new char[4096]);
    int size = 0;

    MediaSessionPtr mediaSessionPtr = _pRtsp->lookMediaSession(_rtspRequestPtr->getRtspUrlSuffix());
    if(!mediaSessionPtr)
    {
        size = _rtspRequestPtr->buildNotFoundRes(res.get(), 4096);
    }
    else
    {
        _sessionId = mediaSessionPtr->getMediaSessionId();
        mediaSessionPtr->addClient(this->fd(), _rtpConnPtr);

        for(int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
        {
            MediaSource* source = mediaSessionPtr->getMediaSource((MediaChannelId)chn);
            if(source != nullptr)
            {
                /* 设置时钟频率 */
                _rtpConnPtr->setClockRate((MediaChannelId)chn, source->getClockRate());
				/* 设置媒体负载类型 */
                _rtpConnPtr->setPayloadType((MediaChannelId)chn, source->getPayloadType());
            }
        }

        std::string sdp = mediaSessionPtr->getSdpMessage(_pRtsp->getVersion());
        if(sdp == "")
        {
            size = _rtspRequestPtr->buildServerErrorRes(res.get(), 4096);
        }
        else
        {
            size = _rtspRequestPtr->buildDescribeRes(res.get(), 4096, sdp.c_str());		
        }
    }

    sendMessage(res, size);
    return ;
}

void RtspConnection::handleCmdSetup()
{
	if (_authInfoPtr != nullptr && !handleAuthentication())
	{
		return;
	}

    std::shared_ptr<char> res(new char[4096]);
    int size = 0;
    MediaChannelId channelId = _rtspRequestPtr->getChannelId();

    MediaSessionPtr mediaSessionPtr = _pRtsp->lookMediaSession(_sessionId);
    if(!mediaSessionPtr)
    {
        goto server_error;
    }

    if(mediaSessionPtr->isMulticast()) /* 会话使用组播 */
    {
        std::string multicastIP = mediaSessionPtr->getMulticastIp();
        if(_rtspRequestPtr->getTransportMode() == RTP_OVER_MULTICAST)
        {
            uint16_t port = mediaSessionPtr->getMulticastPort(channelId);
            uint16_t sessionId = _rtpConnPtr->getRtpSessionId();
            if (!_rtpConnPtr->setupRtpOverMulticast(channelId, multicastIP.c_str(), port))
            {
                goto server_error;
            }

            size = _rtspRequestPtr->buildSetupMulticastRes(res.get(), 4096, multicastIP.c_str(), port, sessionId);
        }
        else
        {
            goto transport_unsupport;
        }
    }
    else /* 会话使用单播 */
    {
        if(_rtspRequestPtr->getTransportMode() == RTP_OVER_TCP)
        {
            uint16_t rtpChannel = _rtspRequestPtr->getRtpChannel();
            uint16_t rtcpChannel = _rtspRequestPtr->getRtcpChannel();
            uint16_t sessionId = _rtpConnPtr->getRtpSessionId();

            _rtpConnPtr->setupRtpOverTcp(channelId, rtpChannel, rtcpChannel);
            size = _rtspRequestPtr->buildSetupTcpRes(res.get(), 4096, rtpChannel, rtcpChannel, sessionId);
        }
        else if(_rtspRequestPtr->getTransportMode() == RTP_OVER_UDP)
        {
            uint16_t cliRtpPort = _rtspRequestPtr->getRtpPort();
            uint16_t cliRtcpPort = _rtspRequestPtr->getRtcpPort();
            uint16_t sessionId = _rtpConnPtr->getRtpSessionId();

            if(_rtpConnPtr->setupRtpOverUdp(channelId, cliRtpPort, cliRtcpPort))
            {                
                SOCKET rtcpfd = _rtpConnPtr->getRtcpfd(channelId);
                _rtcpChannels[channelId].reset(new Channel(rtcpfd));
                _rtcpChannels[channelId]->setReadCallback([rtcpfd, this]() { this->handleRtcp(rtcpfd); });
                _rtcpChannels[channelId]->enableReading();
                _pTaskScheduler->updateChannel(_rtcpChannels[channelId]);
            }
            else
            {
                goto server_error;
            }

            uint16_t serRtpPort = _rtpConnPtr->getRtpPort(channelId);
            uint16_t serRtcpPort = _rtpConnPtr->getRtcpPort(channelId);
            size = _rtspRequestPtr->buildSetupUdpRes(res.get(), 4096, serRtpPort, serRtcpPort, sessionId);
        }
        else
        {          
            goto transport_unsupport;
        }
    }

    sendMessage(res, size);
    return ;

transport_unsupport:
    size = _rtspRequestPtr->buildUnsupportedRes(res.get(), 4096);
    sendMessage(res, size);
    return ;

server_error:
    size = _rtspRequestPtr->buildServerErrorRes(res.get(), 4096);
    sendMessage(res, size);
    return ;
}

void RtspConnection::handleCmdPlay()
{
	if (_authInfoPtr != nullptr && !handleAuthentication())
	{
		return;
	}

    _rtpConnPtr->play();

    uint16_t sessionId = _rtpConnPtr->getRtpSessionId();
    std::shared_ptr<char> res(new char[2048]);

    int size = _rtspRequestPtr->buildPlayRes(res.get(), 2048, nullptr, sessionId);
    sendMessage(res, size);
}

void RtspConnection::handleCmdTeardown()
{
    _rtpConnPtr->teardown();

    uint16_t sessionId = _rtpConnPtr->getRtpSessionId();
    std::shared_ptr<char> res(new char[2048]);
    int size = _rtspRequestPtr->buildTeardownRes(res.get(), 2048, sessionId);
    sendMessage(res, size);

    handleClose();
}

void RtspConnection::handleCmdGetParamter()
{
    uint16_t sessionId = _rtpConnPtr->getRtpSessionId();
    std::shared_ptr<char> res(new char[2048]);
    int size = _rtspRequestPtr->buildGetParamterRes(res.get(), 2048, sessionId);
    sendMessage(res, size);
}

bool RtspConnection::handleAuthentication()
{
	if (_authInfoPtr != nullptr && !_hasAuth)
	{
		std::string cmd = _rtspRequestPtr->MethodToString[_rtspRequestPtr->getMethod()];
		std::string url = _rtspRequestPtr->getRtspUrl();

		if (_nonce.size() > 0 && (_authInfoPtr->getResponse(_nonce, cmd, url) == _rtspRequestPtr->getAuthResponse()))
		{
			_nonce.clear();
			_hasAuth = true;
		}
		else
		{
			std::shared_ptr<char> res(new char[4096]);
			_nonce = _authInfoPtr->getNonce();
			int size = _rtspRequestPtr->buildUnauthorizedRes(res.get(), 4096, _authInfoPtr->getRealm().c_str(), _nonce.c_str());
			sendMessage(res, size);
			return false;
		}
	}

	return true;
}

void RtspConnection::sendOptions(ConnectionMode mode)
{
    _connMode = mode;
    _rtspResponsePtr->setUserAgent(USER_AGENT);
    _rtspResponsePtr->setRtspUrl(_pRtsp->getRtspUrl().c_str());

    std::shared_ptr<char> req(new char[2048]);
    int size = _rtspResponsePtr->buildOptionReq(req.get(), 2048);
    sendMessage(req, size);
}

void RtspConnection::sendAnnounce()
{
    MediaSessionPtr mediaSessionPtr = _pRtsp->lookMediaSession(1);
    if (!mediaSessionPtr)
    {
        handleClose();
        return;
    }
    else
    {
        /* 关联媒体会话 */
        _sessionId = mediaSessionPtr->getMediaSessionId();
        mediaSessionPtr->addClient(this->fd(), _rtpConnPtr);

        for (int chn = 0; chn<2; chn++)
        {
            MediaSource* source = mediaSessionPtr->getMediaSource((MediaChannelId)chn);
            if (source != nullptr)
            {
                /* 设置时钟频率 */
                _rtpConnPtr->setClockRate((MediaChannelId)chn, source->getClockRate());
				/* 设置媒体负载类型 */
                _rtpConnPtr->setPayloadType((MediaChannelId)chn, source->getPayloadType());
            }
        }
    }

    std::string sdp = mediaSessionPtr->getSdpMessage(_pRtsp->getVersion());
    if (sdp == "")
    {
        handleClose();
        return;
    }

    std::shared_ptr<char> req(new char[4096]);
    int size = _rtspResponsePtr->buildAnnounceReq(req.get(), 4096, sdp.c_str());
    sendMessage(req, size);
}

void RtspConnection::sendDescribe()
{
	std::shared_ptr<char> req(new char[2048]);
	int size = _rtspResponsePtr->buildDescribeReq(req.get(), 2048);
	sendMessage(req, size);
}

void RtspConnection::sendSetup()
{
    std::shared_ptr<char> buf(new char[2048]);
    int size = 0;

    MediaSessionPtr mediaSessionPtr = _pRtsp->lookMediaSession(_sessionId);
    if (!mediaSessionPtr)
    {
        handleClose();
        return;
    }

    if (mediaSessionPtr->getMediaSource(channel_0) && !_rtpConnPtr->isSetup(channel_0))
    {
        _rtpConnPtr->setupRtpOverTcp(channel_0, 0, 1);
        size = _rtspResponsePtr->buildSetupTcpReq(buf.get(), 2048, channel_0);
    }
    else if (mediaSessionPtr->getMediaSource(channel_1) && !_rtpConnPtr->isSetup(channel_1))
    {
        _rtpConnPtr->setupRtpOverTcp(channel_1, 2, 3);
        size = _rtspResponsePtr->buildSetupTcpReq(buf.get(), 2048, channel_1);
    }
    else
    {
        size = _rtspResponsePtr->buildRecordReq(buf.get(), 2048);
    }

    sendMessage(buf, size);
}

void RtspConnection::handleRecord()
{
	_rtpConnPtr->record();
}
