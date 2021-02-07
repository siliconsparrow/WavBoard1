// *************************************************************************
// **
// ** SD Card driver for the Kinetis KL17 using SPI
// **
// **   by Adam Pierce <adam@siliconsparrow.com>
// **   created 5-Feb-2021
// **
// *************************************************************************

#include "SDCard.h"
#include "board.h"

SDCard::SDCard()
	: _csPort(SDCARD_CS_PORT)
{
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



#ifdef OLD

#include "GPIO.h"
#include "SDCard.h"
#include "platform.h"
#include "Timeout.h"
#include "util.h"

// Define this symbol to use DMA for SPI data transfers.
#define SD_USE_DMA

// R1 response status flags.
#define R1_IDLE_STATE           0x01
#define R1_ERASE_RESET          0x02
#define R1_ILLEGAL_COMMAND      0x04
#define R1_COM_CRC_ERROR        0x08
#define R1_ERASE_SEQUENCE_ERROR 0x10
#define R1_ADDRESS_ERROR        0x20
#define R1_PARAMETER_ERROR      0x40

// First byte of each packet has this pattern plus a command ID.
#define SDCARD_SYNC 0x40

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

// Maintain a single instance of this class.
SDCard *SDCard::getInstance()
{
	static SDCard sdcardInstance;

	if(sdcardInstance._spi == 0)
		sdcardInstance.initHW();

	return &sdcardInstance;
}

// Enable the chip select pin for the SD Card.
void SDCard::cardSelect()
{
	GPIO::CLR(SDCARD_CS_PORT, SDCARD_CS_PINMASK);
}

// Disable the chip select pin for the SD Card.
void SDCard::cardDeselect()
{
	GPIO::SET(SDCARD_CS_PORT, SDCARD_CS_PINMASK);
}

// Block if the card is busy
bool SDCard::waitReady()
{
	Timeout t(1000);

	_spi->read();
	while(!t.isExpired())
	{
		if(_spi->read() == 0xFF)
			return true;
	}
	return false;
}

// Set up the ports required to talk to the SD Card.
void SDCard::initHW()
{
	// Set up chip select pin.
#if defined(STM32F10X)
	GPIO::pinInit(SDCARD_CS, GPIO_Mode_Out_PP, GPIO_Speed_2MHz);
#elif defined(STM32L1XX)
	GPIO::pinInit(SDCARD_CS, GPIO_Mode_OUT, GPIO_PuPd_NOPULL);
#endif

	// Make sure card is deactivated.
	cardDeselect();

	// Set up SPI.
	_spi = SPI::getPort(JOEY_SPI_PORT_SDCARD);
}

// Send a command to the card and receive an R1 response.
// The return value from this function may contain any of the
// R1_* flags:
uint8_t SDCard::command(uint8_t cmd, uint32_t arg)
{
	if(!waitReady())
		return 0xFF; // Card not responding.

	// Send a command.
	_spi->write(0x40 | cmd);
	_spi->write(arg >> 24);
	_spi->write((arg >> 16) & 0xFF);
	_spi->write((arg >> 8) & 0xFF);
	_spi->write(arg & 0xFF);
	if(cmd == 0)
		_spi->write(0x95);
	else if(cmd == 8)
		_spi->write(0x87);
	else
		_spi->write(0x01);

	if(cmd == 12)
		_spi->read();

    // Wait for the repsonse (response[7] == 0).
	uint8_t response;
    for(int i = 0; i < 10; i++)
	{
        response = _spi->read();
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
					return SDCARD_SD1; // SDSC card.
			}
		}
		else
		{
			for(int i = 0; i < 100; i++)
			{
				if(command(1, 0) == 0)
					return SDCARD_MMC; // MMC card.
			}
		}
	}

	cardDeselect();
	return SDCARD_NONE;
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
				cardDeselect();
				//cout << "CMD58 FAIL\r\n";
				return SDCARD_NONE;
			}

			uint8_t ocr[4];
			for(int j = 0; j < 4; j++)
				ocr[j] = _spi->read();

			if(0 != (ocr[0] & 0x40))
				return SDCARD_SD2 | SDCARD_BLOCK;

			return SDCARD_SD2;
		}
	}

	cardDeselect();
	return SDCARD_NONE;
}

// SD Card first stage initialisation.
// Returns the card type detected.
unsigned SDCard::startSequence()
{
	// Send CMD0, should enter IDLE STATE.
	cardSelect();
	if(command(0, 0) != R1_IDLE_STATE)
	{
		cardDeselect();
		return SDCARD_NONE; // No card detected.
	}

	// Now attempt to identify the card.

	// Send CMD8 to determine whether it is v1 or v2 interface.
	if(command(8, 0x1AA) == R1_IDLE_STATE)
	{
		// Card has v2 interface.
		uint8_t ocr[4];
		for(int j = 0; j < 4; j++)
		   	ocr[j] = _spi->read();

		if(ocr[2] == 0x01 && ocr[3] == 0xAA)
		  	return initCardV2();
	}
	else
	{
		// Card has v1 interface.
		unsigned cardtype = initCardV1();
		if(cardtype != SDCARD_NONE)
		{
			// Set block length to 512 (CMD16)
			if(command(16, SD_BLOCK_SIZE) != 0)
				return SDCARD_NONE; // Failed to set block length.
		}
		return cardtype;
	}

	cardDeselect(); // Timeout or error.
	return SDCARD_NONE;
}

// SD Card initialisation procedure.
bool SDCard::initCard()
{
	_cardType = SDCARD_NONE;

    // Set SPI clock to 100kHz for initialisation, and clock card with cs = 1
	cardDeselect();
	sleepms(100);
	_spi->setSpeed(SPI_SPEED_SDCARD_SLOW);
	for(int i = 0; i < 10; i++)
		_spi->clock();

	// Start up the card.
	_cardType = startSequence();
	if(_cardType == SDCARD_NONE)
		return false;

	// Set SPI clock to 18MHz for data transfer.
	sleepms(100);
	_spi->setSpeed(SPI_SPEED_SDCARD_MAX);
	return true;
}

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
