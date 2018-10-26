// PHZ
// 2018-6-7

#include "MediaSession.h"
#include "RtpConnection.h"
#include <cstring>
#include <ctime>
#include <random>

#include "xop/log.h"
#include "xop/NetInterface.h"
#include "xop/SocketUtil.h"

using namespace xop;
using namespace std;

std::atomic_uint MediaSession::_lastMediaSessionId(1);

MediaSession::MediaSession(std::string rtspUrlSuffxx)
    : _suffix(rtspUrlSuffxx)
    , _mediaSources(2)
    , _buffer(2)
{
	_sessionId = ++_lastMediaSessionId;

    for(int n=0; n<MAX_MEDIA_CHANNEL; n++)
    {
        _multicastSockfd[n] = 0;
        _multicastPort[n] = 0;
    }
}

MediaSession* MediaSession::createNew(std::string rtspUrlSuffxx)
{
    return new MediaSession(std::move(rtspUrlSuffxx));
}

MediaSession::~MediaSession()
{
    for(int n=0; n<MAX_MEDIA_CHANNEL; n++)
    {
        if(_multicastSockfd[n])
            SocketUtil::close(_multicastSockfd[n]);
    }
}

bool MediaSession::addMediaSource(MediaChannelId channelId, MediaSource* source)
{
    source->setSendFrameCallback([this](MediaChannelId channelId, uint8_t frameType, RtpPacketPtr& rtpPkt, uint32_t pktSize, uint8_t last, uint32_t ts)
    {
        for(auto iter=_clients.begin(); iter!=_clients.end(); iter++)
        {
            if(iter->second)
            {
                iter->second->setFrameType(frameType);
                iter->second->setRtpHeader(channelId, rtpPkt, last, ts);
                iter->second->sendRtpPacket(channelId, rtpPkt, pktSize);

                if(_isMulticast) // 组播只发送一次
                    break;
            }           
        }
    });
    _mediaSources[channelId].reset(source);

    return true;
}

bool MediaSession::removeMediaSource(MediaChannelId channelId)
{
    _mediaSources[channelId] = nullptr;
    return true;
}

bool MediaSession::startMulticast()
{
    struct sockaddr_in addr = {0}; 
    addr.sin_family = AF_INET;  
    _multicastSockfd[channel_0] = socket(AF_INET, SOCK_DGRAM, 0);
    _multicastSockfd[channel_1] = socket(AF_INET, SOCK_DGRAM, 0);

    std::random_device rd;
    uint32_t range = 0xE8FFFFFF - 0xE8000100; //  组播地址范围 [232.0.1.0, 232.255.255.255)

    for(int n=0; n<=10; n++)
    {
        addr.sin_addr.s_addr = ntohl(0xE8000100 + (rd())%range); 
        addr.sin_port = htons(rd() & 0xfffe); 
        if(::bind(_multicastSockfd[channel_0], (struct sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR)
        {
            _multicastPort[channel_0] = ntohs(addr.sin_port);
            addr.sin_port = htons(rd() & 0xfffe); 
            if(::bind(_multicastSockfd[channel_1], (struct sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR)
            {
                _multicastPort[channel_1] = ntohs(addr.sin_port);
                _multicastIp = inet_ntoa(addr.sin_addr);
                break;
            }
        }
        
        if(n == 10)
        {
            SocketUtil::close(_multicastSockfd[channel_0]);
            SocketUtil::close(_multicastSockfd[channel_1]);
            return false;
        }
    }

    _isMulticast = true;

    return true;
}

std::string MediaSession::getSdpMessage(std::string sessionName)
{
    if(_sdp != "")
        return _sdp;

    if(_mediaSources.empty())
        return "";
            
    std::string ip = NetInterface::getLocalIPAddress();
    char buf[2048] = {0};


    snprintf(buf, sizeof(buf),
            "v=0\r\n"
			"o=- 9%ld 1 IN IP4 %s\r\n"
            "t=0 0\r\n"
            "a=control:*\r\n" ,
            (long)std::time(NULL), ip.c_str()); 

    if(sessionName != "")
    {
        snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), 
                "s=%s\r\n",
                sessionName.c_str());
    }
    
    if(_isMulticast)
    {
        snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
                 "a=type:broadcast\r\n"
                 "a=rtcp-unicast: reflection\r\n");
    }
		
    for (uint32_t chn=0; chn<_mediaSources.size(); chn++)
    {
        if(_mediaSources[chn])
        {	
            if(_isMulticast)		 
            {
                snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), 
                        "%s\r\n",
                        _mediaSources[chn]->getMediaDescription(_multicastPort[chn]).c_str()); 
                     
                snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), 
                        "c=IN IP4 %s/255\r\n",
                        _multicastIp.c_str()); 
            }
            else 
            {
                snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), 
                        "%s\r\n",
                        _mediaSources[chn]->getMediaDescription(0).c_str());
            }
            
            snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), 
                    "%s\r\n",
                    _mediaSources[chn]->getAttribute().c_str());
                     
            snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),											
                    "a=control:track%d\r\n", chn);	
        }
    }

    _sdp = buf;
    return _sdp;
}

MediaSource* MediaSession::getMediaSource(MediaChannelId channelId)
{
    if(_mediaSources[channelId])
        return _mediaSources[channelId].get();

    return nullptr;
}

bool MediaSession::saveFrame(MediaChannelId channelId, AVFrame frame)
{
    return _buffer[channelId].push(move(frame));
}

bool MediaSession::handleFrame(MediaChannelId channelId)
{
    if(_mediaSources[channelId])
    {
        AVFrame frame;
        if(_buffer[channelId].pop(frame))
        {
            _mediaSources[channelId]->handleFrame(channelId, frame);
        }
    }

    return true;
}

bool MediaSession::handleFrame(MediaChannelId channelId, AVFrame frame)
{
    if(_mediaSources[channelId])
    {
        _mediaSources[channelId]->handleFrame(channelId, frame);
    }

    return true;
}

bool MediaSession::addClient(SOCKET sockfd, std::shared_ptr<RtpConnection> rtpConnPtr)
{
    auto iter = _clients.find (sockfd);
    if(iter == _clients.end())
    {
        _clients.emplace(sockfd, rtpConnPtr);
        
        if(_notifyCallback)
            _notifyCallback(_sessionId, _clients.size()); //回调通知当前客户端数量
        
        return true;
    }
            
    return false;
}

void MediaSession::removeClient(SOCKET sockfd)
{  
    _clients.erase(sockfd);
    if(_notifyCallback)        
        _notifyCallback(_sessionId, _clients.size());  //回调通知当前客户端数量
}

