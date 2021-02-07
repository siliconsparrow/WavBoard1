/*
 * Filesystem.h
 *
 *  Created on: 6 Feb 2021
 *      Author: adam
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include "SDCard.h"
#include "ff.h"

class File
{
public:
	File();

	enum FileMode {
		modeRead   = FA_READ,
		modeWrite  = FA_WRITE,
		modeCreate = FA_CREATE_NEW + FA_WRITE,
		modeAppend = FA_OPEN_APPEND + FA_WRITE,
	};

	bool open(const TCHAR *filename, FileMode mode = modeRead);

private:
	FIL _f;
};

class Filesystem
{
public:
	Filesystem();

private:
	SDCard _card;
	FATFS  _fs;
};

#endif /* FILESYSTEM_H_ */
