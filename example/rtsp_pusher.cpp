// RTSP推流器Demo, 目前只支持TCP推流, 在EasyDarwin下测试通过

#include "xop/RtspPusher.h"
#include <thread>
#include <memory>
#include <iostream>
#include <string>

#define PUSH_TEST "rtsp://192.168.43.213:554/test" //流媒体转发服务器地址

void snedFrameThread(xop::RtspPusher* rtspPusher, xop::MediaSessionId sessionId);

int main(int agrc, char **argv)
{	
    std::shared_ptr<xop::EventLoop> eventLoop(new xop::EventLoop());  
    xop::RtspPusher rtspPusher(eventLoop.get());  //创建一个RTSP推流器

    xop::MediaSession *session = xop::MediaSession::createNew(); 

    // 添加音视频流到媒体会话, track0:h264, track1:aac
    session->addMediaSource(xop::channel_0, xop::H264Source::createNew()); 
    session->addMediaSource(xop::channel_1, xop::AACSource::createNew(44100,2));

    xop::MediaSessionId sessionId = rtspPusher.addMeidaSession(session); //添加session到RtspServer后, session会失效

    if (!rtspPusher.openUrl(PUSH_TEST))
    {
        std::cout << "Open " << PUSH_TEST << " failed." << std::endl; // 连接服务器超时
        return 0;
    }

    std::cout << "Push stream to " << PUSH_TEST << " ..." << std::endl; 
        
    std::thread t1(snedFrameThread, &rtspPusher, sessionId); //开启负责音视频数据转发的线程
    t1.detach(); 

    eventLoop->loop(); //主线程运行 rtspPusher 

    getchar();
    return 0;
}

void snedFrameThread(xop::RtspPusher* rtspPusher, xop::MediaSessionId sessionId)
{       
    while(1)
    {      
        {
            /*                     
                //获取一帧 H264, 打包
                xop::AVFrame videoFrame = {0};
                videoFrame.size = video frame size;  // 视频帧大小 
                videoFrame.timestamp = H264Source::getTimeStamp(); // 时间戳, 建议使用编码器提供的时间戳
                videoFrame.buffer.reset(new char[videoFrame.size]);
                memcpy(videoFrame.buffer.get(), video frame data, videoFrame.size);					
               
                rtspPusher->pushFrame(0, xop::channel_0, videoFrame); //推流到服务器, 接口线程安全
            */
        }
                
        {				
            /*
                //获取一帧 AAC, 打包
                xop::AVFrame audioFrame = {0};
                audioFrame.size = audio frame size;  // 音频帧大小 
                audioFrame.timestamp = AACSource::getTimeStamp(44100); // 时间戳
                audioFrame.buffer.reset(new char[audioFrame.size]);
                memcpy(audioFrame.buffer.get(), audio frame data, audioFrame.size);

                rtspPusher->pushFrame(0, xop::channel_1, audioFrame); //推流到服务器, 接口线程安全
            */
        }		

        xop::Timer::sleep(1000); // 实际使用需要根据帧率计算延时!
    }
}
