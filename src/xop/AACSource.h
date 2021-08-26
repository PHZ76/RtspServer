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
    static AACSource* CreateNew(uint32_t samplerate=44100, uint32_t channels=2, bool has_adts=true);
    virtual ~AACSource();

    uint32_t GetSamplerate() const
    { return samplerate_; }

    uint32_t GetChannels() const
    { return channels_; }

    virtual std::string GetMediaDescription(uint16_t port=0);

    virtual std::string GetAttribute();

    virtual bool HandleFrame(MediaChannelId channel_id, AVFrame frame);

    static uint32_t GetTimestamp(uint32_t samplerate =44100);

private:
    AACSource(uint32_t samplerate, uint32_t channels, bool has_adts);

    uint32_t samplerate_ = 44100;  
    uint32_t channels_ = 2;         
    bool has_adts_ = true;

    static const int ADTS_SIZE = 7;
    static const int AU_SIZE   = 4;
};

}

#endif
