/*
 * SineSource.h
 *
 *  Created on: 13 Feb 2021
 *      Author: adam
 */

#ifndef AUDIO_SINESOURCE_H_
#define AUDIO_SINESOURCE_H_

#include "AudioSource.h"

class SineSource
	: public AudioSource
{
public:
	SineSource();
	virtual ~SineSource();

	virtual void fillBuffer(AUDIOSAMPLE *buffer);

private:
	enum {
		kSineSamples = 32
	};
	unsigned _pos;
	uint32_t _sinedata[kSineSamples];
};

#endif // AUDIO_SINESOURCE_H_
