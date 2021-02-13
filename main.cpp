// WAVBoard - A simple cheap device to replay WAV files and maybe do a
//            few extra audio functions.

// Running on a Kinetis MKL17Z32VLH4 at 48MHz with 32k Flash and 8k SRAM

// TODO:
// * DMA SPI not working
// * Granulation or some kind of filter thing
// * Inputs - GPIO and ADC
// * Use inputs to modulate playback and filters.
// * Looper

#include "board.h"
#include "SystemIntegration.h"
#include "AudioKinetisI2S.h"
#include "AudioSource.h"
#include "Filesystem.h"
#include "SystemTick.h"
#include "SineSource.h"
#include "WavSource.h"
#include <stdint.h>

void delay(void)
{
    volatile uint32_t i = 0;
    for (i = 0; i < 800000; ++i)
    {
        __asm("NOP"); /* delay */
    }
}

// Engage HIRC mode for the main clock. This is the fastest
// clock speed on the KL17 (48MHz).
void clockSetup()
{
	MCG->C1 = 0x00; // Switch to high-frequency (48MHz) clock.
}

int main(void)
{
	// Core init - make sure we are running at max clock speed.
	clockSetup();

	// System tick is a useful timer.
	SystemTick::init();

	// Init the SD card and mount the filesystem.
	Filesystem fs;

	// Set up I2S DAC to produce audio.
	AudioKinetisI2S audio;

	// Create audio source object and link it to the audio output.
	SineSource sine;
	WavSource wav;
	if(wav.open("/LOOP001.WAV"))
		audio.setDataSource(&wav);
	else
		audio.setDataSource(&sine);

	wav.play(true);

	while(1)
    {
    }
}
