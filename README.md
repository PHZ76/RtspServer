# RtspServer

项目介绍
-
* C++11实现的RTSP服务器和推流器。
* 设计目的是为了替代项目中使用的live555模块, live555在嵌入式平台上性能一般。
* 不考虑多线程的实现, 项目模块的定位是轻量级的RTSP服务器和推流器。

项目Demo
-
* 为了方便调试做的一个Demo: [DesktopSharing](https://github.com/PHZ76/DesktopSharing): 抓取屏幕和麦克风的音视频数据，编码后进行RTSP转发和推流。

目前情况
-
* 支持 Windows 和 Linux平台。
* 支持 H264, H265, G711A, AAC 四种音视频格式的转发。
* 支持同时传输音视频。
* 支持单播(RTP_OVER_UDP, RTP_OVER_RTSP), 组播。
* 单播支持心跳检测。组播暂未支持。
* 支持RTSP推流(TCP)。

编译环境
-
* Linux: gcc 4.7
* Windows: vs2015

整体框架
-
![image](https://github.com/PHZ76/RtspServer/blob/master/pic/1.pic.JPG) 

后续计划
-
* 增加RTMP推流模块。
* 增加RTSP客户端模块。

其他问题
-
* 如何转发或推流媒体文件? 项目目前没有实现对媒体文件的解析, 可以通过ffmpeg对文件进行帧读取再进行转发。

联系方式
-
* QQ: 2235710879

