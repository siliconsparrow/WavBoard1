// *************************************************************************
// **
// ** SD Card driver for the Kinetis KL17 using SPI
// **
// **   by Adam Pierce <adam@siliconsparrow.com>
// **   created 5-Feb-2021
// **
// *************************************************************************

#include "SDCard.h"
#include "SystemTick.h"
#include "board.h"

// Speed tokens for SPI.
enum {
	kSpiSpeedSlow =   400000, // 400kHz for slow cards.
	kSpiSpeedFast = 25000000, // 25MHz for fast cards.
};

enum {
	kBlockSize = 512, // SD Card standard block size.
};

// R1 response status flags.
#define R1_IDLE_STATE           0x01
#define R1_ERASE_RESET          0x02
#define R1_ILLEGAL_COMMAND      0x04
#define R1_COM_CRC_ERROR        0x08
#define R1_ERASE_SEQUENCE_ERROR 0x10
#define R1_ADDRESS_ERROR        0x20
#define R1_PARAMETER_ERROR      0x40

// Command IDs used by this module.
#define SDCARD_CMD_GO_IDLE_STATE     0
#define SDCARD_CMD_SEND_IF_COND      8
#define SDCARD_CMD_READ_BLOCK       17
#define SDCARD_CMD_WRITE_BLOCK      24
#define SDCARD_CMD_SET_BLOCKLEN     16
#define SDCARD_CMD_APP_CMD          55
#define SDCARD_CMD_READ_EXTR_MULTI  58
#define SDCARD_CMD_WRITE_EXTR_MULTI 59

// "Application" commands used by this module.
#define SDCARD_APPCMD_SD_SEND_OP_COND 41

// Single instance. We assume only the one card.
static SDCard *g_sdCard = 0;

SDCard *SDCard::instance()
{
	return g_sdCard;
}

SDCard::SDCard()
	: _csPort(SDCARD_CS_PORT)
	, _cardType(cardtypeNone)
{
	// Set up the chip select pin.
	_csPort.setPinMode(SDCARD_CS_PIN, Gpio::OUTPUT);
	deselect();

	// Register this as the one and only SD card object.
	g_sdCard = this;
}

// Return true if there is a valid card inserted.
bool SDCard::getStatus()
{
	if(_cardType == cardtypeNone)
		init();

	return _cardType != cardtypeNone;
}

// SD Card initialisation procedure.
bool SDCard::init()
{
	_cardType = cardtypeNone;

	// Set SPI clock to 100kHz for initialisation, and clock card with cs = 1
	deselect();
	SystemTick::delayTicks(MS_TO_TICKS(100));
	_spi.setFrequency(kSpiSpeedSlow);

	// Provide 10 bytes worth of clock.
	uint8_t buf[10];
	_spi.recv(buf, 10);

	// Start up the card.
	_cardType = startSequence();
	if(_cardType == cardtypeNone)
		return false;

	// Set SPI clock to 18MHz for data transfer.
	SystemTick::delayTicks(MS_TO_TICKS(100));
	_spi.setFrequency(kSpiSpeedFast);
	return true;
}

bool SDCard::readBlocks(uint8_t *buf, unsigned sector, unsigned nSectors)
{
	// TODO: Implement me.
	return false;
}

bool SDCard::writeBlocks(const uint8_t *buf, unsigned sector, unsigned nSectors)
{
	// TODO: Implement me.
	return false;
}
unsigned SDCard::getBlockCount() const
{
	// TODO: Implement me!
	return 0;
}

unsigned SDCard::getBlockSize() const
{
	// TODO: Implement me!
	return 0;
}

unsigned SDCard::getEraseSectorSize() const
{
	// TODO: Implement me!
	return 0;
}

// Enable the card's SPI interface.
void SDCard::select()
{
	_csPort.clrPin(SDCARD_CS_PIN);
}

// Disable the card's SPI interface.
void SDCard::deselect()
{
	_csPort.setPin(SDCARD_CS_PIN);
}

// SD Card first stage initialisation.
// Returns the card type detected.
unsigned SDCard::startSequence()
{
	// Send CMD0, should enter IDLE STATE.
	select();
	if(command(0, 0) != R1_IDLE_STATE) {
		deselect();
		return cardtypeNone; // No card detected.
	}

	// Now attempt to identify the card.

	// Send CMD8 to determine whether it is v1 or v2 interface.
	if(command(8, 0x1AA) == R1_IDLE_STATE) {
		// Card has v2 interface.
		uint8_t ocr[4];
		_spi.recv(ocr, 4);

		if(ocr[2] == 0x01 && ocr[3] == 0xAA)
		  	return initCardV2();
	} else {
		// Card has v1 interface.
		unsigned cardtype = initCardV1();
		if(cardtype != cardtypeNone)
		{
			// Set block length to 512 (CMD16)
			if(command(16, kBlockSize) != 0)
				return cardtypeNone; // Failed to set block length.
		}
		return cardtype;
	}

	deselect(); // Timeout or error.
	return cardtypeNone;
}

// Send a command to the card and receive an R1 response.
// The return value from this function may contain any of the
// R1_* flags:
uint8_t SDCard::command(uint8_t cmd, uint32_t arg)
{
	if(!waitReady())
		return 0xFF; // Card not responding.

	// Send a command.
	uint8_t req[8];
	unsigned p = 0;
	req[p++] = 0x40 | cmd;
	req[p++] = arg >> 24;
	req[p++] = (arg >> 16) & 0xFF;
	req[p++] = (arg >> 8) & 0xFF;
	req[p++] = arg & 0xFF;
	if(cmd == 0) {
		req[p++] = 0x95;
	} else if(cmd == 8) {
		req[p++] = 0x87;
	} else {
		req[p++] = 0x01;
	}

	_spi.send(req, p);

	if(cmd == 12) {
		_spi.recv();
	}

    // Wait for the repsonse (response[7] == 0).
	uint8_t response = 0;
    for(int i = 0; i < 10; i++)
	{
        response = _spi.recv();
        if(0 == (response & 0x80)) // Check validation bit.
            return response;
    }

    return response; // timeout
}

// Send an "Application" command.
uint8_t SDCard::appCommand(uint8_t cmd, uint32_t arg)
{
	uint8_t response = command(SDCARD_CMD_APP_CMD, 0);
	if(0 != (0xFE & response))
		return response;

	return command(cmd, arg);
}

// Block if the card is busy
bool SDCard::waitReady()
{
	Timeout t(MS_TO_TICKS(1000));

	_spi.recv();
	while(!t.isExpired())
	{
		if(_spi.recv() == 0xFF)
			return true;
	}
	return false;
}

// Card is an older v1.x SPI interface. Identify the card and return its type.
unsigned SDCard::initCardV1()
{
	for(int j = 0; j < 10; j++)
	{
		if(appCommand(41, 0) <= 1)
		{
			for(int i = 0; i < 100; i++)
			{
				if(appCommand(41, 0) == 0)
					return cardtypeSDv1; // SDSC card.
			}
		}
		else
		{
			for(int i = 0; i < 100; i++)
			{
				if(command(1, 0) == 0)
					return cardtypeMMC; // MMC card.
			}
		}
	}

	deselect();
	return cardtypeNone;
}

// Card has v2 interface. Identify the card and return the card type.
unsigned SDCard::initCardV2()
{
	Timeout t(1000);
	while(!t.isExpired()) // Wait until out of idle state.
	{
		if(appCommand(41, 0x40000000) == 0)
		{
			if(0 != command(58, 0))
			{
				deselect();
				return cardtypeNone;
			}

			uint8_t ocr[4];
			_spi.recv(ocr, 4);

			if(0 != (ocr[0] & 0x40))
				return cardtypeSDv2 | cardtypeBlock;

			return cardtypeSDv2;
		}
	}

	deselect();
	return cardtypeNone;
}

#ifdef OLD

#include "platform.h"
#include "Timeout.h"
#include "util.h"

// Define this symbol to use DMA for SPI data transfers.
#define SD_USE_DMA

// First byte of each packet has this pattern plus a command ID.
#define SDCARD_SYNC 0x40



bool SDCard::isCardOK()
{
	if(_cardType == SDCARD_NONE)
		initCard();

	return _cardType != SDCARD_NONE;
}

bool SDCard::isHighCapacity() const
{
	return 0 != (_cardType & SDCARD_BLOCK);
}

// *************************************************************************
// ** PUBLIC FUNCTIONS

// Read a 512-byte block of data from the card.
// Obviously make sure your buffer is at least 512 bytes long.
bool SDCard::readSector(unsigned sector, uint8_t *buffer)
{
	if(!isCardOK())
		return false;

	// set read address for single block (CMD17)
	if(!isHighCapacity())
		sector <<= 9;

	if(command(17, sector) != 0)
		return false;

	//cardSelect();
	Timeout t(1000);
	while(!t.isExpired())
	{
		// Wait for a start flag (0xFE) from the SD Card.
		if(_spi->read() == 0xFE)
		{
			// Read data.
			#ifdef SD_USE_DMA
				_spi->dmaXfer(buffer, 0, SD_BLOCK_SIZE);
			#else
				spiReadBlock(_spi, buffer, SD_BLOCK_SIZE);
			#endif
			_spi->read(); // Read (and ignore) checksum.
			_spi->read();

			//cardDeselect();
			return true;
		}
	}

	//cardDeselect();
	return false; // Timeout with no data received.
}


// Write a 512-byte block of data to the card.
bool SDCard::writeSector(unsigned sector, const uint8_t *buffer)
{
	if(_cardType == SDCARD_NONE)
		return false;

	// set write address for single block (CMD24)
	if(!isHighCapacity())
		sector <<= 9;

    if(command(24, sector) != 0)
        return false;

    if(!waitReady())
    	return false;

	// Indicate start of block.
	//cardSelect();
	_spi->write(0xFE);

	// Write the data.
	#ifdef SD_USE_DMA
		_spi->dmaXfer(0, buffer, SD_BLOCK_SIZE);
	#else
		_spi->write(buffer, SD_BLOCK_SIZE);
	#endif

	// Write the (dummy) checksum.
	_spi->write(0xFF);
	_spi->write(0xFF);

	// Check the response token.
	uint8_t r = _spi->read();
	if((r & 0x1F) != 0x05)
	{
		//cardDeselect();
		return false;
	}

	return true;

	// Wait for write to finish.
    //for(int i = 0; i < 1000; i++)
	//{
	//	if(_spi->read() != 0)
	//	{
	//	    //cardDeselect();
	//		return true;
	//	}
	//}

    //cardDeselect();
    //return false; // Flash write failed.
}
#endif // OLD
