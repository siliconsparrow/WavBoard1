/*
 * Filesystem.cpp
 *
 *  Created on: 6 Feb 2021
 *      Author: adam
 */

#include "Filesystem.h"
#include "board.h"

Filesystem::Filesystem()
	: _csPort(SDCARD_CS_PORT)
{
	_csPort.setPin(SDCARD_CS_PIN, Gpio::OUTPUT);
	_csPort.set(1 << SDCARD_CS_PIN);

	const uint8_t *data = (const uint8_t *)"HELLO";

	_csPort.clrPin(SDCARD_CS_PIN);
	_spi.send(data, 5);
	_csPort.setPin(SDCARD_CS_PIN);

	uint8_t buf[5];
	_csPort.clrPin(SDCARD_CS_PIN);
	_spi.recv(buf, 5);
	_csPort.setPin(SDCARD_CS_PIN);
}
