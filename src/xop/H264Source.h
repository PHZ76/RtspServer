// PHZ
// 2018-5-16

#ifndef XOP_H264_SOURCE_H
#define XOP_H264_SOURCE_H

#include "MediaSource.h"
#include "rtp.h"

namespace xop
{

class H264Source : public MediaSource
{
public:
    static H264Source* createNew(uint32_t frameRate=25);
    ~H264Source();

    void setFrameRate(uint32_t frameRate)
    { _frameRate = frameRate; }

    uint32_t getFrameRate() const 
    { return _frameRate; }

    /* SDP媒体描述 m= */
    virtual std::string getMediaDescription(uint16_t port); 

    /* SDP媒体属性 a= */
    virtual std::string getAttribute(); 

    bool handleFrame(MediaChannelId channelId, AVFrame frame);

    static uint32_t getTimeStamp();
	
private:
    H264Source(uint32_t frameRate);

    uint32_t _frameRate = 25;
};
	
}

#endif



