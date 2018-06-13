// PHZ
// 2018-5-16

#include "RtpConnection.h"
#include "RtspConnection.h"
#include "xop/SocketUtil.h"

#ifdef HISI
#include "hi3516a_comm.h"
#endif

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
        _mediaChannelInfo[chn].packetSeq = 1; //rd()&0xffff;
        _mediaChannelInfo[chn].rtpHeader.seq = htons(1);
        _mediaChannelInfo[chn].rtpHeader.ts = htonl(rd());
        _mediaChannelInfo[chn].rtpHeader.ssrc = htonl(rd());
    }
}

RtpConnection::~RtpConnection()
{
	if(_transportMode != RTP_OVER_MULTICAST)
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

bool RtpConnection::setupRtpOverMulticast(MediaChannelId channelId, int sockfd, std::string ip, uint16_t port)
{
    _mediaChannelInfo[channelId].rtpPort = port;
    _rtpfd[channelId] = sockfd;

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
        if(_mediaChannelInfo[chn].isSetup)
            _mediaChannelInfo[chn].isPlay = true;
    }
}

void RtpConnection::record()
{
	for (int chn = 0; chn<MAX_MEDIA_CHANNEL; chn++)
	{
		if (_mediaChannelInfo[chn].isSetup)
			_mediaChannelInfo[chn].isRecord = true;
	}
}

void RtpConnection::teardown()
{
    if(!_isClosed)
    {
        _isClosed = true; // teardown只调用一次
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
#ifdef HISI
    char buf[1024] = {0};
    strcpy(buf, "RTP-Info: ");

    int numChannel = 0;

    HI_U64 pu64CurPts;
    HI_MPI_SYS_GetCurPts(&pu64CurPts);
    uint64_t ts = (pu64CurPts/1000) ;

    for(int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
    {
        uint32_t rtpTime = (uint32_t)(ts*_mediaChannelInfo[chn].clockRate/1000);
        if(_mediaChannelInfo[chn].isPlay)
        {
            if(numChannel != 0)
                snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), ",");

            snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
                    "url=%s/track%d;seq=0;rtptime=%u",
                    rtspUrl.c_str(), chn, rtpTime);
            numChannel++;
        }
    }

    return std::string(buf);
#else
	char buf[2048] = { 0 };
	snprintf(buf, 1024, "RTP-Info: ");

	int numChannel = 0;

	auto timePoint = chrono::time_point_cast<chrono::milliseconds>(chrono::high_resolution_clock::now());
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
#endif
}

void RtpConnection::setFrameType(uint8_t frameType)
{
    _frameType = frameType;
    if(!_isIDRFrame && (_frameType==0 || _frameType==VIDEO_FRAME_I))
    {
        _isIDRFrame = true;
    }
}

void RtpConnection::setRtpHeader(MediaChannelId channelId, RtpPacketPtr& rtpPkt, uint8_t last, uint32_t ts)
{
    if((_mediaChannelInfo[channelId].isPlay || _mediaChannelInfo[channelId].isRecord)  &&  _isIDRFrame ) //第一帧发送I帧
    {
        _mediaChannelInfo[channelId].rtpHeader.marker = last;
        _mediaChannelInfo[channelId].rtpHeader.ts = htonl(ts);
        _mediaChannelInfo[channelId].rtpHeader.seq = htons(_mediaChannelInfo[channelId].packetSeq++);
        memcpy(rtpPkt.get()+4, &_mediaChannelInfo[channelId].rtpHeader, RTP_HEADER_SIZE);
    }
}

void RtpConnection::sendRtpPacket(MediaChannelId channelId, RtpPacketPtr& rtpPkt, uint32_t pktSize)
{
    if(_isClosed)
        return ;

    if((_mediaChannelInfo[channelId].isPlay || _mediaChannelInfo[channelId].isRecord) && _isIDRFrame) //第一帧发送I帧
    {
        if(_transportMode == RTP_OVER_TCP)
        {
            sendRtpOverTcp(channelId, rtpPkt, pktSize);	// 缓存
        }
        else //if(_transportMode == RTP_OVER_UDP || _transportMode==RTP_OVER_MULTICAST)
        {
            sendRtpOverUdp(channelId, rtpPkt, pktSize);
        }
        
        // 发送统计
        _mediaChannelInfo[channelId].octetCount  += pktSize;
        _mediaChannelInfo[channelId].packetCount += 1;
    }
}

int RtpConnection::sendRtpOverTcp(MediaChannelId channelId, RtpPacketPtr rtpPkt, uint32_t pktSize)
{
    int bytesSend = 0;
    char* rtpPktPtr = rtpPkt.get();
    rtpPktPtr[0] = '$';
    rtpPktPtr[1] = (char)_mediaChannelInfo[channelId].rtpChannel;
    rtpPktPtr[2] = (char)(((pktSize-4)&0xFF00)>>8);
    rtpPktPtr[3] = (char)((pktSize-4)&0xFF);

    // 添加到缓冲区再发送
    _rtspConnection->_writeBuffer->append(rtpPkt, pktSize, bytesSend);
    int ret = 0;
    bool empty = false;
    do
    {
        ret = _rtspConnection->_writeBuffer->send(_rtpfd[channelId]);
        if(ret < 0)
        {
            teardown();
            return -1;
        }
        bytesSend += ret;
        empty = _rtspConnection->_writeBuffer->isEmpty();
    }while(!empty && ret>0);

    if(empty && !_rtspConnection->_channel->isWriting())
    {
        _rtspConnection->_channel->setEvents(EVENT_IN|EVENT_OUT);
        _rtspConnection->_loop->updateChannel(_rtspConnection->_channel);
    }

    return bytesSend;
}

int RtpConnection::sendRtpOverUdp(MediaChannelId channelId, RtpPacketPtr& rtpPkt, uint32_t pktSize)
{
    //_mediaChannelInfo[channelId].octetCount  += pktSize;
    //_mediaChannelInfo[channelId].packetCount += 1;

    // 去掉RTP-OVER-TCP传输的4字节header
    int ret = sendto(_rtpfd[channelId], rtpPkt.get()+4,
                    pktSize-4, 0, (struct sockaddr *)&(_peerRtpAddr[channelId]),
                    sizeof(struct sockaddr_in));
    if(ret < 0)
    {
        teardown();
        return -1;
    }

    return ret;
}
