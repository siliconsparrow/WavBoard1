/*
 * AudioSource.h
 *
 *  Created on: 13Jan.,2018
 *      Author: adam
 */

#ifndef AUDIOSOURCE_H_
#define AUDIOSOURCE_H_

#include <stdint.h>

typedef uint32_t AUDIOSAMPLE;

class AudioSource
{
public:
	enum {
		kFrameSize = 256,                               // Frame size in number of samples.
		kFrameBytes = kFrameSize * sizeof(AUDIOSAMPLE), // Number of bytes in a frame.
	};

	AudioSource() { }
	virtual ~AudioSource() { }

	virtual void fillBuffer(AUDIOSAMPLE *buffer) = 0;
};

#endif // AUDIOSOURCE_H_
