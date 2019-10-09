// PHZ
// 2018-5-16

#ifndef XOP_H265_SOURCE_H
#define XOP_H265_SOURCE_H

#include "MediaSource.h"
#include "rtp.h"

namespace xop
{

class H265Source : public MediaSource
{
public:
    static H265Source* createNew(uint32_t frameRate=25);
    virtual ~H265Source();

    void setFrameRate(uint32_t frameRate)
    { _frameRate = frameRate; }

    uint32_t getFrameRate() const 
    { return _frameRate; }

    /* SDP媒体描述 m= */
    virtual std::string getMediaDescription(uint16_t port=0); 

    /* SDP属性 a= */
    virtual std::string getAttribute(); 

    bool handleFrame(MediaChannelId channelId, AVFrame frame);

    static uint32_t getTimeStamp();
	
private:
    H265Source(uint32_t frameRate);

    uint32_t _frameRate = 25;
};
	
}

#endif


