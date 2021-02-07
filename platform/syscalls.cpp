// ***************************************************************************
// **
// ** Some required functions for C-language startup.
// **
// **   by Adam Pierce <adam@siliconsparrow.com>
// **   created 1-Jun-2016
// **
// ***************************************************************************

//#include "board.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <errno.h>

extern "C" {
#if 0
	#undef errno
	extern int errno;

	extern int __HeapStart;
	extern int __HeapLimit;

	extern int _write(int file, char *ptr, int len)
	{
		return -1;
	}
#endif
	extern caddr_t _sbrk(int incr)
	{/*
		static unsigned char *heap = NULL ;
		unsigned char *prev_heap ;

		if(heap == NULL)
			heap = (unsigned char *)&__HeapStart;

		prev_heap = heap;

		if((int)heap + incr > (int)&__HeapLimit)
		{
			_write(0, (char *)"OUT OF MEMORY!!\r\n", 17);
			errno = ENOMEM;
			return (caddr_t)(-1);
		}

		heap += incr;

		return (caddr_t) prev_heap;*/
		return 0;
	}
#if 0
	// Returns number of bytes free on the heap.
	unsigned getUnusedHeap()
	{
		return 0; // (uint8_t *)&__HeapLimit - (uint8_t *)_sbrk(0);
	}

	extern int link(char *oldlink, char *newlink)
	{
		return -1;
	}

	extern int _close(int file)
	{
		return -1;
	}

	extern int _fstat(int file, struct stat *st)
	{
		st->st_mode = S_IFCHR ;

		return 0;
	}

	// Open a file.
	extern int _open(const char *name, int mode)
	{
		return -1;
	}

	extern int _isatty(int file)
	{
		return 1;
	}

	extern int _lseek(int file, int ptr, int dir)
	{
		return 0;
	}

	extern int _read(int file, char *ptr, int len)
	{
		return 0;
	}
#endif
	extern void _exit(int status)
	{
		while(1)
			;
			//reboot();
	}

	extern void _kill(int pid, int sig)
	{
		return;
	}

	extern int _getpid( void)
	{
		return -1;
	}

} // Extern "C"
