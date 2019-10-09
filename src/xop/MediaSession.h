// PHZ
// 2018-6-8

#ifndef XOP_MEDIA_SESSION_H
#define XOP_MEDIA_SESSION_H

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <random>
#include <cstdint>
#include <unordered_set>
#include "media.h"
#include "H264Source.h"
#include "H265Source.h"
#include "G711ASource.h"
#include "AACSource.h"
#include "MediaSource.h"
#include "net/Socket.h"
#include "net/RingBuffer.h"

namespace xop
{

class RtpConnection;

class MediaSession
{
public:
    typedef std::function<void (MediaSessionId sessionId, uint32_t numClients)> NotifyCallback;

    static MediaSession* createNew(std::string rtspUrlSuffxx=" ");
    ~MediaSession();

    bool addMediaSource(MediaChannelId channelId, MediaSource* source);
    bool removeMediaSource(MediaChannelId channelId);

    /* 启动组播, IP端口随机生成 */
    bool startMulticast();

	/* 新的客户端加入, 会触发回调函数通知客户端 */
    void setNotifyCallback(const NotifyCallback& cb)
    { _notifyCallback = cb; }

    std::string getRtspUrlSuffix() const
    { return _suffix; }

    void setRtspUrlSuffix(std::string& suffix)
    { _suffix = suffix; }

    std::string getSdpMessage(std::string sessionName="");
    MediaSource* getMediaSource(MediaChannelId channelId); 
    bool handleFrame(MediaChannelId channelId, AVFrame frame);
    bool addClient(SOCKET rtspfd, std::shared_ptr<RtpConnection> rtpConnPtr);
    void removeClient(SOCKET rtspfd);

    MediaSessionId getMediaSessionId()
    { return _sessionId; }

    uint32_t getNumClient() const
    { return (uint32_t)_clients.size(); }

    bool isMulticast() const
    { return _isMulticast; }

    std::string getMulticastIp() const
    { return _multicastIp; }

    uint16_t getMulticastPort(MediaChannelId channelId) const
    {
        if(channelId >= MAX_MEDIA_CHANNEL)
            return 0;
        return _multicastPort[channelId];
    }

private:
    friend class MediaSource;
    friend class RtspServer;
    MediaSession(std::string rtspUrlSuffxx);

    MediaSessionId _sessionId = 0;
    std::string _suffix;
    std::string _sdp;

    std::vector<std::shared_ptr<MediaSource>> _mediaSources;
    std::vector<RingBuffer<AVFrame>> _buffer;

    NotifyCallback _notifyCallback;
    std::mutex _mutex;
    std::mutex _mtxMap;
    std::map<SOCKET, std::weak_ptr<RtpConnection>> _clients;

    bool _isMulticast = false;
    uint16_t _multicastPort[MAX_MEDIA_CHANNEL];
    std::string _multicastIp;

    std::atomic_bool _hasNewClient;

    static std::atomic_uint _lastMediaSessionId;
};

typedef std::shared_ptr<MediaSession> MediaSessionPtr;

class MulticastAddr
{
public:
	static MulticastAddr& instance()
	{
		static MulticastAddr s_multi_addr;
		return s_multi_addr;
	}

	std::string getAddr()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		std::string addrPtr;
		struct sockaddr_in addr = { 0 };
		std::random_device rd;
		for (int n = 0; n <= 10; n++)
		{
			uint32_t range = 0xE8FFFFFF - 0xE8000100;
			addr.sin_addr.s_addr = htonl(0xE8000100 + (rd()) % range);
			addrPtr = inet_ntoa(addr.sin_addr);

			if (m_addrs.find(addrPtr) != m_addrs.end())
			{
				addrPtr.clear(); 
			}
			else
			{
				m_addrs.insert(addrPtr);
				break;
			}
		}

		return addrPtr;
	}

	void release(std::string addr)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_addrs.erase(addr);
	}

private:
	std::mutex m_mutex;
	std::unordered_set<std::string> m_addrs;
};

}

#endif
