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

class AudioKinetisI2S //: public Audio
{
public:
	static AudioKinetisI2S *instance();

	AudioKinetisI2S();

	void write(const uint32_t *data, unsigned dataSize);
	void setDataSource(AudioSource *src);

	void irq();

private:
	AudioSource *_dataSource;

	void dmaAbort();
	void dmaStart();
	void dmaStartTransfer(void *srcAddr, void *destAddr, unsigned transferBytes);
};

#endif /* AUDIOKINETISI2S_H_ */
