// ***************************************************************************
// **
// ** I2S driver for FlexIO on the Kinetis MKL17Z
// **
// **   by Adam Pierce <adam@siliconsparrow.com>
// **   created 6-Jan-2018
// **
// ***************************************************************************

#ifndef AUDIOKINETISI2S_H_
#define AUDIOKINETISI2S_H_

#include <inttypes.h>
#include "AudioSource.h"
#include "Dma.h"

class AudioKinetisI2S
{
public:
	static AudioKinetisI2S *instance();

	AudioKinetisI2S();

	void setDataSource(AudioSource *src);

	void irq();

private:
	AudioSource *_dataSource;
	Dma          _dma;
	AUDIOSAMPLE *_currentBuf;
	AUDIOSAMPLE  _buf1[AudioSource::kFrameSize];
	AUDIOSAMPLE  _buf2[AudioSource::kFrameSize];

	void dmaStart();
};

#endif // AUDIOKINETISI2S_H_
