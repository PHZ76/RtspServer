# RtspServer


项目介绍<br>
-
* C++11实现的RTSP服务器。<br>
* 案例1: 抓取屏幕和麦克风进行RTSP推送, [DesktopSharing](https://github.com/PHZ76/DesktopSharing)  <br>

目前情况<br>
-
* 支持 Windows 和 Linux平台。<br>
* 支持 H264, H265, G711A, AAC 四种音视频格式的转发。<br>
* 支持同时传输音视频。<br>
* 支持单播(RTP_OVER_UDP, RTP_OVER_RTSP), 组播。<br>
* 单播支持心跳检测。组播暂未支持。<br>

编译环境<br>
-
* Linux: ubuntu16.04 -- gcc4.7<br>
* Windows: win10 -- vs2015<br>

后续计划<br>
-
* 增加其他音视频格式的支持。<br>
* 增加RTCP。<br>
* 其他优化。<br>

其他问题<br>
-
* 如何转发媒体文件? 目前没有实现对媒体文件的解析, 可以通过ffmpeg对文件进行帧读取再进行转发。<br>

联系方式<br>
-
* QQ: 2235710879

