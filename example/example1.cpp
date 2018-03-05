// PHZ
// 2018-2-10

#include "RtspServer.h"
#include "H264Source.h"
#include "AACSource.h"
#include "xop.h"
#include <thread>
#include <memory>
#include <iostream>
#include <string>

#if defined(WIN32) || defined(_WIN32) 
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib,"Iphlpapi.lib")
#endif 

using namespace std;
using namespace xop;

void snedFrame(RtspServer* rtspServer, MediaSessionId sessionId, int& clients)
{
	while(1)
	{
		if(clients > 0) // MediaSession有客户端在线, 发送音视频数据
		{
			{
				/* 
					//获取一帧 H264, 打包
					xop::AVFrame videoFrame = {0};
					videoFrame.size = 100000;  // 视频帧大小 
					videoFrame.timestamp = H264Source::getTimeStamp(); // 时间戳
					videoFrame.buffer.reset(new char[videoFrame.size+1000]);
					memcpy(videoFrame.buffer.get(), 视频帧数据, videoFrame.size);
					
					// 推送到服务器进行转发
					rtspServer->pushFrame(sessionId, channel_0, videoFrame); // 接口 线程安全
				*/
			}
			
			static uint32_t audio_pts = 1024;
			{
				
				/*
					//获取一帧 AAC, 打包
					xop::AVFrame audioFrame = {0};
					audioFrame.size = 500;  // 音频帧大小 
					audioFrame.timestamp = audio_pts; // 时间戳
					audio_pts += 1024;
					audioFrame.buffer.reset(new char[audioFrame.size+500]);
					memcpy(audioFrame.buffer.get(), 音频帧数据, audioFrame.size);
					
					// 推送到服务器进行转发
					rtspServer->pushFrame(sessionId, channel_1, audioFrame); // 接口 线程安全
				*/
			}		
		}

		Timer::sleep(1000);
	}
}

int main(int agrc, char **argv)
{	
	XOP_Init();
	
	// 记录当前客户端数量
	int clients = 0;
	std::string ip = NetInterface::getLocalIPAddress();
	
	std::shared_ptr<EventLoop> eventLoop(new EventLoop());
	RtspServer server(eventLoop.get(), ip, 554); 

	MediaSession *session = MediaSession::createNew("live"); // rtsp://ip/live
	// 同时传输音视频, track0:h264, track1:aac
	session->addMediaSource(xop::channel_0, H264Source::createNew()); 
	session->addMediaSource(xop::channel_1, AACSource::createNew(44100,2));

	//  开启组播(ip,端口随机生成), 默认使用 RTP_OVER_UDP, RTP_OVER_RTSP
	// session->startMulticast();
	
	// 设置通知回调函数。 在当前会话, 客户端连接或断开会发起通知
	session->setNotifyCallback([&clients](MediaSessionId sessionId, uint32_t numClients) 
	{
		clients = numClients; //获取当前MediaSession客户端数量
		cout << "MediaSession" << "(" << sessionId << ") "
			 << "clients: " << clients << endl;
	});
	
	cout << "rtsp://" << ip << "/" << session->getRtspUrlSuffix() << endl;
	
	// 添加session到RtspServer后, session会失效
	MediaSessionId sessionId = server.addMeidaSession(session); // 接口线程安全

	// 音视频数据转发线程
	thread t(snedFrame, &server, sessionId, ref(clients));
	t.detach();

	// 运行 RtspServer 
	eventLoop->loop();
	
	getchar();
	return 0;
}

