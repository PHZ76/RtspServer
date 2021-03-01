#include "RtspPusher.h"
#include "RtspConnection.h"
#include "net/Logger.h"
#include "net/TcpSocket.h"
#include "net/Timestamp.h"
#include <memory>

using namespace xop;

RtspPusher::RtspPusher(xop::EventLoop *event_loop)
	: event_loop_(event_loop)
{

}

RtspPusher::~RtspPusher()
{
	this->Close();
}

std::shared_ptr<RtspPusher> RtspPusher::Create(xop::EventLoop* loop)
{
	std::shared_ptr<RtspPusher> pusher(new RtspPusher(loop));
	return pusher;
}

void RtspPusher::AddSession(MediaSession* session)
{
    std::lock_guard<std::mutex> locker(mutex_);
    media_session_.reset(session);
}

void RtspPusher::RemoveSession(MediaSessionId sessionId)
{
	std::lock_guard<std::mutex> locker(mutex_);
	media_session_ = nullptr;
}

MediaSession::Ptr RtspPusher::LookMediaSession(MediaSessionId session_id)
{
	return media_session_;
}

int RtspPusher::OpenUrl(std::string url, int msec)
{
	std::lock_guard<std::mutex> lock(mutex_);

	static xop::Timestamp timestamp;
	int timeout = msec;
	if (timeout <= 0) {
		timeout = 10000;
	}

	timestamp.Reset();

	if (!this->ParseRtspUrl(url)) {
		LOG_ERROR("rtsp url(%s) was illegal.\n", url.c_str());
		return -1;
	}

	if (rtsp_conn_ != nullptr) {
		std::shared_ptr<RtspConnection> rtspConn = rtsp_conn_;
		SOCKET sockfd = rtspConn->GetSocket();
		task_scheduler_->AddTriggerEvent([sockfd, rtspConn]() {
			rtspConn->Disconnect();
		});
		rtsp_conn_ = nullptr;
	}

	TcpSocket tcpSocket;
	tcpSocket.Create();
	if (!tcpSocket.Connect(rtsp_url_info_.ip, rtsp_url_info_.port, timeout))
	{
		tcpSocket.Close();
		return -1;
	}

	task_scheduler_ = event_loop_->GetTaskScheduler().get();
	rtsp_conn_.reset(new RtspConnection(shared_from_this(), task_scheduler_, tcpSocket.GetSocket()));
    event_loop_->AddTriggerEvent([this]() {
		rtsp_conn_->SendOptions(RtspConnection::RTSP_PUSHER);
    });

	timeout -= (int)timestamp.Elapsed();
	if (timeout < 0) {
		timeout = 1000;
	}

	do
	{
		xop::Timer::Sleep(100);
		timeout -= 100;
	} while (!rtsp_conn_->IsRecord() && timeout > 0);

	if (!rtsp_conn_->IsRecord()) {
		std::shared_ptr<RtspConnection> rtspConn = rtsp_conn_;
		SOCKET sockfd = rtspConn->GetSocket();
		task_scheduler_->AddTriggerEvent([sockfd, rtspConn]() {
			rtspConn->Disconnect();
		});
		rtsp_conn_ = nullptr;
		return -1;
	}

	return 0;
}

void RtspPusher::Close()
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (rtsp_conn_ != nullptr) {
		std::shared_ptr<RtspConnection> rtsp_conn = rtsp_conn_;
		SOCKET sockfd = rtsp_conn->GetSocket();
		task_scheduler_->AddTriggerEvent([sockfd, rtsp_conn]() {
			rtsp_conn->Disconnect();
		});
		rtsp_conn_ = nullptr;
	}
}

bool RtspPusher::IsConnected()
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (rtsp_conn_ != nullptr) {
		return (!rtsp_conn_->IsClosed());
	}
	return false;
}

bool RtspPusher::PushFrame(MediaChannelId channelId, AVFrame frame)
{
	std::lock_guard<std::mutex> locker(mutex_);
	if (!media_session_ || !rtsp_conn_) {
		return false;
	}

	return media_session_->HandleFrame(channelId, frame);
}