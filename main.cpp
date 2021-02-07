/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Actually I need to get rid of all the crap in driver/ and write it from scratch so
// as to not have a huge image.

// Running on a Kinetis MKL17Z32VLH4 at 48MHz with 32k Flash and 8k SRAM

// TODO:
// * Create a Makefile
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

#define TEST_LED_PORT Gpio::portE
#define TEST_LED_PIN  0


int main(void)
{
	// Core init - make sure we are running at max clock speed.
	clockSetup();

	// Set up the LED for testing.
	Gpio ledPort(TEST_LED_PORT);
	ledPort.setPin(TEST_LED_PIN, Gpio::OUTPUT);

	// Init the SD card.
	Filesystem fs;

	// Set up I2S DAC to produce audio.
	AudioKinetisI2S audio;

	// Create audio source object and link it to the audio output.
	AudioSource src;
	audio.setDataSource(&src);

	while(1)
    {
    	delay();

    	//audio.write(sinedata, SYNTH_SINE_SAMPLES);

    	ledPort.toggle(1 << TEST_LED_PIN);
    }
}
