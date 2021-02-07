// Adam's cut-down version of this file. SD card is the only supported device.

#include "ffconf.h"     /* FatFs configuration options */
#include "diskio.h"     /* FatFs lower layer API */
#include "SDCard.h"

// Get Drive Status
extern "C" DSTATUS disk_status(BYTE pdrv)
{
	return SDCard::instance()->getStatus() ? RES_OK : STA_NOINIT;
}

// Initialize a Drive
DSTATUS disk_initialize(BYTE pdrv)
{
	return SDCard::instance()->init() ? RES_OK : STA_NOINIT;
}

// Read Sector(s)
DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT nSectors)
{
	return SDCard::instance()->readBlocks(buff, sector, nSectors) ? RES_OK : RES_PARERR;
}

// Write Sector(s)
DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT nSectors)
{
	return SDCard::instance()->writeBlocks(buff, sector, nSectors) ? RES_OK : RES_PARERR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buffer)
{
	DRESULT result = RES_OK;

	switch(cmd)
	{
	case GET_SECTOR_COUNT:
	    if(0 != buffer) {
	    	*(uint32_t *)buffer = SDCard::instance()->getBlockCount();
	    } else {
	    	result = RES_PARERR;
	    }
	    break;

	case GET_SECTOR_SIZE:
		if(0 != buffer) {
			*(uint32_t *)buffer = SDCard::instance()->getBlockSize();
		} else {
			result = RES_PARERR;
		}
		break;

	case GET_BLOCK_SIZE:
		if(0 != buffer) {
			*(uint32_t *)buffer = SDCard::instance()->getEraseSectorSize();
		} else {
			result = RES_PARERR;
		}
		break;

	case CTRL_SYNC:
		break;

	default:
		result = RES_PARERR;
		break;
	}

	return result;
}
