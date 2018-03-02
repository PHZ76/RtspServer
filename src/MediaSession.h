#ifndef _MEDIA_SESSION_H
#define _MEDIA_SESSION_H

#include <memory>
#include <string>
#include <vector>
#include <list>
#include <cstdint>
#include "media.h"
#include "MediaSource.h"
#include "xop/Socket.h"
#include "xop/RingBuffer.h"

namespace xop
{
class MediaSession
{
public:
	typedef std::function<void (MediaSessionId sessionId, uint32_t numClients)> NotifyCallback;
	
	static MediaSession* createNew(std::string rtspUrlSuffxx);
	~MediaSession();
	
	bool addMediaSource(MediaChannelId channelId, MediaSource* source);
	bool removeMediaSource(MediaChannelId channelId);
	
	// 启动组播, 可以不指定IP端口
	bool startMulticast(); 
	
	// 新的客户端加入, 会触发回调函数通知客户端
	void setNotifyCallback(const NotifyCallback& cb)
	{ _notifyCallback = cb; }
	
	std::string getRtspUrlSuffix() const
	{ return _suffix; }
	
	void setRtspUrlSuffix(std::string& suffix) 
	{ _suffix = suffix; }
	
	std::string getSdpMessage(); 
	
	MediaSource* getMediaSource(MediaChannelId channelId);
	
	bool getFrame(MediaChannelId channelId, AVFrame& frame);
	bool handleFrame(MediaChannelId channelId);
	
	bool addClient(SOCKET sockfd, std::shared_ptr<RtpConnection>& rtpConnPtr);
	void removeClient(SOCKET sockfd);
	
	void setMediaSessionId(MediaSessionId sessionId)
	{ _sessionId = sessionId; }
	
	MediaSessionId getMediaSessionId()
	{ return _sessionId; }
	
	uint32_t getClientNum() const
	{ return _clients.size(); }
	
	bool isMulticast() const 
	{ return _isMulticast; }
	
	std::string getMulticastIp() const 
	{ return _multicastIp; }
	
	uint16_t getMulticastPort(MediaChannelId channelId) const 
	{ return _multicastPort[channelId]; }
	
	uint16_t getMulticastSockfd(MediaChannelId channelId) const 
	{ return _multicastSockfd[channelId]; }
	
private:
	friend class MediaSource;
	friend class RtspServer;
	MediaSession(std::string rtspUrlSuffxx);
	//bool sendRtpPacket(MediaChannelId channelId, RtpPacketPtr& rtpPkt, uint32_t pktSize, uint8_t last, uint32_t ts);
	
	MediaSessionId _sessionId = 0;
	std::string _suffix; 
	std::string _sdp;
	
	std::vector<std::shared_ptr<MediaSource>> _mediaSources;
	std::vector<RingBuffer<AVFrame>> _buffer;
	
	NotifyCallback _notifyCallback;
	ClientMap _clients;
	
	// 
	bool _isMulticast = false;
	uint16_t _multicastPort[MAX_MEDIA_CHANNEL]; 
	std::string _multicastIp;
	int _multicastSockfd[MAX_MEDIA_CHANNEL];
};	

typedef std::shared_ptr<MediaSession> MediaSessionPtr;

}

#endif

