// PHZ
// 2018-6-8

#ifndef _RTSP_CONNECTION_H
#define _RTSP_CONNECTION_H

#include "net/EventLoop.h"
#include "net/TcpConnection.h"
#include "RtpConnection.h"
#include "RtspMessage.h"
#include "DigestAuthentication.h"
#include "rtsp.h"
#include <iostream>
#include <functional>
#include <memory>
#include <vector>
#include <cstdint>

namespace xop
{

class RtspServer;
class MediaSession;

class RtspConnection : public TcpConnection
{
public:
	using CloseCallback = std::function<void (SOCKET sockfd)>;

	enum ConnectionMode
	{
		RTSP_SERVER, 
		RTSP_PUSHER,
		//RTSP_CLIENT,
	};

	enum ConnectionState
	{
		START_CONNECT,
		START_PLAY,
		START_PUSH
	};

	RtspConnection() = delete;
	RtspConnection(std::shared_ptr<Rtsp> rtsp_server, TaskScheduler *task_scheduler, SOCKET sockfd);
	virtual ~RtspConnection();

	MediaSessionId GetMediaSessionId()
	{ return session_id_; }

	TaskScheduler *GetTaskScheduler() const 
	{ return task_scheduler_; }

	void KeepAlive()
	{ alive_count_++; }

	bool IsAlive() const
	{
		if (IsClosed()) {
			return false;
		}

		if(rtp_conn_ != nullptr) {
			if (rtp_conn_->IsMulticast()) {
				return true;
			}			
		}

		return (alive_count_ > 0);
	}

	void ResetAliveCount()
	{ alive_count_ = 0; }

	int GetId() const
	{ return task_scheduler_->GetId(); }

	bool IsPlay() const
	{ return conn_state_ == START_PLAY; }

	bool IsRecord() const
	{ return conn_state_ == START_PUSH; }

private:
	friend class RtpConnection;
	friend class MediaSession;
	friend class RtspServer;
	friend class RtspPusher;

	bool OnRead(BufferReader& buffer);
	void OnClose();
	void HandleRtcp(SOCKET sockfd);
	void HandleRtcp(BufferReader& buffer);   
	bool HandleRtspRequest(BufferReader& buffer);
	bool HandleRtspResponse(BufferReader& buffer);

	void SendRtspMessage(std::shared_ptr<char> buf, uint32_t size);

	void HandleCmdOption();
	void HandleCmdDescribe();
	void HandleCmdSetup();
	void HandleCmdPlay();
	void HandleCmdTeardown();
	void HandleCmdGetParamter();
	bool HandleAuthentication();

	void SendOptions(ConnectionMode mode= RTSP_SERVER);
	void SendDescribe();
	void SendAnnounce();
	void SendSetup();
	void HandleRecord();

	std::atomic_int alive_count_;
	std::weak_ptr<Rtsp> rtsp_;
	xop::TaskScheduler *task_scheduler_ = nullptr;

	ConnectionMode  conn_mode_ = RTSP_SERVER;
	ConnectionState conn_state_ = START_CONNECT;
	MediaSessionId  session_id_ = 0;

	bool has_auth_ = true;
	std::string _nonce;
	std::unique_ptr<DigestAuthentication> auth_info_;

	std::shared_ptr<Channel>       rtp_channel_;
	std::shared_ptr<Channel>       rtcp_channels_[MAX_MEDIA_CHANNEL];
	std::unique_ptr<RtspRequest>   rtsp_request_;
	std::unique_ptr<RtspResponse>  rtsp_response_;
	std::shared_ptr<RtpConnection> rtp_conn_;
};

}

#endif
