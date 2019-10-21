# RtspServer

项目介绍
-
* C++11实现的RTSP服务器和推流器。

项目Demo
-
* [DesktopSharing](https://github.com/PHZ76/DesktopSharing): 抓取屏幕和麦克风的音视频数据，编码后进行RTSP转发和推流。

目前情况
-
* 支持 Windows 和 Linux平台。
* 支持 H264, H265, G711A, AAC 四种音视频格式的转发。
* 支持同时传输音视频。
* 支持单播(RTP_OVER_UDP, RTP_OVER_RTSP), 组播。
* 支持心跳检测(单播)。
* 支持RTSP推流(TCP)。
* 支持摘要认证(Digest Authentication)。

编译环境
-
* Linux: gcc 4.8
* Windows: vs2015

整体框架
-
![image](https://github.com/PHZ76/RtspServer/blob/master/pic/1.pic.JPG) 


感谢
-
* [websocketpp-md5](https://github.com/zaphoyd/websocketpp)

其他问题
-
* 如何转发或推流媒体文件? 项目目前没有实现对媒体文件的解析, 可以通过ffmpeg对文件进行帧读取再进行转发。
* RTSP连接成功没有收到数据? (1)可以使用Wireshark分析RTP包。(2)确定系统的大小端模式。

联系方式
-
* penghaoze76@qq.com