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

protected:
	virtual const AUDIOSAMPLE *getBuffer(unsigned *oSize);

private:
	WavFile _wav;
	bool    _loop;
	bool    _isPlaying;
};

#endif /* AUDIO_WAVSOURCE_H_ */
