/*
 * Filesystem.h
 *
 *  Created on: 6 Feb 2021
 *      Author: adam
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include "SDCard.h"

class Filesystem
{
public:
	Filesystem();

private:
	SDCard _sdcard;
};

#endif /* FILESYSTEM_H_ */
