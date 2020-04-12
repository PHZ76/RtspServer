// RTSP Pusher

#include "xop/RtspPusher.h"
#include "net/Timer.h"
#include <thread>
#include <memory>
#include <iostream>
#include <string>

#define PUSH_TEST "rtsp://10.11.165.203:554/test" 

void snedFrameThread(xop::RtspPusher* rtspPusher);

int main(int argc, char **argv)
{	
	std::shared_ptr<xop::EventLoop> event_loop(new xop::EventLoop());  
	std::shared_ptr<xop::RtspPusher> rtsp_pusher = xop::RtspPusher::Create(event_loop.get());

	xop::MediaSession *session = xop::MediaSession::CreateNew(); 
	session->AddSource(xop::channel_0, xop::H264Source::CreateNew()); 
	session->AddSource(xop::channel_1, xop::AACSource::CreateNew(44100, 2, false));
	rtsp_pusher->AddSession(session);

	if (rtsp_pusher->OpenUrl(PUSH_TEST, 3000) < 0) {
		std::cout << "Open " << PUSH_TEST << " failed." << std::endl; 
		getchar();
		return 0;
	}

	std::cout << "Push stream to " << PUSH_TEST << " ..." << std::endl; 
        
	std::thread thread(snedFrameThread, rtsp_pusher.get()); 
	thread.detach();

	while (1) {
		xop::Timer::Sleep(100);
	}

	getchar();
	return 0;
}

void snedFrameThread(xop::RtspPusher* rtsp_pusher)
{       
	while(rtsp_pusher->IsConnected())
	{      
		{                              
			/*
				//获取一帧 H264, 打包
				xop::AVFrame videoFrame = {0};
				//videoFrame.size = video frame size;  // 视频帧大小 
				videoFrame.timestamp = xop::H264Source::GetTimestamp(); // 时间戳, 建议使用编码器提供的时间戳
				videoFrame.buffer.reset(new uint8_t[videoFrame.size]);
				//memcpy(videoFrame.buffer.get(), video frame data, videoFrame.size);					

				rtsp_pusher->PushFrame(xop::channel_0, videoFrame); //推流到服务器, 接口线程安全
			*/
		}
                
		{				                       
			/*
				//获取一帧 AAC, 打包
				xop::AVFrame audioFrame = {0};
				//audioFrame.size = audio frame size;  // 音频帧大小 
				audioFrame.timestamp = xop::AACSource::GetTimestamp(44100); // 时间戳
				audioFrame.buffer.reset(new uint8_t[audioFrame.size]);
				//memcpy(audioFrame.buffer.get(), audio frame data, audioFrame.size);

				rtsp_pusher->PushFrame(xop::channel_1, audioFrame); //推流到服务器, 接口线程安全
			*/
		}		

		xop::Timer::Sleep(1); 
	}
}
