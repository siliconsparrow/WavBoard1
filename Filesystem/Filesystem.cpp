/*
 * Filesystem.cpp
 *
 *  Created on: 6 Feb 2021
 *      Author: adam
 */

#include "Filesystem.h"
#include "board.h"
#include <string.h>

File::File()
{
	memset(&_f, 0, sizeof(_f));
}

File::~File()
{
	close();
}

bool File::open(const TCHAR *filename, FileMode mode)
{
	return FR_OK == f_open(&_f, filename, (BYTE)mode);
}

void File::close()
{
	if(0 != _f.obj.fs)
		f_close(&_f);
}

bool File::isEOF() const
{
	return f_eof(&_f); // _f.fptr == _f.obj.objsize;
}

unsigned File::size() const
{
	return f_size(&_f);
}

int File::read(uint8_t *buf, int nBytes)
{
	UINT bytesRead;

	if(FR_OK != f_read(&_f, buf, nBytes, &bytesRead))
		return -1;

	return bytesRead;
}

unsigned File::tell() const
{
	return f_tell(&_f);
}

bool File::seek(int offset, SeekMode mode)
{
	FRESULT rslt;

	switch(mode)
	{
	case SEEK_SET:
		rslt = f_lseek(&_f, offset);
		break;

	case SEEK_CUR:
		rslt = f_lseek(&_f, tell() + offset);
		break;

	case SEEK_END:
		rslt = f_lseek(&_f, size() + offset);
		break;
	}

	return FR_OK == rslt;
}


#if 0
// Other functions I might like to add.
	FRESULT f_open (FIL* fp, const TCHAR* path, BYTE mode);				/* Open or create a file */
	FRESULT f_close (FIL* fp);											/* Close an open file object */
	FRESULT f_read (FIL* fp, void* buff, UINT btr, UINT* br);			/* Read data from the file */
	FRESULT f_write (FIL* fp, const void* buff, UINT btw, UINT* bw);	/* Write data to the file */
	FRESULT f_lseek (FIL* fp, FSIZE_t ofs);								/* Move file pointer of the file object */
	FRESULT f_truncate (FIL* fp);										/* Truncate the file */
	FRESULT f_sync (FIL* fp);											/* Flush cached data of the writing file */
	FRESULT f_opendir (DIR* dp, const TCHAR* path);						/* Open a directory */
	FRESULT f_closedir (DIR* dp);										/* Close an open directory */
	FRESULT f_readdir (DIR* dp, FILINFO* fno);							/* Read a directory item */
	FRESULT f_findfirst (DIR* dp, FILINFO* fno, const TCHAR* path, const TCHAR* pattern);	/* Find first file */
	FRESULT f_findnext (DIR* dp, FILINFO* fno);							/* Find next file */
	FRESULT f_mkdir (const TCHAR* path);								/* Create a sub directory */
	FRESULT f_unlink (const TCHAR* path);								/* Delete an existing file or directory */
	FRESULT f_rename (const TCHAR* path_old, const TCHAR* path_new);	/* Rename/Move a file or directory */
	FRESULT f_stat (const TCHAR* path, FILINFO* fno);					/* Get file status */
	FRESULT f_chmod (const TCHAR* path, BYTE attr, BYTE mask);			/* Change attribute of a file/dir */
	FRESULT f_utime (const TCHAR* path, const FILINFO* fno);			/* Change timestamp of a file/dir */
	FRESULT f_chdir (const TCHAR* path);								/* Change current directory */
	FRESULT f_chdrive (const TCHAR* path);								/* Change current drive */
	FRESULT f_getcwd (TCHAR* buff, UINT len);							/* Get current directory */
	FRESULT f_getfree (const TCHAR* path, DWORD* nclst, FATFS** fatfs);	/* Get number of free clusters on the drive */
	FRESULT f_getlabel (const TCHAR* path, TCHAR* label, DWORD* vsn);	/* Get volume label */
	FRESULT f_setlabel (const TCHAR* label);							/* Set volume label */
	FRESULT f_forward (FIL* fp, UINT(*func)(const BYTE*,UINT), UINT btf, UINT* bf);	/* Forward data to the stream */
	FRESULT f_expand (FIL* fp, FSIZE_t szf, BYTE opt);					/* Allocate a contiguous block to the file */
	FRESULT f_mount (FATFS* fs, const TCHAR* path, BYTE opt);			/* Mount/Unmount a logical drive */
	FRESULT f_mkfs (const TCHAR* path, BYTE opt, DWORD au, void* work, UINT len);	/* Create a FAT volume */
	FRESULT f_fdisk (BYTE pdrv, const DWORD* szt, void* work);			/* Divide a physical drive into some partitions */
	int f_putc (TCHAR c, FIL* fp);										/* Put a character to the file */
	int f_puts (const TCHAR* str, FIL* cp);								/* Put a string to the file */
	int f_printf (FIL* fp, const TCHAR* str, ...);						/* Put a formatted string to the file */
	TCHAR* f_gets (TCHAR* buff, int len, FIL* fp);						/* Get a string from the file */

	#define f_eof(fp) ((int)((fp)->fptr == (fp)->obj.objsize))
	#define f_error(fp) ((fp)->err)
	#define f_tell(fp) ((fp)->fptr)
	#define f_size(fp) ((fp)->obj.objsize)
	#define f_rewind(fp) f_lseek((fp), 0)
	#define f_rewinddir(dp) f_readdir((dp), 0)

	#ifndef EOF
	#define EOF (-1)
	#endif

#endif

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
