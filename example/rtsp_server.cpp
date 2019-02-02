// RTSP服务器Demo

#include "xop/RtspServer.h"
#include "net/NetInterface.h"
#include <thread>
#include <memory>
#include <iostream>
#include <string>

void snedFrameThread(xop::RtspServer* rtspServer, xop::MediaSessionId sessionId, int& clients);

int main(int agrc, char **argv)
{	
    int clients = 0; // 记录当前客户端数量
    std::string ip = "127.0.0.1";//xop::NetInterface::getLocalIPAddress(); //获取网卡ip地址
    std::string rtspUrl;

    std::shared_ptr<xop::EventLoop> eventLoop(new xop::EventLoop());  
    xop::RtspServer server(eventLoop.get(), ip, 554);  //创建一个RTSP服务器

    xop::MediaSession *session = xop::MediaSession::createNew("live"); //创建一个媒体会话, url: rtsp://ip/live
    rtspUrl = "rtsp://" + ip + "/" + session->getRtspUrlSuffix();

    // 添加音视频流到媒体会话, track0:h264, track1:aac
    session->addMediaSource(xop::channel_0, xop::H264Source::createNew()); 
    session->addMediaSource(xop::channel_1, xop::AACSource::createNew(44100,2));
    // session->startMulticast(); // 开启组播(ip,端口随机生成), 默认使用 RTP_OVER_UDP, RTP_OVER_RTSP

    // 设置通知回调函数。 在当前会话中, 客户端连接或断开会通过回调函数发起通知
    session->setNotifyCallback([&clients, &rtspUrl](xop::MediaSessionId sessionId, uint32_t numClients) {
        clients = numClients; //获取当前媒体会话客户端数量
        std::cout << "[" << rtspUrl << "]" << " Online: " << clients << std::endl;
    });

    std::cout << "URL: " <<rtspUrl << std::endl;
        
    xop::MediaSessionId sessionId = server.addMeidaSession(session); //添加session到RtspServer后, session会失效
    //server.removeMeidaSession(sessionId); //取消会话, 接口线程安全
         
    std::thread t1(snedFrameThread, &server, sessionId, std::ref(clients)); //开启负责音视频数据转发的线程
    t1.detach(); 

    eventLoop->loop(); //主线程运行 RtspServer 

    getchar();
    return 0;
}

void snedFrameThread(xop::RtspServer* rtspServer, xop::MediaSessionId sessionId, int& clients)
{       
    while(1)
    {
        if(clients > 0) // 会话有客户端在线, 发送音视频数据
        {
            {
                /*                     
                    //获取一帧 H264, 打包
                    xop::AVFrame videoFrame = {0};
                    videoFrame.type = 0; // 建议确定帧类型。I帧(xop::VIDEO_FRAME_I) P帧(xop::VIDEO_FRAME_P)
                    videoFrame.size = video frame size;  // 视频帧大小 
                    videoFrame.timestamp = H264Source::getTimeStamp(); // 时间戳, 建议使用编码器提供的时间戳
                    videoFrame.buffer.reset(new char[videoFrame.size]);                    
                    memcpy(videoFrame.buffer.get(), video frame data, videoFrame.size);					
                   
                    rtspServer->pushFrame(sessionId, xop::channel_0, videoFrame); //送到服务器进行转发, 接口线程安全
                */
            }
                    
            {				
                /*
                    //获取一帧 AAC, 打包
                    xop::AVFrame audioFrame = {0};
                    audioFrame.type = xop::AUDIO_FRAME;
                    audioFrame.size = audio frame size;  // 音频帧大小 
                    audioFrame.timestamp = AACSource::getTimeStamp(44100); // 时间戳
                    audioFrame.buffer.reset(new char[audioFrame.size]);                    
                    memcpy(audioFrame.buffer.get(), audio frame data, audioFrame.size);

                    rtspServer->pushFrame(sessionId, xop::channel_1, audioFrame); //送到服务器进行转发, 接口线程安全
                */
            }		
        }

        xop::Timer::sleep(1000); // 实际使用需要根据帧率计算延时!
    }
}
