// RTSP-Pusher

#include "xop/RtspPusher.h"
#include "net/Timer.h"
#include <thread>
#include <memory>
#include <iostream>
#include <string>

#define PUSH_TEST "rtsp://10.11.165.203:554/test" /* 推流地址 */

void snedFrameThread(xop::RtspPusher* rtspPusher);

int main(int argc, char **argv)
{	
    std::shared_ptr<xop::EventLoop> eventLoop(new xop::EventLoop());  
    xop::RtspPusher rtspPusher(eventLoop.get());  

    xop::MediaSession *session = xop::MediaSession::createNew(); 

    // 添加音视频流到媒体会话, track0:h264, track1:aac
    session->addMediaSource(xop::channel_0, xop::H264Source::createNew()); 
    session->addMediaSource(xop::channel_1, xop::AACSource::createNew(44100, 2, false));
    rtspPusher.addMeidaSession(session); /* 添加session到RtspServer后, session会失效 */

    if (rtspPusher.openUrl(PUSH_TEST, 3000) < 0)
    {
        std::cout << "Open " << PUSH_TEST << " failed." << std::endl; /* 连接服务器失败 */
		getchar();
        return 0;
    }

    std::cout << "Push stream to " << PUSH_TEST << " ..." << std::endl; 
        
    std::thread t1(snedFrameThread, &rtspPusher); /* 开启负责音视频数据转发的线程 */
    t1.detach(); 

    while (1)
	{
		xop::Timer::sleep(100);
	}

    getchar();
    return 0;
}

void snedFrameThread(xop::RtspPusher* rtspPusher)
{       
    while(rtspPusher->isConnected())
    {      
        {                  
            //获取一帧 H264, 打包
            /*
                xop::AVFrame videoFrame = {0};
                //videoFrame.size = video frame size;  // 视频帧大小 
                videoFrame.timestamp = xop::H264Source::getTimeStamp(); // 时间戳, 建议使用编码器提供的时间戳
                videoFrame.buffer.reset(new uint8_t[videoFrame.size]);
                //memcpy(videoFrame.buffer.get(), video frame data, videoFrame.size);					

                rtspPusher->pushFrame(xop::channel_0, videoFrame); //推流到服务器, 接口线程安全
            */ 
        }
                
        {				           
            //获取一帧 AAC, 打包
            /*
                xop::AVFrame audioFrame = {0};
                //audioFrame.size = audio frame size;  // 音频帧大小 
                audioFrame.timestamp = xop::AACSource::getTimeStamp(44100); // 时间戳
                audioFrame.buffer.reset(new uint8_t[audioFrame.size]);
                //memcpy(audioFrame.buffer.get(), audio frame data, audioFrame.size);

                rtspPusher->pushFrame(xop::channel_1, audioFrame); //推流到服务器, 接口线程安全
            */            
        }		

        xop::Timer::sleep(1); /* 实际使用需要根据帧率计算延时! */
    }
}
