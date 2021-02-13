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

void WavSource::fillBuffer(AUDIOSAMPLE *buffer)
{
	unsigned  size = 0;
	uint8_t  *dest = (uint8_t *)buffer;

	if(_isPlaying) {
		int r = _wav.readBlock(dest, kFrameBytes);
		if(r >= 0) {
			size = r;

			// Handle looping.
			while(size < kFrameBytes && _loop) { // This is a while loop because the size of the file might be less than the frame size.
				_wav.rewind();
				r = _wav.readBlock(&dest[size], kFrameBytes - size);
				if(r <= 0)
					break;

				size += r;
			}
		}
	}

	// Run out of data or not playing. Fill remains of buffer with silence.
	unsigned remainingSpace = kFrameBytes - size;
	if(remainingSpace > 0)
		memset(&dest[size], 0, remainingSpace);

	// Convert to unsigned data.
	//convert16((uint16_t *)buffer, kFrameSize * 2);

	// Quieten it down a bit (temporary).
	int16_t *samp = (int16_t *)buffer;
	for(unsigned i = 0; i < kFrameSize * 2; i++)
		samp[i] = samp[i] / 4;

	// TODO: Different conversions for 8-bit or mono or different sample rates.
}

// Convert 16-bit samples from signed (as used by WAV files) to unsigned (as used by I2S).
void WavSource::convert16(uint16_t *data, unsigned count)
{
	for(unsigned i = 0; i < count; i++)
		data[i] = data[i] + 0x8000;
}
