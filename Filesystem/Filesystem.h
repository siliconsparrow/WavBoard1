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
	~File();

	enum FileMode {
		modeRead   = FA_READ,
		modeWrite  = FA_WRITE,
		modeCreate = FA_CREATE_NEW + FA_WRITE,
		modeAppend = FA_OPEN_APPEND + FA_WRITE,
	};

	enum SeekMode {
		SEEK_SET,
		SEEK_CUR,
		SEEK_END
	};


	bool open(const TCHAR *filename, FileMode mode = modeRead);
	void close();

	bool     isEOF() const;
	unsigned size()  const;
	int      read(uint8_t *buf, int nBytes);
	unsigned tell()  const;
	bool     seek(int offset, SeekMode mode = SEEK_SET);

#ifndef SDCARD_READONLY
	int  write(const uint8_t *data, int size);
#endif // SDCARD_READONLY

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
