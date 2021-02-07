/*
 * Filesystem.h
 *
 *  Created on: 6 Feb 2021
 *      Author: adam
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include "Spi.h"
#include "Gpio.h"

class Filesystem
{
public:
	Filesystem();

private:
	Spi  _spi;
	Gpio _csPort;
};

#endif /* FILESYSTEM_H_ */
