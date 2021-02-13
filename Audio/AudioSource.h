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
		kFrameSize = 256, // Recommended frame size (number of samples).
	};

	AudioSource() { }
	virtual ~AudioSource() { }

	virtual const AUDIOSAMPLE *getBuffer(unsigned *oSize) = 0;
};

#endif // AUDIOSOURCE_H_
