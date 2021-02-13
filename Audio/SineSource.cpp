/*
 * SineSource.cpp
 *
 *  Created on: 13 Feb 2021
 *      Author: adam
 */

#include "SineSource.h"

// Pre-calculated sine wave.
#define SYNTH_SINE_SAMPLES 32
static const uint16_t SYNTH_SINE[] =
{
	0x8000,
	0x98F8,
	0xB0FB,
	0xC71C,
	0xDA82,
	0xEA6D,
	0xF641,
	0xFD8A,
	0xFFFF,
	0xFD8A,
	0xF641,
	0xEA6D,
	0xDA82,
	0xC71C,
	0xB0FB,
	0x98F8,
	0x8000,
	0x6707,
	0x4F04,
	0x38E3,
	0x257D,
	0x1592,
	0x09BE,
	0x0275,
	0x0000,
	0x0275,
	0x09BE,
	0x1592,
	0x257D,
	0x38E2,
	0x4F04,
	0x6707
};

uint32_t g_sinedata[SYNTH_SINE_SAMPLES];

SineSource::SineSource()
{
	// Create stereo sine wave from mono data.
	for(int i = 0; i < SYNTH_SINE_SAMPLES; i++)
	{
		uint16_t val = SYNTH_SINE[i] / 8; // Lower the volume.
		val += 0x2000;
		g_sinedata[i] = ((uint32_t)val << 16) | (uint32_t)val;
	}
}

SineSource::~SineSource()
{
}

const AUDIOSAMPLE *SineSource::getBuffer(unsigned *oSize)
{
	*oSize = SYNTH_SINE_SAMPLES * sizeof(AUDIOSAMPLE);
	return g_sinedata;
}
