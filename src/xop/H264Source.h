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
	static std::unique_ptr<MediaSource> CreateNew(uint32_t framerate=25);

	void SetFramerate(uint32_t framerate)
	{ framerate_ = framerate; }

	uint32_t GetFramerate() const 
	{ return framerate_; }

	virtual std::string GetMediaDescription(uint16_t port); 

	virtual std::string GetAttribute(); 

	bool HandleFrame(MediaChannelId channel_id, AVFrame frame);

	static uint32_t GetTimestamp();
	
private:
	H264Source(uint32_t framerate);

	uint32_t framerate_ = 25;
};
	
}

#endif



