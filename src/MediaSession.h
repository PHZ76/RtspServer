// PHZ
// 2018-6-8

#ifndef XOP_MEDIA_SESSION_H
#define XOP_MEDIA_SESSION_H

#include <memory>
#include <string>
#include <vector>
#include <list>
#include <atomic>
#include <cstdint>
#include "media.h"
#include "H264Source.h"
#include "H265Source.h"
#include "G711ASource.h"
#include "AACSource.h"
#include "MediaSource.h"
#include "xop/Socket.h"
#include "xop/RingBuffer.h"

namespace xop
{

class MediaSession
{
public:
    typedef std::function<void (MediaSessionId sessionId, uint32_t numClients)> NotifyCallback;

    static MediaSession* createNew(std::string rtspUrlSuffxx=" ");
    ~MediaSession();

    bool addMediaSource(MediaChannelId channelId, MediaSource* source);
    bool removeMediaSource(MediaChannelId channelId);

    // 启动组播, IP端口随机生成
    bool startMulticast();

    // 新的客户端加入, 会触发回调函数通知客户端
    void setNotifyCallback(const NotifyCallback& cb)
    { _notifyCallback = cb; }

    std::string getRtspUrlSuffix() const
    { return _suffix; }

    void setRtspUrlSuffix(std::string& suffix)
    { _suffix = suffix; }

    std::string getSdpMessage(std::string sessionName="");

    MediaSource* getMediaSource(MediaChannelId channelId);

    bool saveFrame(MediaChannelId channelId, AVFrame frame);
    bool handleFrame(MediaChannelId channelId);
    bool handleFrame(MediaChannelId channelId, AVFrame frame);

    bool addClient(SOCKET sockfd, std::shared_ptr<RtpConnection> rtpConnPtr);
    void removeClient(SOCKET sockfd);

    MediaSessionId getMediaSessionId()
    { return _sessionId; }

    uint32_t getClientNum() const
    { return _clients.size(); }

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

    uint16_t getMulticastSockfd(MediaChannelId channelId) const
    {
        if(channelId >= MAX_MEDIA_CHANNEL)
            return 0;
        return _multicastSockfd[channelId];
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
    std::map<SOCKET, std::shared_ptr<RtpConnection>> _clients;

    //
    bool _isMulticast = false;
    uint16_t _multicastPort[MAX_MEDIA_CHANNEL];
    std::string _multicastIp;
    int _multicastSockfd[MAX_MEDIA_CHANNEL];

    static std::atomic_uint _lastMediaSessionId;
};

typedef std::shared_ptr<MediaSession> MediaSessionPtr;

}

#endif
