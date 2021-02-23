﻿#include "RtspServer.h"
#include "RtspConnection.h"
#include "net/SocketUtil.h"
#include "net/Logger.h"

using namespace xop;
using namespace std;

RtspServer::RtspServer(EventLoop* loop)
	: TcpServer(loop)
{

}

RtspServer::~RtspServer()
{
	
}

std::shared_ptr<RtspServer> RtspServer::Create(xop::EventLoop* loop)
{
	std::shared_ptr<RtspServer> server(new RtspServer(loop));
	return server;
}

MediaSessionId RtspServer::AddSession(std::shared_ptr<MediaSession> session)
{
    std::lock_guard<std::mutex> locker(mutex_);

    if (rtsp_suffix_map_.find(session->GetRtspUrlSuffix()) != rtsp_suffix_map_.end()) {
        return 0;
    }

    MediaSessionId sessionId = session->GetMediaSessionId();
	rtsp_suffix_map_.emplace(std::move(session->GetRtspUrlSuffix()), sessionId);
	media_sessions_.emplace(sessionId, session);

    return sessionId;
}

void RtspServer::RemoveSession(MediaSessionId sessionId)
{
    std::lock_guard<std::mutex> locker(mutex_);

    auto iter = media_sessions_.find(sessionId);
    if(iter != media_sessions_.end()) {
        rtsp_suffix_map_.erase(iter->second->GetRtspUrlSuffix());
        media_sessions_.erase(sessionId);
    }
}

MediaSessionPtr RtspServer::LookMediaSession(const std::string& suffix)
{
    std::lock_guard<std::mutex> locker(mutex_);

    auto iter = rtsp_suffix_map_.find(suffix);
    if(iter != rtsp_suffix_map_.end()) {
        MediaSessionId id = iter->second;
        return media_sessions_[id];
    }

    return nullptr;
}

MediaSessionPtr RtspServer::LookMediaSession(MediaSessionId sessionId)
{
    std::lock_guard<std::mutex> locker(mutex_);

    auto iter = media_sessions_.find(sessionId);
    if(iter != media_sessions_.end()) {
        return iter->second;
    }

    return nullptr;
}

bool RtspServer::PushFrame(MediaSessionId sessionId, MediaChannelId channelId, AVFrame frame)
{
    std::shared_ptr<MediaSession> sessionPtr = nullptr;

    {
        std::lock_guard<std::mutex> locker(mutex_);
        auto iter = media_sessions_.find(sessionId);
        if (iter != media_sessions_.end()) {
            sessionPtr = iter->second;
        }
        else {
            return false;
        }
    }

    if (sessionPtr!=nullptr && sessionPtr->GetNumClient()!=0) {
        return sessionPtr->HandleFrame(channelId, frame);
    }

    return false;
}

TcpConnection::Ptr RtspServer::OnConnect(SOCKET sockfd, std::string ip, int port)
{	
	return std::make_shared<RtspConnection>(shared_from_this(), event_loop_->GetTaskScheduler().get(), sockfd, ip, port);
}

