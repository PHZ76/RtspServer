// RTSP Server

#include "xop/RtspServer.h"
#include "net/Timer.h"
#include <thread>
#include <memory>
#include <iostream>
#include <string>

void SendFrameThread(xop::RtspServer* rtsp_server, xop::MediaSessionId session_id, int& clients);

int main(int argc, char **argv)
{	
	int clients = 0;
	std::string ip = "0.0.0.0";
	std::string rtsp_url = "rtsp://127.0.0.1:554/live";

	std::shared_ptr<xop::EventLoop> event_loop(new xop::EventLoop());  
	std::shared_ptr<xop::RtspServer> server = xop::RtspServer::Create(event_loop.get());
	if (!server->Start(ip, 554)) {
		return -1;
	}
	
#ifdef AUTH_CONFIG
	server->SetAuthConfig("-_-", "admin", "12345");
#endif
	 
	xop::MediaSession *session = xop::MediaSession::CreateNew("live"); // url: rtsp://ip/live
	session->AddSource(xop::channel_0, xop::H264Source::CreateNew()); 
	session->AddSource(xop::channel_1, xop::AACSource::CreateNew(44100,2));
	// session->startMulticast(); /* 开启组播(ip,端口随机生成), 默认使用 RTP_OVER_UDP, RTP_OVER_RTSP */

	session->AddNotifyConnectedCallback([] (xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port){
		printf("RTSP client connect, ip=%s, port=%hu \n", peer_ip.c_str(), peer_port);
	});
   
	session->AddNotifyDisconnectedCallback([](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port) {
		printf("RTSP client disconnect, ip=%s, port=%hu \n", peer_ip.c_str(), peer_port);
	});

	std::cout << "URL: " << rtsp_url << std::endl;
        
	xop::MediaSessionId session_id = server->AddSession(session); 
	//server->removeMeidaSession(session_id); /* 取消会话, 接口线程安全 */
         
	std::thread thread(SendFrameThread, server.get(), session_id, std::ref(clients));
	thread.detach();

	while(1) {
		xop::Timer::Sleep(100);
	}

	getchar();
	return 0;
}

void SendFrameThread(xop::RtspServer* rtsp_server, xop::MediaSessionId session_id, int& clients)
{       
	while(1)
	{
		if(clients > 0) /* 会话有客户端在线, 发送音视频数据 */
		{
			{     
				/*
				//获取一帧 H264, 打包
				xop::AVFrame videoFrame = {0};
				videoFrame.type = 0; // 建议确定帧类型。I帧(xop::VIDEO_FRAME_I) P帧(xop::VIDEO_FRAME_P)
				videoFrame.size = video frame size;  // 视频帧大小 
				videoFrame.timestamp = xop::H264Source::GetTimestamp(); // 时间戳, 建议使用编码器提供的时间戳
				videoFrame.buffer.reset(new uint8_t[videoFrame.size]);                    
				memcpy(videoFrame.buffer.get(), video frame data, videoFrame.size);					
                   
				rtsp_server->PushFrame(session_id, xop::channel_0, videoFrame); //送到服务器进行转发, 接口线程安全
				*/
			}
                    
			{				
				/*
				//获取一帧 AAC, 打包
				xop::AVFrame audioFrame = {0};
				audioFrame.type = xop::AUDIO_FRAME;
				audioFrame.size = audio frame size;  /* 音频帧大小 
				audioFrame.timestamp = xop::AACSource::GetTimestamp(44100); // 时间戳
				audioFrame.buffer.reset(new uint8_t[audioFrame.size]);                    
				memcpy(audioFrame.buffer.get(), audio frame data, audioFrame.size);

				rtsp_server->PushFrame(session_id, xop::channel_1, audioFrame); // 送到服务器进行转发, 接口线程安全
				*/
			}		
		}

		xop::Timer::Sleep(1); /* 实际使用需要根据帧率计算延时! */
	}
}
