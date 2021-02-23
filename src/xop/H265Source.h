﻿// PHZ
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
	static std::unique_ptr<MediaSource> CreateNew(uint32_t framerate=25);
        
	void Setframerate(uint32_t framerate)
	{ framerate_ = framerate; }

	uint32_t GetFramerate() const 
	{ return framerate_; }

	virtual std::string GetMediaDescription(uint16_t port=0); 

	virtual std::string GetAttribute(); 

	bool HandleFrame(MediaChannelId channelId, AVFrame frame);

	static uint32_t GetTimestamp();
	 
private:
	H265Source(uint32_t framerate);

	uint32_t framerate_ = 25;
};
	
}

#endif


