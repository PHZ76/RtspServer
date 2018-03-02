# RtspServer


项目初衷<br>
-
一直使用live555做流媒体转发服务，但是live555在嵌入式平台表现不佳，3路1080P并发访问开始出现丢包, 而且音视频不同步。所以自己重新实现了一个RTSP服务器，效果不错，在项目中配合编解码器使用，画面延时在60~100ms，支持多路转发。<br>

目前情况<br>
-
* 支持 Windows 和 Linux。
* 支持 H264,H265,G711A,AAC 四种音视频格式的转发。<br>
* 支持同时传输音视频。<br>
* 支持单播(RTP_OVER_UDP, RTSP_OVER_RTP), 组播。<br>
 
编译环境<br>
-
* Linux: ubuntu16.04 -- gcc4.7<br>
* Windows: win10 -- vs2015<br>
 
后续计划<br>
-
* 增加其他音视频格式的支持
* 增加RTCP
* 其他优化
