// PHZ
// 2018-5-16

#ifndef XOP_AAC_SOURCE_H
#define XOP_AAC_SOURCE_H

#include "MediaSource.h"
#include "rtp.h"

namespace xop
{

class AACSource : public MediaSource
{
public:
    static AACSource* createNew(uint32_t sampleRate=44100, uint32_t channels=2, bool hasADTS=true);
    virtual ~AACSource();

    uint32_t getSampleRate() const
    { return _sampleRate; }

    uint32_t getChannels() const
    { return _channels; }

    /* SDP媒体描述 m= */
    virtual std::string getMediaDescription(uint16_t port=0);

    /* SDP属性 a= */
    virtual std::string getAttribute();

    bool handleFrame(MediaChannelId channelId, AVFrame frame);

    static uint32_t getTimeStamp(uint32_t sampleRate=44100);

private:
    AACSource(uint32_t sampleRate, uint32_t channels, bool hasADTS);

    uint32_t _sampleRate = 44100;  
    uint32_t _channels = 2;         
    bool _hasADTS = true;
};

}

#endif
