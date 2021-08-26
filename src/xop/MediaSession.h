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
#include "VP8Source.h"
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
	using Ptr = std::shared_ptr<MediaSession>;
	using NotifyConnectedCallback = std::function<void (MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)> ;
	using NotifyDisconnectedCallback = std::function<void (MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)> ;

	static MediaSession* CreateNew(std::string url_suffix="live");
	virtual ~MediaSession();

	bool AddSource(MediaChannelId channel_id, MediaSource* source);
	bool RemoveSource(MediaChannelId channel_id);

	bool StartMulticast();

	void AddNotifyConnectedCallback(const NotifyConnectedCallback& callback);
	void AddNotifyDisconnectedCallback(const NotifyDisconnectedCallback& callback);

	std::string GetRtspUrlSuffix() const
	{ return suffix_; }

	void SetRtspUrlSuffix(std::string& suffix)
	{ suffix_ = suffix; }

	std::string GetSdpMessage(std::string ip, std::string session_name ="");

	MediaSource* GetMediaSource(MediaChannelId channel_id);

	bool HandleFrame(MediaChannelId channel_id, AVFrame frame);

	bool AddClient(SOCKET rtspfd, std::shared_ptr<RtpConnection> rtp_conn);
	void RemoveClient(SOCKET rtspfd);

	MediaSessionId GetMediaSessionId()
	{ return session_id_; }

	uint32_t GetNumClient() const
	{ return (uint32_t)clients_.size(); }

	bool IsMulticast() const
	{ return is_multicast_; }

	std::string GetMulticastIp() const
	{ return multicast_ip_; }

	uint16_t GetMulticastPort(MediaChannelId channel_id) const
	{
		if (channel_id >= MAX_MEDIA_CHANNEL) {
			return 0;
		}         
		return multicast_port_[channel_id];
	}

private:
	friend class MediaSource;
	friend class RtspServer;
	MediaSession(std::string url_suffxx);

	MediaSessionId session_id_ = 0;
	std::string suffix_;
	std::string sdp_;

	std::vector<std::unique_ptr<MediaSource>> media_sources_;
	std::vector<RingBuffer<AVFrame>> buffer_;

	std::vector<NotifyConnectedCallback> notify_connected_callbacks_;
	std::vector<NotifyDisconnectedCallback> notify_disconnected_callbacks_;
	std::mutex mutex_;
	std::mutex map_mutex_;
	std::map<SOCKET, std::weak_ptr<RtpConnection>> clients_;

	bool is_multicast_ = false;
	uint16_t multicast_port_[MAX_MEDIA_CHANNEL];
	std::string multicast_ip_;
	std::atomic_bool has_new_client_;

	static std::atomic_uint last_session_id_;
};

class MulticastAddr
{
public:
	static MulticastAddr& instance()
	{
		static MulticastAddr s_multi_addr;
		return s_multi_addr;
	}

	std::string GetAddr()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		std::string addr_str;
		struct sockaddr_in addr = { 0 };
		std::random_device rd;

		for (int n = 0; n <= 10; n++) {
			uint32_t range = 0xE8FFFFFF - 0xE8000100;
			addr.sin_addr.s_addr = htonl(0xE8000100 + (rd()) % range);
			addr_str = inet_ntoa(addr.sin_addr);

			if (addrs_.find(addr_str) != addrs_.end()) {
				addr_str.clear();
			}
			else {
				addrs_.insert(addr_str);
				break;
			}
		}

		return addr_str;
	}

	void Release(std::string addr) {
		std::lock_guard<std::mutex> lock(mutex_);
		addrs_.erase(addr);
	}

private:
	std::mutex mutex_;
	std::unordered_set<std::string> addrs_;
};

}

#endif
