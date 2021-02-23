﻿// PHZ
// 2018-9-30

#include "MediaSession.h"
#include "RtpConnection.h"
#include <cstring>
#include <ctime>
#include <map>
#include <forward_list>
#include "net/Logger.h"
#include "net/SocketUtil.h"

using namespace xop;
using namespace std;

std::atomic_uint MediaSession::last_session_id_(1);

MediaSession::MediaSession(std::string url_suffxx)
    : suffix_(url_suffxx)
    , media_sources_(2)
    , _buffer(2)
{
	has_new_client_ = false;
	session_id_ = ++last_session_id_;

	for(int n=0; n<MAX_MEDIA_CHANNEL; n++) {
		multicast_port_[n] = 0;
	}
}

std::shared_ptr<MediaSession> MediaSession::CreateNew(std::string url_suffxx)
{
	return std::shared_ptr<MediaSession>(new MediaSession(std::move(url_suffxx)));
}

MediaSession::~MediaSession()
{
	if (multicast_ip_ != "") {
		MulticastAddr::instance().Release(multicast_ip_);
	}
}

void MediaSession::AddNotifyConnectedCallback(const NotifyConnectedCallback& cb)
{
    _notifyConnectedCallbacks.push_back(cb);
}

void MediaSession::AddNotifyDisconnectedCallback(const NotifyDisconnectedCallback& cb) 
{
    _notifyDisconnectedCallbacks.push_back(cb);
}

bool MediaSession::AddSource(MediaChannelId channelId,  std::unique_ptr<MediaSource>&& source)
{
	source->SetSendFrameCallback([this](MediaChannelId channelId, RtpPacket pkt) {
		std::forward_list<std::shared_ptr<RtpConnection>> clients;
		std::map<int, RtpPacket> packets;
		{
			std::lock_guard<std::mutex> lock(map_mutex_);
			for (auto iter = clients_.begin(); iter != clients_.end();) {
				auto conn = iter->second.lock();
				if (conn == nullptr) {
					clients_.erase(iter++);
				}
				else  {				
					int id = conn->GetId();
					if (id >= 0) {
						if (packets.find(id) == packets.end()) {
							RtpPacket tmpPkt;
							memcpy(tmpPkt.data.get(), pkt.data.get(), pkt.size);
							tmpPkt.size = pkt.size;
							tmpPkt.last = pkt.last;
							tmpPkt.timestamp = pkt.timestamp;
							tmpPkt.type = pkt.type;
							packets.emplace(id, tmpPkt);
						}
						clients.emplace_front(conn);
					}
					iter++;
				}
			}
		}
        
		int count = 0;
		for(auto iter : clients) {
			int ret = 0;
			int id = iter->GetId();
			if (id >= 0) {
				auto iter2 = packets.find(id);
				if (iter2 != packets.end()) {
					count++;
					ret = iter->SendRtpPacket(channelId, iter2->second);
					if (is_multicast_ && ret == 0) {
						break;
					}				
				}
			}					
		}
		return true;
    });

	media_sources_[channelId] = std::move(source);
	return true;
}

bool MediaSession::RemoveSource(MediaChannelId channelId)
{
	media_sources_[channelId] = nullptr;
	return true;
}

bool MediaSession::StartMulticast()
{  
	if (is_multicast_) {
		return true;
	}

	multicast_ip_ = MulticastAddr::instance().GetAddr();
	if (multicast_ip_ == "") {
		return false;
	}

	std::random_device rd;
	multicast_port_[channel_0] = htons(rd() & 0xfffe);
	multicast_port_[channel_1] = htons(rd() & 0xfffe);

	is_multicast_ = true;
	return true;
}

std::string MediaSession::GetSdpMessage(std::string ip, std::string sessionName)
{
	if (sdp_ != "") {
		return sdp_;
	}
    
	if (media_sources_.empty()) {
		return "";
	}

	char buf[2048] = {0};

	snprintf(buf, sizeof(buf),
			"v=0\r\n"
			"o=- 9%ld 1 IN IP4 %s\r\n"
			"t=0 0\r\n"
			"a=control:*\r\n" ,
			(long)std::time(NULL), ip.c_str()); 

	if(sessionName != "") {
		snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), 
				"s=%s\r\n",
				sessionName.c_str());
	}
    
	if(is_multicast_) {
		snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
					"a=type:broadcast\r\n"
					"a=rtcp-unicast: reflection\r\n");
	}
		
	for (uint32_t chn=0; chn<media_sources_.size(); chn++) {
		if(media_sources_[chn]) {	
			if(is_multicast_) {
				snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), 
						"%s\r\n",
						media_sources_[chn]->GetMediaDescription(multicast_port_[chn]).c_str()); 
                     
				snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), 
						"c=IN IP4 %s/255\r\n",
						multicast_ip_.c_str()); 
			}
			else {
				snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), 
						"%s\r\n",
						media_sources_[chn]->GetMediaDescription(0).c_str());
			}
            
			snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), 
					"%s\r\n",
					media_sources_[chn]->GetAttribute().c_str());
                     
			snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),											
					"a=control:track%d\r\n", chn);	
		}
	}

	sdp_ = buf;
	return sdp_;
}

MediaSource* MediaSession::GetMediaSource(MediaChannelId channelId)
{
	if (media_sources_[channelId]) {
		return media_sources_[channelId].get();
	}

	return nullptr;
}

bool MediaSession::HandleFrame(MediaChannelId channelId, AVFrame frame)
{
	std::lock_guard<std::mutex> lock(mutex_);

	if(media_sources_[channelId]) {
		media_sources_[channelId]->HandleFrame(channelId, frame);
	}
	else {
		return false;
	}

	return true;
}

bool MediaSession::AddClient(SOCKET rtspfd, std::shared_ptr<RtpConnection> rtp_conn)
{
	std::lock_guard<std::mutex> lock(map_mutex_);

	auto iter = clients_.find (rtspfd);
	if(iter == clients_.end()) {
		std::weak_ptr<RtpConnection> rtp_conn_weak_ptr = rtp_conn;
		clients_.emplace(rtspfd, rtp_conn_weak_ptr);
		for (auto& cb : _notifyConnectedCallbacks)
			cb(session_id_, (uint32_t)clients_.size(), rtp_conn->GetIp());
        
		has_new_client_ = true;
		return true;
	}
            
	return false;
}

void MediaSession::RemoveClient(SOCKET rtspfd)
{  
	std::lock_guard<std::mutex> lock(map_mutex_);
    	auto it = clients_.find(rtspfd);
    	if (it != clients_.end())
    	{
                auto conn = it->second.lock();
                if (conn) {
                    for (auto& cb : _notifyDisconnectedCallbacks)
                        cb(session_id_, (uint32_t)clients_.size() - 1, conn->GetIp());
                }
		clients_.erase(it);
    	}
}

