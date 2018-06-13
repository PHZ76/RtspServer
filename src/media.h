// PHZ
// 2018-5-16

#ifndef XOP_MEDIA_H
#define XOP_MEDIA_H

#include <memory>
//#include "xop/MemoryManager.h" // 内存池

namespace xop
{

// RTSP服务支持的媒体类型
typedef enum __MediaType
{
    //PCMU = 0,	 
    PCMA = 8,	  
    H264 = 96,
    AAC  = 37,
    H265 = 265,   
    NONE
} MediaType;	

#define VIDEO_FRAME_I 0x01	/* I帧 */
#define VIDEO_FRAME_P 0x02	/* P帧 */
#define VIDEO_FRAME_B 0x03	/* B帧 */

#define AUDIO_FRAME   0x11

typedef struct __AVFrame
{	
    __AVFrame() 
    {
        size = 0;
        type = 0;
        timestamp = 0;
    }

    __AVFrame(uint32_t size)
        :buffer(new char[size])//: buffer((char*)xop::Alloc(size), xop::Free)
    {
        this->size = size;
        type = 0;
        timestamp = 0;
    }

    std::shared_ptr<char> buffer; /* 帧数据 */
    uint32_t size;				  /* 帧大小 */
    uint8_t  type;				  /* 帧类型 */	
    uint32_t timestamp;		  	  /* 时间戳 */
} AVFrame;

#define MAX_MEDIA_CHANNEL 2

typedef  enum __MediaChannel_Id
{
    channel_0,
    channel_1
} MediaChannelId;

typedef uint32_t MediaSessionId;

}

// xop
typedef enum _StreamId
{
    stream_0,
    stream_1
} StreamId;

#endif

