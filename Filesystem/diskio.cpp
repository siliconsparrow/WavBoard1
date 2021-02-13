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
	SDCard *sd = SDCard::instance();

	if(sd->readBlocks(buff, sector, nSectors))
		return RES_OK;
	else
		return RES_PARERR;
	/*
	for(UINT i = 0; i < nSectors; i++) {
		if(!sd->readSector(sector, buff))
			return RES_PARERR;

		sector++;
		buff += sd->getBlockSize();
	}

	return RES_OK;*/
}

// Write Sector(s)
DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT nSectors)
{
#ifndef SDCARD_READONLY
	SDCard *sd = SDCard::instance();

	for(UINT i = 0; i < nSectors; i++) {
		if(!sd->writeSector(sector, buff))
			return RES_PARERR;

		sector++;
		buff += sd->getBlockSize();
	}
	return RES_OK;
#else
	return RES_PARERR;
#endif
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
