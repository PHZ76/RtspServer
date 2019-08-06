#include "RtspPusher.h"
#include "RtspConnection.h"
#include "net/Logger.h"
#include "net/TcpSocket.h"

using namespace xop;

RtspPusher::RtspPusher(xop::EventLoop *eventLoop)
	: _eventLoop(eventLoop)
{

}

RtspPusher::~RtspPusher()
{
	
}

MediaSessionId RtspPusher::addMeidaSession(MediaSession* session)
{
    std::lock_guard<std::mutex> locker(_mutex);
    _mediaSessionPtr.reset(session);
    return _mediaSessionPtr->getMediaSessionId();
}

void RtspPusher::removeMeidaSession(MediaSessionId sessionId)
{
    std::lock_guard<std::mutex> locker(_mutex);
    _mediaSessionPtr = nullptr;
}

MediaSessionPtr RtspPusher::lookMediaSession(MediaSessionId sessionId)
{
    std::lock_guard<std::mutex> locker(_mutex);
    return _mediaSessionPtr;
}

int RtspPusher::openUrl(std::string url)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!this->parseRtspUrl(url))
    {
        LOG_ERROR("rtsp url:%s was illegal.\n", url.c_str());
        return -1;
    }

    TcpSocket tcpSocket;
    tcpSocket.create();
    if (!tcpSocket.connect(_rtspUrlInfo.ip, _rtspUrlInfo.port, 3000)) // 3s timeout
    {
        tcpSocket.close();
        return -1;
    }

    SOCKET sockfd = tcpSocket.fd();
    _eventLoop->addTriggerEvent([sockfd, this]() {
        std::shared_ptr<RtspConnection> rtspConn = this->newConnection(sockfd);
        rtspConn->sendOptions(RtspConnection::RTSP_PUSHER);
    });

    return 0;
}

void RtspPusher::close()
{
    std::lock_guard<std::mutex> locker(_mutex);
    for (auto iter : _connections)
    {
        iter.second->handleClose();
    }
    _connections.clear();
}

std::shared_ptr<RtspConnection> RtspPusher::newConnection(SOCKET sockfd)
{
    std::shared_ptr<RtspConnection> rtspConn(new RtspConnection(this, _eventLoop->getTaskScheduler().get(), sockfd));
    rtspConn->setCloseCallback([this](std::shared_ptr<TcpConnection> conn) {
        SOCKET sockfd = conn->fd();
        if (!_eventLoop->addTriggerEvent([sockfd, this]() {this->removeConnection(sockfd); }))
        {
            _eventLoop->addTimer([sockfd, this]() { this->removeConnection(sockfd); return false; }, 1);
        }
    });

    auto conn = rtspConn;
    _connections.emplace(sockfd, conn);
    return rtspConn;
}

void RtspPusher::removeConnection(SOCKET sockfd)
{	
    std::lock_guard<std::mutex> locker(_mutex);
    _connections.erase(sockfd);
}

bool RtspPusher::pushFrame(MediaSessionId sessionId, MediaChannelId channelId, AVFrame frame)
{
    std::lock_guard<std::mutex> locker(_mutex);
    if (_mediaSessionPtr == nullptr)
    {
        return false;
    }

    return _mediaSessionPtr->handleFrame(channelId, frame);
}