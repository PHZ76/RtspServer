// PHZ
// 2018-9-30

#include "RtpConnection.h"
#include "RtspConnection.h"
#include "net/SocketUtil.h"

using namespace std;
using namespace xop;

RtpConnection::RtpConnection(RtspConnection* rtspConnection)
    : _rtspConnection(rtspConnection)
{
    std::random_device rd;

    for(int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
    {
        _rtpfd[chn] = 0;
        _rtcpfd[chn] = 0;
        memset(&_mediaChannelInfo[chn], 0, sizeof(_mediaChannelInfo[chn]));
        _mediaChannelInfo[chn].rtpHeader.version = RTP_VERSION;
        _mediaChannelInfo[chn].packetSeq = rd()&0xffff;
        _mediaChannelInfo[chn].rtpHeader.seq = 0;//htons(1);
        _mediaChannelInfo[chn].rtpHeader.ts = htonl(rd());
        _mediaChannelInfo[chn].rtpHeader.ssrc = htonl(rd());
    }
}

RtpConnection::~RtpConnection()
{
    for(int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
    {
        if(_rtpfd[chn] > 0)
        {
            SocketUtil::close(_rtpfd[chn]);
        }

        if(_rtcpfd[chn] > 0)
        {
            SocketUtil::close(_rtcpfd[chn]);
        }
    }
}

int RtpConnection::getId() const
{
	return _rtspConnection->getId();
}

bool RtpConnection::setupRtpOverTcp(MediaChannelId channelId, uint16_t rtpChannel, uint16_t rtcpChannel)
{
    _mediaChannelInfo[channelId].rtpChannel = rtpChannel;
    _mediaChannelInfo[channelId].rtcpChannel = rtcpChannel;
    _rtpfd[channelId] = _rtspConnection->fd();
    _rtcpfd[channelId] = _rtspConnection->fd();
    _mediaChannelInfo[channelId].isSetup = true;
    _transportMode = RTP_OVER_TCP;

    return true;
}

bool RtpConnection::setupRtpOverUdp(MediaChannelId channelId, uint16_t rtpPort, uint16_t rtcpPort)
{
    if(SocketUtil::getPeerAddr(_rtspConnection->fd(), &_peerAddr) < 0)
    {
        return false;
    }

    _mediaChannelInfo[channelId].rtpPort = rtpPort;
    _mediaChannelInfo[channelId].rtcpPort = rtcpPort;

    std::random_device rd;
    for (int n = 0; n <= 10; n++)
    {
        if(n == 10)
            return false;

        _localRtpPort[channelId] = rd() & 0xfffe;
        _localRtcpPort[channelId] =_localRtpPort[channelId] + 1;

        _rtpfd[channelId] = ::socket(AF_INET, SOCK_DGRAM, 0);
        if(!SocketUtil::bind(_rtpfd[channelId], "0.0.0.0",  _localRtpPort[channelId]))
        {
            SocketUtil::close(_rtpfd[channelId]);
            continue;
        }

         _rtcpfd[channelId] = ::socket(AF_INET, SOCK_DGRAM, 0);
        if(!SocketUtil::bind(_rtcpfd[channelId], "0.0.0.0", _localRtcpPort[channelId]))
        {
            SocketUtil::close(_rtpfd[channelId]);
            SocketUtil::close(_rtcpfd[channelId]);
            continue;
        }

        break;
    }

    SocketUtil::setSendBufSize(_rtpfd[channelId], 50*1024);

    _peerRtpAddr[channelId].sin_family = AF_INET;
    _peerRtpAddr[channelId].sin_addr.s_addr = _peerAddr.sin_addr.s_addr;
    _peerRtpAddr[channelId].sin_port = htons(_mediaChannelInfo[channelId].rtpPort);

    _peerRtcpAddr[channelId].sin_family = AF_INET;
    _peerRtcpAddr[channelId].sin_addr.s_addr = _peerAddr.sin_addr.s_addr;
    _peerRtcpAddr[channelId].sin_port = htons(_mediaChannelInfo[channelId].rtcpPort);

    _mediaChannelInfo[channelId].isSetup = true;
    _transportMode = RTP_OVER_UDP;

    return true;
}

bool RtpConnection::setupRtpOverMulticast(MediaChannelId channelId, std::string ip, uint16_t port)
{
    std::random_device rd;
    for (int n = 0; n <= 10; n++)
    {
        if (n == 10)
            return false;

        _localRtpPort[channelId] = rd() & 0xfffe;
        _rtpfd[channelId] = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (!SocketUtil::bind(_rtpfd[channelId], "0.0.0.0", _localRtpPort[channelId]))
        {
            SocketUtil::close(_rtpfd[channelId]);
            continue;
        }

        break;
    }

    _mediaChannelInfo[channelId].rtpPort = port;

    _peerRtpAddr[channelId].sin_family = AF_INET;
    _peerRtpAddr[channelId].sin_addr.s_addr = inet_addr(ip.c_str());
    _peerRtpAddr[channelId].sin_port = htons(port);

    _mediaChannelInfo[channelId].isSetup = true;
    _transportMode = RTP_OVER_MULTICAST;
    _isMulticast = true;
    return true;
}

void RtpConnection::play()
{
    for(int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
    {
		if (_mediaChannelInfo[chn].isSetup)
		{
			_mediaChannelInfo[chn].isPlay = true;
		}
    }
}

void RtpConnection::record()
{
	for (int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
	{
		if (_mediaChannelInfo[chn].isSetup)
		{
			_mediaChannelInfo[chn].isRecord = true;
			_mediaChannelInfo[chn].isPlay = true;
		}
	}
}

void RtpConnection::teardown()
{
    if(!_isClosed)
    {
        _isClosed = true;
        for(int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
        {
            _mediaChannelInfo[chn].isPlay = false;
			_mediaChannelInfo[chn].isRecord = false;
        }
    }
}

string RtpConnection::getMulticastIp(MediaChannelId channelId) const
{
    return std::string(inet_ntoa(_peerRtpAddr[channelId].sin_addr));
}

string RtpConnection::getRtpInfo(const std::string& rtspUrl)
{
    char buf[2048] = { 0 };
    snprintf(buf, 1024, "RTP-Info: ");

    int numChannel = 0;

    auto timePoint = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now());
    auto ts = timePoint.time_since_epoch().count();
    for (int chn = 0; chn<MAX_MEDIA_CHANNEL; chn++)
    {
        uint32_t rtpTime = (uint32_t)(ts*_mediaChannelInfo[chn].clockRate / 1000);
        if (_mediaChannelInfo[chn].isSetup)
        {
            if (numChannel != 0)
                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), ",");

            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                    "url=%s/track%d;seq=0;rtptime=%u",
                    rtspUrl.c_str(), chn, rtpTime);
            numChannel++;
        }
    }

    return std::string(buf);
}

void RtpConnection::setFrameType(uint8_t frameType)
{
    _frameType = frameType;
    if(!_hasIDRFrame && (_frameType==0 || _frameType==VIDEO_FRAME_I))
    {
		_hasIDRFrame = true;
    }
}

void RtpConnection::setRtpHeader(MediaChannelId channelId, RtpPacket pkt)
{
    if((_mediaChannelInfo[channelId].isPlay || _mediaChannelInfo[channelId].isRecord) && _hasIDRFrame)
    {
        _mediaChannelInfo[channelId].rtpHeader.marker = pkt.last;
        _mediaChannelInfo[channelId].rtpHeader.ts = htonl(pkt.timestamp);
        _mediaChannelInfo[channelId].rtpHeader.seq = htons(_mediaChannelInfo[channelId].packetSeq++);
        memcpy(pkt.data.get()+4, &_mediaChannelInfo[channelId].rtpHeader, RTP_HEADER_SIZE);
    }
}

int RtpConnection::sendRtpPacket(MediaChannelId channelId, RtpPacket pkt)
{    
    if (_isClosed)
    {
        return -1;
    }
   
    bool ret = _rtspConnection->_pTaskScheduler->addTriggerEvent([this, channelId, pkt] {        
        this->setFrameType(pkt.type);
        this->setRtpHeader(channelId, pkt);
        if((_mediaChannelInfo[channelId].isPlay || _mediaChannelInfo[channelId].isRecord) && _hasIDRFrame )
        {            
            if(_transportMode == RTP_OVER_TCP)
            {
                sendRtpOverTcp(channelId, pkt);
            }
            else //if(_transportMode == RTP_OVER_UDP || _transportMode==RTP_OVER_MULTICAST)
            {
                sendRtpOverUdp(channelId, pkt);
            }
                   
            //_mediaChannelInfo[channelId].octetCount  += pkt.size;
            //_mediaChannelInfo[channelId].packetCount += 1;
        }
    });

	return ret ? 0 : -1;
}

int RtpConnection::sendRtpOverTcp(MediaChannelId channelId, RtpPacket pkt)
{
    uint8_t* rtpPktPtr = pkt.data.get();
    rtpPktPtr[0] = '$';
    rtpPktPtr[1] = (char)_mediaChannelInfo[channelId].rtpChannel;
    rtpPktPtr[2] = (char)(((pkt.size-4)&0xFF00)>>8);
    rtpPktPtr[3] = (char)((pkt.size -4)&0xFF);

    _rtspConnection->send((char*)rtpPktPtr, pkt.size);
    return pkt.size;
}

int RtpConnection::sendRtpOverUdp(MediaChannelId channelId, RtpPacket pkt)
{
    //_mediaChannelInfo[channelId].octetCount  += pktSize;
    //_mediaChannelInfo[channelId].packetCount += 1;

    /* 去掉RTP-OVER-TCP传输的4字节header */
    int ret = sendto(_rtpfd[channelId], (const char*)pkt.data.get()+4, pkt.size-4, 0, 
					(struct sockaddr *)&(_peerRtpAddr[channelId]),
                    sizeof(struct sockaddr_in));
                   
    if(ret < 0)
    {        
        teardown();
        return -1;
    }

    return ret;
}
