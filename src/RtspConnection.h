#ifndef _RTSP_CONNECTION
#define _RTSP_CONNECTION

#include <iostream>
#include <functional>
#include <memory>
#include <vector>

#include "RtpConnection.h"
#include "RtspRequest.h"

#include "xop/BufferReader.h"
#include "xop/BufferWriter.h"
#include "xop/EventLoop.h"
#include "xop/Channel.h"

#include "media.h"

namespace xop
{

class RtspServer;
class MediaSession;

class RtspConnection : public std::enable_shared_from_this<RtspConnection>
{
public:
    typedef std::function<void (std::shared_ptr<RtspConnection> conn)> CloseCallback;

    RtspConnection() = delete;
    RtspConnection(RtspServer* server, int sockfd);
    ~RtspConnection();

    int fd() const 
    { return _sockfd; }

    MediaSessionId getMediaSessionId()
    { return _sessionId; }

    void setCloseCallback(const CloseCallback& cb)
    { _closeCallback = cb; }
	
private:
    friend class RtpConnection;
    friend class MediaSession;

    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();	
    void sendResponse(const char* data, int len);
    void handleCmdOption();
    void handleCmdDescribe();
    void handleCmdSetup();
    void handleCmdPlay();
    void handleCmdTeardown();

    int _sockfd = -1;
    bool _isClosed=false, _isPlay=false;	
    xop::EventLoop* _loop;
    RtspServer* _rtspServer;

    MediaSessionId _sessionId = 0;

    std::shared_ptr<xop::Channel> _channel;		
    std::shared_ptr<RtspRequest> _rtspRequest;
    std::shared_ptr<xop::BufferReader> _readBuffer;
    std::shared_ptr<xop::BufferWriter> _writeBuffer;
    std::shared_ptr<RtpConnection> _rtpConnection;

    CloseCallback _closeCallback;
};

}

#endif

