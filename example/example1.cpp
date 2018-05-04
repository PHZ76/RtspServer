// PHZ
// 2018-2-10 创建demo
// 2018-5-1  补充注释

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

// 负责音视频数据转发的线程函数
void snedFrame(RtspServer* rtspServer, MediaSessionId sessionId, int& clients)
{
    static uint32_t audio_pts = 1024;
        
    while(1)
    {
        if(clients > 0) // 媒体会话有客户端在线, 发送音视频数据
        {
            {
                /* 
                    ... 
                    //获取一帧 H264, 打包
                    xop::AVFrame videoFrame = {0};
                    videoFrame.size = 100000;  // 视频帧大小 
                    videoFrame.timestamp = H264Source::getTimeStamp(); // 时间戳, 建议使用编码器提供的时间戳
                    videoFrame.buffer.reset(new char[videoFrame.size+1000]);
                    memcpy(videoFrame.buffer.get(), 视频帧数据, videoFrame.size);					
                    
                    rtspServer->pushFrame(sessionId, channel_0, videoFrame); // 推送到服务器进行转发
                */
            }
                    
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

        xop::Timer::sleep(1000); 
    }
}

int main(int agrc, char **argv)
{	
	XOP_Init();
	
	
	int clients = 0; // 记录当前客户端数量
	std::string ip = NetInterface::getLocalIPAddress(); //获取网卡ip地址
	
	std::shared_ptr<EventLoop> eventLoop(new EventLoop());  
	RtspServer server(eventLoop.get(), ip, 554);  //创建一个RTSP服务器

	MediaSession *session = MediaSession::createNew("live"); // 创建一个媒体会话, url: rtsp://ip/live
    
	// 会话同时传输音视频, track0:h264, track1:aac
	session->addMediaSource(xop::channel_0, H264Source::createNew()); 
	session->addMediaSource(xop::channel_1, AACSource::createNew(44100,2));
	
	// session->startMulticast(); // 开启组播(ip,端口随机生成), 默认使用 RTP_OVER_UDP, RTP_OVER_RTSP
	
	// 设置通知回调函数。 在当前会话, 客户端连接或断开会发起通知
	session->setNotifyCallback([&clients](MediaSessionId sessionId, uint32_t numClients) 
	{
		clients = numClients; //获取当前媒体会话客户端数量
		cout << "MediaSession" << "(" << sessionId << ") "
			 << "clients: " << clients << endl;
	});
	
	cout << "rtsp://" << ip << "/" << session->getRtspUrlSuffix() << endl;
		
	MediaSessionId sessionId = server.addMeidaSession(session); // 添加session到RtspServer后, session会失效
    //server.removeMeidaSession(sessionId); //取消会话, 接口线程安全
       	 
	thread t(snedFrame, &server, sessionId, ref(clients)); // 负责音视频数据转发的线程
	t.detach(); // 后台运行

	// 主线程运行 RtspServer 
	eventLoop->loop();
	
	getchar();
	return 0;
}

