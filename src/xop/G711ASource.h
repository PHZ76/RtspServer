// PHZ
// 2018-5-16

#ifndef XOP_G711A_SOURCE_H
#define XOP_G711A_SOURCE_H

#include "MediaSource.h"
#include "rtp.h"

namespace xop
{

class G711ASource : public MediaSource
{
public:
    static G711ASource* createNew();
    virtual ~G711ASource();

    uint32_t getSampleRate() const
    { return _sampleRate; }

    uint32_t getChannels() const
    { return _channels; }

    /* SDP媒体描述 m= */
    virtual std::string getMediaDescription(uint16_t port=0);

    /* SDP属性 a= */
    virtual std::string getAttribute();

    bool handleFrame(MediaChannelId channelId, AVFrame frame);

    static uint32_t getTimeStamp();

private:
    G711ASource();

    uint32_t _sampleRate = 8000;   
    uint32_t _channels = 1;       
};

}

#endif
