/*
 * WavSource.cpp
 *
 *  Created on: 13 Feb 2021
 *      Author: adam
 */

#include "WavSource.h"
#include <string.h>

WavSource::WavSource()
	: _loop(false)
	, _isPlaying(false)
{
}

WavSource::~WavSource()
{
}

bool WavSource::open(const TCHAR *filename)
{
	return _wav.open(filename);
}

void WavSource::close()
{
	_wav.close();
}

void WavSource::play(bool loop)
{
	_loop = loop;
	_isPlaying = true;
}

void WavSource::stop()
{
	_isPlaying = false;
	_wav.rewind();
}

const AUDIOSAMPLE *WavSource::getBuffer(unsigned *oSize)
{
	static AUDIOSAMPLE buf[AudioSource::kFrameSize];

	if(_isPlaying) {
		*oSize = 0;
		int r = _wav.readBlock((uint8_t *)buf, sizeof(buf));
		if(r >= 0) {
			// Handle looping.
			*oSize = r;
			if(*oSize < sizeof(buf)) {
				if(_loop) {
					while(1) { // This is a while loop because the size of the file might be less than the frame size.
						_wav.rewind();
						r = _wav.readBlock((uint8_t *)&buf[*oSize], sizeof(buf) - *oSize);
						if(r <= 0)
							break;

						*oSize += r;
					}
				}
			}
		}
	}

	// Run out of data or not playing. Fill remains of buffer with silence.
	memset(&buf[*oSize], 0, AudioSource::kFrameSize * sizeof(AUDIOSAMPLE) - *oSize);
	*oSize = sizeof(buf);

	// Convert to unsigned data.
	//convert16((uint16_t *)buf, AudioSource::kFrameSize * 2);

	// TODO: Different conversions for 8-bit or mono or different sample rates.

	return buf;
}

// Convert 16-bit samples from signed (as used by WAV files) to unsigned (as used by I2S).
void WavSource::convert16(uint16_t *data, unsigned count)
{
	for(unsigned i = 0; i < count; i++)
		*data += 0x8000;
}
