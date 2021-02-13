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

	virtual const AUDIOSAMPLE *getBuffer(unsigned *oSize);
};

#endif // AUDIO_SINESOURCE_H_
