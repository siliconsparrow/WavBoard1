/*
 * WavSource.h
 *
 *  Created on: 13 Feb 2021
 *      Author: adam
 */

#ifndef AUDIO_WAVSOURCE_H_
#define AUDIO_WAVSOURCE_H_

#include "AudioSource.h"
#include "WavFile.h"

class WavSource
    : public AudioSource
{
public:
	WavSource();
	virtual ~WavSource();

	bool open(const TCHAR *filename);
	void close();

	void play(bool loop = false);
	void stop();

	virtual void fillBuffer(AUDIOSAMPLE *buffer);

private:
	WavFile _wav;
	bool    _loop;
	bool    _isPlaying;

	void convert16(uint16_t *data, unsigned count);
};

#endif /* AUDIO_WAVSOURCE_H_ */
