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
    typedef std::function<void (SOCKET sockfd)> CloseCallback;

    RtspConnection() = delete;
    RtspConnection(RtspServer* server, int sockfd);
    ~RtspConnection();

    int fd() const 
    { return _sockfd; }

    MediaSessionId getMediaSessionId()
    { return _sessionId; }

    void setCloseCallback(const CloseCallback& cb)
    { _closeCallback = cb; }
	
    void keepAlive() 
    { _aliveCount++; }
    
    bool isAlive() const 
    { 
        if(_rtpConnection) 
        {
            // 组播暂时不加入心跳检测
            if(_rtpConnection->isMulticast())
                return true;
        }
        
        return (_aliveCount > 0); 
    }
    
    void resetAliveCount() 
    { _aliveCount = 0; }
    
private:
    friend class RtpConnection;
    friend class MediaSession;
    friend class RtspServer;
     
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();	
    void handleRtcp(SOCKET sockfd);
    void sendResponse(const char* data, int len);
    void handleCmdOption();
    void handleCmdDescribe();
    void handleCmdSetup();
    void handleCmdPlay();
    void handleCmdTeardown();
    void handleCmdGetParamter();
    
    int _sockfd = -1;
    int _aliveCount = 0;
    bool _isClosed=false, _isPlay=false;	
    xop::EventLoop* _loop;
    RtspServer* _rtspServer;

    MediaSessionId _sessionId = 0;

    std::shared_ptr<xop::Channel> _channel;		
    std::shared_ptr<RtspRequest> _rtspRequest;
    std::shared_ptr<xop::BufferReader> _readBuffer;
    std::shared_ptr<xop::BufferWriter> _writeBuffer;
    std::shared_ptr<RtpConnection> _rtpConnection;
    
    std::shared_ptr<xop::Channel> _rtcpChannel[MAX_MEDIA_CHANNEL];

    CloseCallback _closeCallback;
};

}

#endif

