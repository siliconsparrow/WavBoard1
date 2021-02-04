/*
 * AudioSource.h
 *
 *  Created on: 13Jan.,2018
 *      Author: adam
 */

#ifndef AUDIOSOURCE_H_
#define AUDIOSOURCE_H_

#include <inttypes.h>

typedef uint32_t AUDIOSAMPLE;

class AudioSource
{
public:
	AudioSource();

	const AUDIOSAMPLE *getBuffer(unsigned *oSize);
};

#endif /* AUDIOSOURCE_H_ */
