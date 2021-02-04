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

#include "board.h"
#include "SystemIntegration.h"
#include "AudioKinetisI2S.h"
#include "AudioSource.h"
#include <stdint.h>

void delay(void)
{
    volatile uint32_t i = 0;
    for (i = 0; i < 800000; ++i)
    {
        __asm("NOP"); /* delay */
    }
}

class GPIO
{
public:
	enum GPIODIR { INPUT, OUTPUT };

	GPIO(unsigned portnum);

	void setPin(unsigned pin, GPIODIR direction);

	inline void set(unsigned mask)    { _gpio->PSOR = mask; }
	inline void clr(unsigned mask)    { _gpio->PCOR = mask; }
	inline void toggle(unsigned mask) { _gpio->PTOR = mask; }

private:
	GPIO_Type *_gpio;
	PORT_Type *_port;
};

GPIO::GPIO(unsigned portnum)
{
	_gpio = (GPIO_Type *)(((uint8_t *)GPIOA) + (portnum * 0x40));
	_port = (PORT_Type *)(((uint8_t *)PORTA) + (portnum * 0x1000));
}

// Set pin data direction.
void GPIO::setPin(unsigned pinNum, GPIO::GPIODIR direction)
{
	// Set the pin mode to ALT1.
	SystemIntegration::setPinAlt(_port, pinNum, SystemIntegration::ALT1);

	// Set the direction.
	unsigned mask = (1 << pinNum);
	switch(direction)
	{
	case INPUT:
		_gpio->PDDR &= ~mask;
		break;

	case OUTPUT:
		_gpio->PDDR |= mask;
		break;
	}
}

// Engage HIRC mode for the main clock. This is the fastest
// clock speed on the KL17 (48MHz).
void clockSetup()
{
	MCG->C1 = 0x00; // Switch to high-frequency (48MHz) clock.
}

#define TEST_LED_PORT gpioe
#define TEST_LED_PIN  0


int main(void)
{
	// Core init - make sure we are running at max clock speed.
	clockSetup();

	// GPIO enable port E.
	SystemIntegration::enableClock(SystemIntegration::kCLOCK_PortE);
	GPIO gpioe(4);

	// Set pin direction for LED.
	TEST_LED_PORT.setPin(TEST_LED_PIN, GPIO::OUTPUT);

	// Set up I2S DAC to produce audio.
	AudioKinetisI2S audio;

	// Create audio source object and link it to the audio output.
	AudioSource src;
	audio.setDataSource(&src);

    while(1)
    {
    	delay();

    	//audio.write(sinedata, SYNTH_SINE_SAMPLES);

        TEST_LED_PORT.toggle((1<<TEST_LED_PIN));
    }
}
