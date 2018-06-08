# RtspServer

项目介绍<br>
-
* C++11实现的RTSP服务器。<br>
* 案例1-[DesktopSharing](https://github.com/PHZ76/DesktopSharing): 抓取屏幕和麦克风的音视频数据，编码后进行RTSP推送。<br>

目前情况<br>
-
* 支持 Windows 和 Linux平台。<br>
* 支持 H264, H265, G711A, AAC 四种音视频格式的转发。<br>
* 支持同时传输音视频。<br>
* 支持单播(RTP_OVER_UDP, RTP_OVER_RTSP), 组播。<br>
* 单播支持心跳检测。组播暂未支持。<br>
* 2018-6-8: 支持RTSP推流(测试中)。<br>

编译环境<br>
-
* Linux: gcc 4.7<br>
* Windows: vs2015<br>

整体框架<br>
-
![image](https://github.com/PHZ76/RtspServer/blob/master/pic/1.pic.JPG) <br><br>

后续计划<br>
-
* 增加RTSP推流模块。<br>
* 增加RTSP客户端模块。<br>
* 多线程支持。<br>

其他问题<br>
-
* 如何转发媒体文件? 目前没有实现对媒体文件的解析, 可以通过ffmpeg对文件进行帧读取再进行转发。<br>

联系方式<br>
-
* QQ: 2235710879

