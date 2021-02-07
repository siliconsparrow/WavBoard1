/*
 * Filesystem.cpp
 *
 *  Created on: 6 Feb 2021
 *      Author: adam
 */

#include "Filesystem.h"
#include "board.h"

File::File()
{
}

bool File::open(const TCHAR *filename, FileMode mode)
{
	return FR_OK == f_open(&_f, filename, (BYTE)mode);
}

Filesystem::Filesystem()
{
	f_mount(&_fs, "0", 1);
}

#ifdef OLD
Filesystem::Filesystem()
	: _csPort(SDCARD_CS_PORT)
{

	// TEST OF THE DMA SPI.
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
#endif // OLD
