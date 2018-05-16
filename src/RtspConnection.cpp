// PHZ
// 2018-5-16

#include "RtspConnection.h"
#include "RtspServer.h"
#include "xop/SocketUtil.h"
#include "MediaSession.h"
#include "MediaSource.h"

#define RTSP_DEBUG 0
#define MAX_RTSP_MESSAGE_SIZE 2048

using namespace xop;
using namespace std;

RtspConnection::RtspConnection(RtspServer* server, int sockfd)
    : _loop(server->_loop)
    , _rtspServer(server)
    , _sockfd(sockfd)
    , _channel(new Channel(sockfd))
    , _readBuffer(new BufferReader)
    , _writeBuffer(new BufferWriter(300))
    , _rtspRequest(new RtspRequest)
    , _rtpConnection(new RtpConnection(this))
{
    _channel->setReadCallback([this]() { this->handleRead(); });
    _channel->setWriteCallback([this]() { this->handleWrite(); });
    _channel->setCloseCallback([this]() { this->handleClose(); });
    _channel->setErrorCallback([this]() { this->handleError(); }); 
    
    SocketUtil::setNonBlock(_sockfd);
    SocketUtil::setSendBufSize(_sockfd, 50*1024);
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
    if(ret > 0 && (!_isPlay))
    {
#if RTSP_DEBUG	
        cout << _readBuffer->peek() << endl << endl;
#endif			
        if(_rtspRequest->parseRequest(_readBuffer.get()))
        {
            if(!_rtspRequest->gotAll()) //解析完成
                return ;
                
            RtspRequest::Method method = _rtspRequest->getMethod();
            switch(method) 
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
                case RtspRequest::RTCP:                     
                     break;
                default:																			
                     break;
            }
            
            if(_rtspRequest->gotAll())
            {
                _rtspRequest->reset();
                _readBuffer->retrieveAll();
            }                                               
        }
        else
        {
            handleClose();
        }
    }
    else if(ret <= 0)
    {
        handleClose();
    }

    if(_readBuffer->readableBytes() > MAX_RTSP_MESSAGE_SIZE)
    {
        _readBuffer->retrieveAll();
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
        _isClosed = true; // handleClose只能调用一次
        _loop->removeChannel(_channel);	

        if(_sessionId != 0)
        {
            MediaSessionPtr mediaSessionPtr = _rtspServer->lookMediaSession(_sessionId);	        
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

void RtspConnection::handleRtcp(SOCKET sockfd)
{
    char buf[1024] = {0};
    if(recv(sockfd, buf, 1024, 0) > 0)
    {
        keepAlive();
    }
}

void RtspConnection::sendResponse(const char* data, int len)
{
    if(_isClosed)
        return ;

    int bytesSend = 0;
    if(_writeBuffer->isEmpty())	// 缓冲区没有数据, 直接发送
    {
        bytesSend = ::send(_sockfd, (char*)data, len, 0);
        if(bytesSend < 0)
        {
#if defined(__linux) || defined(__linux__) 
            if(errno != EINTR && errno != EAGAIN)	
#else 
            if(WSAGetLastError() != EWOULDBLOCK)
#endif
            {
                handleClose();
                return ;
            }
        }
        
        if(len == bytesSend)
        {
            return ;
        }
    }

    // 添加到缓冲区再发送
    _writeBuffer->append(data, len, bytesSend);
    bytesSend = _writeBuffer->send(_sockfd); 
    if(bytesSend < 0)
    {			
        handleClose();
        return ;
    }	

    if(!_writeBuffer->isEmpty() && !_channel->isWriting())
    {
        _channel->enableWriting();
        _loop->updateChannel(_channel);	
    }

    return ;
}

void RtspConnection::handleCmdOption()
{
    char response[2048] = {0};

    snprintf(response, sizeof(response),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %u\r\n"
            "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY\r\n"
            "\r\n",
            _rtspRequest->getCSeq());
#if RTSP_DEBUG		
    cout << response << endl;
#endif
    sendResponse(response, strlen(response));
}

void RtspConnection::handleCmdDescribe()
{
    char response[2048] = {0};

    MediaSessionPtr mediaSessionPtr = _rtspServer->lookMediaSession(_rtspRequest->getRtspUrlSuffix());	
    if(!mediaSessionPtr)
    {
        snprintf(response, sizeof(response),
                "RTSP/1.0 404 Not Found\r\n"
                "CSeq: %u\r\n"
                "\r\n",
                _rtspRequest->getCSeq());
#if RTSP_DEBUG					
        cout << response << endl;	
#endif		
        sendResponse(response, strlen(response));
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

    std::string sdp = mediaSessionPtr->getSdpMessage(_rtspServer->getVersion());
    if(sdp == "")
    {
        snprintf(response, sizeof(response),
                "RTSP/1.0 500 Internal Server Error\r\n"
                "CSeq: %u\r\n"
                "\r\n",
                _rtspRequest->getCSeq());
#if RTSP_DEBUG					
        cout << response << endl;	
#endif		
        sendResponse(response, strlen(response));
        return ;
    }

    snprintf(response, sizeof(response),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %u\r\n"
            "Content-Length: %lu\r\n"
            "Content-Type: application/sdp\r\n"
            "\r\n"
            "%s",
            _rtspRequest->getCSeq(), strlen(sdp.c_str()), sdp.c_str());
#if RTSP_DEBUG				
    cout << response << endl;
#endif	
    sendResponse(response, strlen(response)); 

    return ;
}

void RtspConnection::handleCmdSetup()
{
    char response[2048] = {0};
    MediaChannelId channelId = _rtspRequest->getChannelId();   
    
    MediaSessionPtr mediaSessionPtr = _rtspServer->lookMediaSession(_sessionId);
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
            
            snprintf(response, sizeof(response),
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
            snprintf(response, sizeof(response),
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
            
            snprintf(response, sizeof(response),
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

#if RTSP_DEBUG		
    cout << response << endl;
#endif	
    sendResponse(response, strlen(response));		
    return ;

    transport_unsupport:
    snprintf(response, sizeof(response),
            "RTSP/1.0 461 Unsupported transport\r\n"
            "CSeq: %d\r\n"
            "\r\n",
            _rtspRequest->getCSeq()); 
#if RTSP_DEBUG		
    cout << response << endl;
#endif	
    sendResponse(response, strlen(response));		
    return ;

server_error:
    snprintf(response, sizeof(response),
            "RTSP/1.0 500 Internal Server Error\r\n"
            "CSeq: %u\r\n"
            "\r\n",
            _rtspRequest->getCSeq());
#if RTSP_DEBUG					
    cout << response << endl;
#endif			
    sendResponse(response, strlen(response));
    return ;
}

void RtspConnection::handleCmdPlay()
{	
    _rtpConnection->play();

    char response[2048] = {0};
    snprintf(response, sizeof(response),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Range: npt=0.000-\r\n"
            "Session: %u; timeout=60\r\n",                      
            _rtspRequest->getCSeq(), 
            _rtpConnection->getRtpSessionId());

#ifdef HISI
    snprintf(response+strlen(response), sizeof(response)-strlen(response), 
            "%s\r\n",  
            _rtpConnection->getRtpInfo(_rtspRequest->getRtspUrl()).c_str());
#endif             
            
    snprintf(response+strlen(response), sizeof(response)-strlen(response), "\r\n");
#if RTSP_DEBUG		
    cout << response << endl;
#endif	
    sendResponse(response, strlen(response));		
}

void RtspConnection::handleCmdTeardown()
{
    _rtpConnection->teardown();

    char response[2048] = {0};

    snprintf(response, sizeof(response),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Session: %u\r\n"
            "\r\n",
            _rtspRequest->getCSeq(), 
            _rtpConnection->getRtpSessionId());
#if RTSP_DEBUG		
    cout << response << endl;
#endif	
    sendResponse(response, strlen(response));	
    handleClose();
}

void RtspConnection::handleCmdGetParamter()
{
    char response[2048] = {0};

    snprintf(response, sizeof(response),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Session: %u\r\n"
            "\r\n",
            _rtspRequest->getCSeq(), 
            _rtpConnection->getRtpSessionId());
#if RTSP_DEBUG		
    cout << response << endl;
#endif	
    sendResponse(response, strlen(response));	
}


