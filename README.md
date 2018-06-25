# RtspServer

项目介绍
-
* C++11实现的RTSP服务器。
* 案例1-[DesktopSharing](https://github.com/PHZ76/DesktopSharing): 抓取屏幕和麦克风的音视频数据，编码后进行RTSP推送。

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

整体框架<br>
-
![image](https://github.com/PHZ76/RtspServer/blob/master/pic/1.pic.JPG) 

后续计划
-
* 增加RTSP推流模块。
* 增加RTSP客户端模块。
* 多线程支持。

其他问题
-
* 如何转发媒体文件? 目前没有实现对媒体文件的解析, 可以通过ffmpeg对文件进行帧读取再进行转发。

联系方式
-
* QQ: 2235710879

