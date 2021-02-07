// WAVBoard - A simple cheap device to replay WAV files and maybe do a
//            few extra audio functions.

// Running on a Kinetis MKL17Z32VLH4 at 48MHz with 32k Flash and 8k SRAM

// I need to get rid of all the crap in driver/ and write it from scratch so
// as to not have a huge image.

// TODO:
// * SD card driver
// * Fat32 filesystem (only needs to be read-only)
// * WAV file decoder
// * Granulation or some kind of filter thing
// * Inputs - GPIO and ADC
// * Use inputs to modulate playback and filters.
// * Looper

#include "board.h"
#include "SystemIntegration.h"
#include "Gpio.h"
#include "AudioKinetisI2S.h"
#include "AudioSource.h"
#include "Filesystem.h"
#include "SystemTick.h"
#include <stdint.h>

// Blinkenlight.
#define TEST_LED_PORT Gpio::portE
#define TEST_LED_PIN  0

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

//// TEST OF THE DMA SPI.
//void SpiTest()
//{
//	Spi spi;
//
////	uint8_t buf[10];
////	spi.recv(buf, 10);
//
//	Gpio csPort(SDCARD_CS_PORT);
//	csPort.setPinMode(SDCARD_CS_PIN, Gpio::OUTPUT);
//	csPort.set(1 << SDCARD_CS_PIN);
//
//
//	const uint8_t *data = (const uint8_t *)"HELLO";
//
//	csPort.clrPin(SDCARD_CS_PIN);
//	spi.send(data, 5);
//	csPort.setPin(SDCARD_CS_PIN);
//
//	uint8_t buf[8];
//	csPort.clrPin(SDCARD_CS_PIN);
//	spi.recv(buf, 8);
//	csPort.setPin(SDCARD_CS_PIN);
//}

int main(void)
{
	// Core init - make sure we are running at max clock speed.
	clockSetup();

	// System tick is a useful timer.
	SystemTick::init();

	// Set up the LED for testing.
	Gpio ledPort(TEST_LED_PORT);
	ledPort.setPinMode(TEST_LED_PIN, Gpio::OUTPUT);

	//SpiTest();

	// Init the SD card.
	Filesystem fs;

	// Set up I2S DAC to produce audio.
	AudioKinetisI2S audio;

	// Create audio source object and link it to the audio output.
	AudioSource src;
	audio.setDataSource(&src);

	// TEST open file.
	File fTest;
	fTest.open("LOOP001.WAV");

	while(1)
    {
    	delay();

    	//audio.write(sinedata, SYNTH_SINE_SAMPLES);

    	ledPort.toggle(1 << TEST_LED_PIN);
    }
}
