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

// There is a lot of commented out code here. This is all example code stuff
// which I might want to use for additional functionality in the future.

#ifndef SDCARD_KINETIS_DRIVER

#include "SystemTick.h"
#include "board.h"
#include <string.h>

// Speed tokens for SPI.
enum {
	kSpiSpeedSlow =   400000, // 400kHz for slow cards.
	kSpiSpeedFast = 25000000, // 25MHz for fast cards.
};

enum CardType
{
	cardtypeNone  = 0,
	cardtypeMMC   = 1,
	cardtypeSDv1  = 2,
	cardtypeSDv2  = 4,
	cardtypeSdc   = 6,
	cardtypeBlock = 8,
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
enum SDCommand
{
	SDCARD_CMD_GO_IDLE_STATE    =  0,
	SDCARD_CMD_SEND_IF_COND     =  8,
	SDCARD_CMD_SET_BLOCKLEN     = 16,
	SDCARD_CMD_READ_BLOCK       = 17,
	SDCARD_CMD_WRITE_BLOCK      = 24,
	SDCARD_CMD_APP_CMD          = 55,
	SDCARD_CMD_READ_CCS         = 58,
	SDCARD_CMD_WRITE_EXTR_MULTI = 59,
};

// "Application" commands used by this module.
enum SDAppCommand {
	SDCARD_APPCMD_SD_SEND_OP_COND = 41,
};

// Single instance. We assume only the one card slot.
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

bool SDCard::readBlocks(uint8_t *buffer, unsigned startBlock, unsigned blockCount)
{
	for(unsigned i = 0; i < blockCount; i++) {
		if(!readSector(startBlock, buffer))
			return false;

		startBlock++;
		buffer += getBlockSize();
	}
	return true;
}

// Read a 512-byte block of data from the card.
// Obviously make sure your buffer is at least 512 bytes long.
bool SDCard::readSector(unsigned sector, uint8_t *buffer)
{
	select();
	if(!getStatus()) {
		deselect();
		return false;
	}

	// set read address for single block (CMD17)
	if(!isHighCapacity())
		sector <<= 9;

	if(command(SDCARD_CMD_READ_BLOCK, sector) != 0) {
		deselect();
		return false;
	}

	Timeout t(1000);
	while(!t.isExpired())
	{
		// Wait for a start flag (0xFE) from the SD Card.
		uint8_t response = _spi.recv();
		if(response == 0xFE)
		{
			// Read data.
			//memset(buffer, 255, kBlockSize);
			//_spi.exchange(buffer, buffer, kBlockSize);
			_spi.recv(buffer, kBlockSize);

			// Read (and ignore) checksum.
			//uint16_t checksum = 0xFFFF;
			//_spi.exchange((uint8_t *)&checksum, (uint8_t *)&checksum, sizeof(checksum));
			uint16_t checksum;
			_spi.recv((uint8_t *)&checksum, sizeof(checksum));

			deselect();
			return true;
		}
	}

	deselect();
	return false; // Timeout with no data received.
}


// Write a 512-byte block of data to the card.
bool SDCard::writeSector(unsigned sector, const uint8_t *buffer)
{
#ifndef SDCARD_READONLY
	if(_cardType == cardtypeNone)
		return false;

	// set write address for single block (CMD24)
	if(!isHighCapacity())
		sector <<= 9;

    if(command(SDCARD_CMD_WRITE_BLOCK, sector) != 0)
        return false;

    if(!waitReady())
    	return false;

	// Indicate start of block.
	//cardSelect();
	_spi.send(0xFE);

	// Write the data.
	_spi.send(buffer, kBlockSize);

	// Write the (dummy) checksum.
	_spi.send(0xFF);
	_spi.send(0xFF);

	// Check the response token.
	uint8_t r = _spi.recv();
	if((r & 0x1F) != 0x05)
	{
		//cardDeselect();
		return false;
	}

	return true;
#else
	return false;
#endif // SDCARD_READONLY
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

unsigned SDCard::getBlockCount() const
{
	// TODO: Implement me!
	return 0;
}

unsigned SDCard::getBlockSize() const
{
	return kBlockSize;
}

unsigned SDCard::getEraseSectorSize() const
{
	return kBlockSize;
}

// SD Card initialisation procedure.
bool SDCard::init()
{
	_cardType = cardtypeNone;

	// Set SPI clock to 100kHz for initialisation, and clock card with cs = 1
	deselect();
	SystemTick::delay(100);
	_spi.setFrequency(kSpiSpeedSlow);

	// Provide 10 bytes worth of clock.
	uint8_t buf[10];
	_spi.recv(buf, 10);

	// Start up the card.
	_cardType = startSequence();
	if(_cardType == cardtypeNone)
		return false;

	// Increase SPI clock speed for data transfer.
	deselect();
	SystemTick::delay(100);
	_spi.setFrequency(kSpiSpeedFast);
	return true;
}

// SD Card first stage initialisation.
// Returns the card type detected.
unsigned SDCard::startSequence()
{
	// Send CMD0, should enter IDLE STATE.
	select();
	if(command(SDCARD_CMD_GO_IDLE_STATE, 0) != R1_IDLE_STATE) {
		deselect();
		return cardtypeNone; // No card detected.
	}

	// Now attempt to identify the card.

	// Send CMD8 to determine whether it is v1 or v2 interface.
	if(command(SDCARD_CMD_SEND_IF_COND, 0x1AA) == R1_IDLE_STATE) {
		// Card has v2 interface.
		uint8_t ocr[4];
		_spi.recv(ocr, sizeof(ocr));

		if(ocr[2] == 0x01 && ocr[3] == 0xAA)
		  	return initCardV2();
	} else {
		// Card has v1 interface.
		unsigned cardtype = initCardV1();
		if(cardtype != cardtypeNone)
		{
			// Set block length to 512 (CMD16)
			if(command(SDCARD_CMD_SET_BLOCKLEN, kBlockSize) != 0)
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
	uint8_t req[6];
	unsigned p = 0;
	req[p++] = 0x40 | cmd;
	req[p++] = arg >> 24;
	req[p++] = (arg >> 16) & 0xFF;
	req[p++] = (arg >> 8) & 0xFF;
	req[p++] = arg & 0xFF;
	req[p++] = (crc7(req, p) << 1) | 1;
	_spi.send(req, p);

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

bool SDCard::isHighCapacity() const
{
	return 0 != (_cardType & cardtypeBlock);
}

// Block if the card is busy
bool SDCard::waitReady()
{
	Timeout t(1000);

	uint8_t response = _spi.recv();
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
		if(appCommand(SDCARD_APPCMD_SD_SEND_OP_COND, 0) <= 1)
		{
			for(int i = 0; i < 100; i++)
			{
				if(appCommand(SDCARD_APPCMD_SD_SEND_OP_COND, 0) == 0)
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
		if(appCommand(SDCARD_APPCMD_SD_SEND_OP_COND, 0x40000000) == 0)
		{
			if(0 != command(SDCARD_CMD_READ_CCS, 0))
			{
				deselect();
				return cardtypeNone;
			}

			uint8_t ocr[4];
			_spi.recv(ocr, sizeof(ocr));

			if(0 != (ocr[0] & 0x40))
				return cardtypeSDv2 | cardtypeBlock;

			return cardtypeSDv2;
		}
	}

	deselect();
	return cardtypeNone;
}

uint32_t SDCard::crc7(uint8_t *buffer, uint32_t length) const
{
    uint32_t index;
    uint32_t crc = 0;

    static const uint8_t crcTable[] = {0x00U, 0x09U, 0x12U, 0x1BU, 0x24U, 0x2DU, 0x36U, 0x3FU,
                                       0x48U, 0x41U, 0x5AU, 0x53U, 0x6CU, 0x65U, 0x7EU, 0x77U};

    while (length)
    {
        index = (((crc >> 3U) & 0x0FU) ^ ((*buffer) >> 4U));
        crc = ((crc << 4U) ^ crcTable[index]);

        index = (((crc >> 3U) & 0x0FU) ^ ((*buffer) & 0x0FU));
        crc = ((crc << 4U) ^ crcTable[index]);

        buffer++;
        length--;
    }

    return (crc & 0x7FU);
}

#else // SDCARD KINETIS_DRIVER

#include <string.h>

/* Card command maximum timeout value */
#define FSL_SDSPI_TIMEOUT (1000U)

/*! @brief SDSPI card flag */
enum _sdspi_card_flag
{
    kSDSPI_SupportHighCapacityFlag = (1U << 0U), /*!< Card is high capacity */
    kSDSPI_SupportSdhcFlag = (1U << 1U),         /*!< Card is SDHC */
    kSDSPI_SupportSdxcFlag = (1U << 2U),         /*!< Card is SDXC */
    kSDSPI_SupportSdscFlag = (1U << 3U),         /*!< Card is SDSC */
};

/*! @brief Error bit in SPI mode R1 */
enum _sdspi_r1_error_status_flag
{
    kSDSPI_R1InIdleStateFlag = (1U << 0U),        /*!< In idle state */
    kSDSPI_R1EraseResetFlag = (1U << 1U),         /*!< Erase reset */
    kSDSPI_R1IllegalCommandFlag = (1U << 2U),     /*!< Illegal command */
    kSDSPI_R1CommandCrcErrorFlag = (1U << 3U),    /*!< Com crc error */
    kSDSPI_R1EraseSequenceErrorFlag = (1U << 4U), /*!< Erase sequence error */
    kSDSPI_R1AddressErrorFlag = (1U << 5U),       /*!< Address error */
    kSDSPI_R1ParameterErrorFlag = (1U << 6U),     /*!< Parameter error */
};

/*! @brief SD card individual commands */
typedef enum _sd_command
{
    kSD_SendRelativeAddress = 3U,    /*!< Send Relative Address */
    kSD_Switch = 6U,                 /*!< Switch Function */
    kSD_SendInterfaceCondition = 8U, /*!< Send Interface Condition */
    kSD_VoltageSwitch = 11U,         /*!< Voltage Switch */
    kSD_SpeedClassControl = 20U,     /*!< Speed Class control */
    kSD_EraseWriteBlockStart = 32U,  /*!< Write Block Start */
    kSD_EraseWriteBlockEnd = 33U,    /*!< Write Block End */
    kSD_SendTuningBlock = 19U,       /*!< Send Tuning Block */
} sd_command_t;

/*! @brief OCR register in SD card */
enum _sd_ocr_flag
{
    kSD_OcrPowerUpBusyFlag = (1U << 31U),                            /*!< Power up busy status */
    kSD_OcrHostCapacitySupportFlag = (1U << 30U),                    /*!< Card capacity status */
    kSD_OcrCardCapacitySupportFlag = kSD_OcrHostCapacitySupportFlag, /*!< Card capacity status */
    kSD_OcrSwitch18RequestFlag = (1U << 24U),                        /*!< Switch to 1.8V request */
    kSD_OcrSwitch18AcceptFlag = kSD_OcrSwitch18RequestFlag,          /*!< Switch to 1.8V accepted */
    kSD_OcrVdd27_28Flag = (1U << 15U),                               /*!< VDD 2.7-2.8 */
    kSD_OcrVdd28_29Flag = (1U << 16U),                               /*!< VDD 2.8-2.9 */
    kSD_OcrVdd29_30Flag = (1U << 17U),                               /*!< VDD 2.9-3.0 */
    kSD_OcrVdd30_31Flag = (1U << 18U),                               /*!< VDD 2.9-3.0 */
    kSD_OcrVdd31_32Flag = (1U << 19U),                               /*!< VDD 3.0-3.1 */
    kSD_OcrVdd32_33Flag = (1U << 20U),                               /*!< VDD 3.1-3.2 */
    kSD_OcrVdd33_34Flag = (1U << 21U),                               /*!< VDD 3.2-3.3 */
    kSD_OcrVdd34_35Flag = (1U << 22U),                               /*!< VDD 3.3-3.4 */
    kSD_OcrVdd35_36Flag = (1U << 23U),                               /*!< VDD 3.4-3.5 */
};

/*! @brief SD card individual application commands */
typedef enum _sd_application_command
{
    kSD_ApplicationSetBusWdith = 6U,              /*!< Set Bus Width */
    kSD_ApplicationStatus = 13U,                  /*!< Send SD status */
    kSD_ApplicationSendNumberWriteBlocks = 22U,   /*!< Send Number Of Written Blocks */
    kSD_ApplicationSetWriteBlockEraseCount = 23U, /*!< Set Write Block Erase Count */
    kSD_ApplicationSendOperationCondition = 41U,  /*!< Send Operation Condition */
    kSD_ApplicationSetClearCardDetect = 42U,      /*!< Set Connnect/Disconnect pull up on detect pin */
    kSD_ApplicationSendScr = 51U,                 /*!< Send Scr */
} sd_application_command_t;

/*! @brief SD card CSD register flags */
enum _sd_csd_flag
{
    kSD_CsdReadBlockPartialFlag = (1U << 0U),         /*!< Partial blocks for read allowed [79:79] */
    kSD_CsdWriteBlockMisalignFlag = (1U << 1U),       /*!< Write block misalignment [78:78] */
    kSD_CsdReadBlockMisalignFlag = (1U << 2U),        /*!< Read block misalignment [77:77] */
    kSD_CsdDsrImplementedFlag = (1U << 3U),           /*!< DSR implemented [76:76] */
    kSD_CsdEraseBlockEnabledFlag = (1U << 4U),        /*!< Erase single block enabled [46:46] */
    kSD_CsdWriteProtectGroupEnabledFlag = (1U << 5U), /*!< Write protect group enabled [31:31] */
    kSD_CsdWriteBlockPartialFlag = (1U << 6U),        /*!< Partial blocks for write allowed [21:21] */
    kSD_CsdFileFormatGroupFlag = (1U << 7U),          /*!< File format group [15:15] */
    kSD_CsdCopyFlag = (1U << 8U),                     /*!< Copy flag [14:14] */
    kSD_CsdPermanentWriteProtectFlag = (1U << 9U),    /*!< Permanent write protection [13:13] */
    kSD_CsdTemporaryWriteProtectFlag = (1U << 10U),   /*!< Temporary write protection [12:12] */
};

/*! @brief Data Token */
typedef enum _sdspi_data_token
{
    kSDSPI_DataTokenBlockRead = 0xFEU,          /*!< Single block read, multiple block read */
    kSDSPI_DataTokenSingleBlockWrite = 0xFEU,   /*!< Single block write */
    kSDSPI_DataTokenMultipleBlockWrite = 0xFCU, /*!< Multiple block write */
    kSDSPI_DataTokenStopTransfer = 0xFDU,       /*!< Stop transmission */
} sdspi_data_token_t;

/*! @brief SD card CID register */
typedef struct _sd_cid
{
    uint8_t manufacturerID;                     /*!< Manufacturer ID [127:120] */
    uint16_t applicationID;                     /*!< OEM/Application ID [119:104] */
    uint8_t productName[SD_PRODUCT_NAME_BYTES]; /*!< Product name [103:64] */
    uint8_t productVersion;                     /*!< Product revision [63:56] */
    uint32_t productSerialNumber;               /*!< Product serial number [55:24] */
    uint16_t manufacturerData;                  /*!< Manufacturing date [19:8] */
} sd_cid_t;


/* Rate unit(divided by 1000) of transfer speed in non-high-speed mode. */
static const uint32_t g_transferSpeedRateUnit[] = {
    /* 100Kbps, 1Mbps, 10Mbps, 100Mbps*/
    100U, 1000U, 10000U, 100000U,
};

/* Multiplier factor(multiplied by 1000) of transfer speed in non-high-speed mode. */
static const uint32_t g_transferSpeedMultiplierFactor[] = {
    0U, 1000U, 1200U, 1300U, 1500U, 2000U, 2500U, 3000U, 3500U, 4000U, 4500U, 5000U, 5500U, 6000U, 7000U, 8000U,
};

// Single instance. We assume only the one card slot.
static SDCard *g_sdCard = 0;

SDCard *SDCard::instance()
{
	return g_sdCard;
}

SDCard::SDCard()
	: _csPort(SDCARD_CS_PORT)
	, _ocr(0)
	, _flags(0)
	, _blockSize(0)
    , _blockCount(0)
	//	, _cardType(cardtypeNone)
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
	return true; // Or just always return true. Keeping it simple.
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

// SD Card initialisation procedure.
bool SDCard::init()
{
	uint32_t elapsedTime;
	uint8_t  response[5];
	uint32_t applicationCommand41Argument = 0U;
	uint8_t  applicationCommand41Response[5U];
	bool     likelySdV1 = false;

	// Select slow speed for initialization.
	deselect();
	_spi.setFrequency(kSpiSpeedSlow);

	// Card wakeup procedure.
	uint8_t wakeup[10];
	memset(wakeup, 0xFF, 10);
	_spi.exchange(wakeup, wakeup, 10);
	select(); // Enable SPI mode.

	// Reset the card by CMD0.
	if(!goIdle())
		return false;

	// Check the card's supported interface condition.
	if(!sendInterfaceCondition(0xAA, response)) {
		likelySdV1 = true;
	} else if ((response[3] == 0x1) || (response[4] == 0xAA)) {
		applicationCommand41Argument |= kSD_OcrHostCapacitySupportFlag;
	} else {
		return false; // kStatus_SDSPI_SendInterfaceConditionFailed;
	}

	// Set card's interface condition according to host's capability and card's supported interface condition
	unsigned startTime = SystemTick::getMilliseconds();
	do
	{
		if(!applicationSendOperationCondition(applicationCommand41Argument, applicationCommand41Response))
			return false; // kStatus_SDSPI_SendOperationConditionFailed;

		unsigned currentTime = SystemTick::getMilliseconds();
		unsigned elapsedTime = (currentTime - startTime);
		if (elapsedTime > 500)
			return false; // kStatus_Timeout;

		if(0 == applicationCommand41Response[0])
			break;
	} while (applicationCommand41Response[0U] & kSDSPI_R1InIdleStateFlag);

	if(!likelySdV1)
	{
		if(!readOcr())
			return false; // kStatus_SDSPI_ReadOcrFailed;

		if(0 != (_ocr & kSD_OcrCardCapacitySupportFlag))
			_flags |= kSDSPI_SupportHighCapacityFlag;
	}

	// Force to use 512-byte length block, no matter which version.
	if(!setBlockSize(512))
		return false; // kStatus_SDSPI_SetBlockSizeFailed;

	if(!sendCsd())
		return false; // kStatus_SDSPI_SendCsdFailed;

	// Set to max frequency according to the max frequency information in CSD register.
	setMaxFrequencyNormalMode();

	// Save capacity, read only attribute and CID, SCR registers.
	checkCapacity();
	checkReadOnly();
	if(!sendCid())
		return false; // kStatus_SDSPI_SendCidFailed;

	if(!sendScr())
		return false; // kStatus_SDSPI_SendCidFailed;

	return true;

#ifdef OLD
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
#endif
}

/*!
 * @brief Send GO_IDLE command.
 *
 * @param card Card descriptor.
 * @retval kStatus_SDSPI_ExchangeFailed Send timing byte failed.
 * @retval kStatus_SDSPI_SendCommandFailed Send command failed.
 * @retval kStatus_SDSPI_ResponseError Response is error.
 * @retval kStatus_Success Operate successfully.
 */
bool SDCard::goIdle()
{
    sdspi_command_t command = {0};
    uint32_t retryCount = 200U;

    // SD card will enter SPI mode if the CS is asserted (negative) during
    // the reception of the reset command (CMD0) and the card will be IDLE state.
    while(retryCount--)
    {
        command.index = kSDMMC_GoIdleState;
        command.responseType = kSDSPI_ResponseTypeR1;
        if(sendCommand(&command, FSL_SDSPI_TIMEOUT) && command.response[0U] == kSDSPI_R1InIdleStateFlag)
            return true;
    }

    return false;
}

/*!
 * @brief Send command.
 *
 * @param command The command to be wrote.
 * @param timeout The timeout value.
 * @retval kStatus_SDSPI_WaitReadyFailed Wait ready failed.
 * @retval kStatus_SDSPI_ExchangeFailed Exchange data over SPI failed.
 * @retval kStatus_SDSPI_ResponseError Response is error.
 * @retval kStatus_Fail Send command failed.
 * @retval kStatus_Success Operate successfully.
 */
bool SDCard::sendCommand(sdspi_command_t *command, uint32_t timeout)
{
    uint8_t buffer[6U];
    uint8_t response;
    uint8_t i;
    uint8_t timingByte = 0xFFU; /* The byte need to be sent as read/write data block timing requirement */

    if(!waitReady(timeout) && command->index != kSDMMC_GoIdleState)
        return false; // kStatus_SDSPI_WaitReadyFailed;

    // Send command.
    buffer[0U] = (command->index | 0x40U);
    buffer[1U] = ((command->argument >> 24U) & 0xFFU);
    buffer[2U] = ((command->argument >> 16U) & 0xFFU);
    buffer[3U] = ((command->argument >> 8U) & 0xFFU);
    buffer[4U] = (command->argument & 0xFFU);
    buffer[5U] = ((generateCRC7(buffer, 5U, 0U) << 1U) | 1U);
    _spi.exchange(buffer, 0, sizeof(buffer));

    /* Wait for the response coming, the left most bit which is transfered first in first response byte is 0 */
    for(i = 0; i < 9; i++)
    {
        _spi.exchange(&timingByte, &response, 1U);

        // Check if response 0 coming.
        if (!(response & 0x80U))
            break;
    }
    if(response & 0x80U) // Max index byte is high means response comming.
        return false; // kStatus_SDSPI_ResponseEr

    // Receve all the response content.
    command->response[0U] = response;
    switch (command->responseType)
    {
        case kSDSPI_ResponseTypeR1:
            break;

        case kSDSPI_ResponseTypeR1b:
            if(!waitReady(timeout))
                return false; // kStatus_SDSPI_WaitReadyFailed;
            break;

        case kSDSPI_ResponseTypeR2:
            _spi.exchange(&timingByte, &(command->response[1]), 1);
            break;

        case kSDSPI_ResponseTypeR3:
        case kSDSPI_ResponseTypeR7:
            // Left 4 bytes in response type R3 and R7(total 5 bytes in SPI mode)
            _spi.exchange(&timingByte, &(command->response[1]), 4);
            break;

        default:
            return false; // kStatus_Fail;
    }
    return true;
}

bool SDCard::sendApplicationCmd()
{
    sdspi_command_t command = {0};

    command.index = kSDMMC_ApplicationCommand;
    command.responseType = kSDSPI_ResponseTypeR1;
    if(!sendCommand(&command, FSL_SDSPI_TIMEOUT))
        return false; // kStatus_SDSPI_SendCommandFailed;

    if((command.response[0U]) && (!(command.response[0U] & kSDSPI_R1InIdleStateFlag)))
        return false; // kStatus_SDSPI_ResponseError;

    return true;
}

bool SDCard::waitReady(unsigned milliseconds)
{
    uint8_t response;
    unsigned elapsedTime;
    uint8_t timingByte = 0xFFU; /* The byte need to be sent as read/write data block timing requirement */

    unsigned startTime = SystemTick::getMilliseconds();
    do
    {
        _spi.exchange(&timingByte, &response, 1);

        unsigned currentTime = SystemTick::getMilliseconds();
        elapsedTime = (currentTime - startTime);
    } while ((response != 0xFF) && (elapsedTime < milliseconds));

    // Response 0xFF means card is still busy.
    if(response != 0xFFU)
        return false; //kStatus_SDSPI_ResponseError;

    return true;
}

uint32_t SDCard::generateCRC7(uint8_t *buffer, uint32_t length, uint32_t crc) const
{
    uint32_t index;

    static const uint8_t crcTable[] = {0x00U, 0x09U, 0x12U, 0x1BU, 0x24U, 0x2DU, 0x36U, 0x3FU,
                                       0x48U, 0x41U, 0x5AU, 0x53U, 0x6CU, 0x65U, 0x7EU, 0x77U};

    while (length)
    {
        index = (((crc >> 3U) & 0x0FU) ^ ((*buffer) >> 4U));
        crc = ((crc << 4U) ^ crcTable[index]);

        index = (((crc >> 3U) & 0x0FU) ^ ((*buffer) & 0x0FU));
        crc = ((crc << 4U) ^ crcTable[index]);

        buffer++;
        length--;
    }

    return (crc & 0x7FU);
}

bool SDCard::sendInterfaceCondition(uint8_t pattern, uint8_t *response)
{
    sdspi_command_t command = {0};

    command.index = kSD_SendInterfaceCondition;
    command.argument = (0x100U | (pattern & 0xFFU));
    command.responseType = kSDSPI_ResponseTypeR7;
    if(!sendCommand(&command, FSL_SDSPI_TIMEOUT))
        return false; // kStatus_SDSPI_SendCommandFailed;

    memcpy(response, command.response, sizeof(command.response));

    return true;
}

bool SDCard::applicationSendOperationCondition(uint32_t argument, uint8_t *response)
{
    sdspi_command_t command = {0};
    unsigned elapsedTime = 0U;

    command.index = kSD_ApplicationSendOperationCondition;
    command.argument = argument;
    command.responseType = kSDSPI_ResponseTypeR1;

    unsigned startTime = SystemTick::getMilliseconds();
    do
    {
        if(sendApplicationCmd())
        {
            if(sendCommand(&command, FSL_SDSPI_TIMEOUT))
            {
                if (!command.response[0U])
                    break;
            }
        }

        unsigned currentTime = SystemTick::getMilliseconds();
        elapsedTime = (currentTime - startTime);
    } while (elapsedTime < FSL_SDSPI_TIMEOUT);

    if (response)
        memcpy(response, command.response, sizeof(command.response));

    if (elapsedTime < FSL_SDSPI_TIMEOUT)
        return true;

    return false; // kStatus_Timeout;
}

bool SDCard::readOcr()
{
    sdspi_command_t command = {0};

    command.index = kSDMMC_ReadOcr;
    command.responseType = kSDSPI_ResponseTypeR3;
    if(!sendCommand(&command, FSL_SDSPI_TIMEOUT))
        return false; // kStatus_SDSPI_SendCommandFailed;

    if(command.response[0])
        return false; // kStatus_SDSPI_ResponseError;

    // Switch the bytes sequence. All register's content is transferred from highest byte to lowest byte.
    _ocr = 0U;
    for(unsigned i = 4U; i > 0U; i--)
        _ocr |= (uint32_t)command.response[i] << ((4U - i) * 8U);

    return true;
}

bool SDCard::setBlockSize(uint32_t blockSize)
{
    sdspi_command_t command = {0};

    command.index = kSDMMC_SetBlockLength;
    command.argument = blockSize;
    command.responseType = kSDSPI_ResponseTypeR1;

    return sendCommand(&command, FSL_SDSPI_TIMEOUT);
}

bool SDCard::sendCsd()
{
    sdspi_command_t command = {0};

    command.index = kSDMMC_SendCsd;
    command.responseType = kSDSPI_ResponseTypeR1;
    if(!sendCommand(&command, FSL_SDSPI_TIMEOUT))
        return false; // kStatus_SDSPI_SendCommandFailed;

    if(!read(_rawCsd, sizeof(_rawCsd)))
        return false; // kStatus_SDSPI_ReadFailed;

    decodeCsd(_rawCsd);

    return true;
}

#define SD_TRANSFER_SPEED_RATE_UNIT_SHIFT (0U)
#define SD_TRANSFER_SPEED_RATE_UNIT_MASK (0x07U)
#define SD_TRANSFER_SPEED_TIME_VALUE_SHIFT (2U)
#define SD_TRANSFER_SPEED_TIME_VALUE_MASK (0x78U)
#define SD_RD_TRANSFER_SPEED_RATE_UNIT(x) \
    (((x.transferSpeed) & SD_TRANSFER_SPEED_RATE_UNIT_MASK) >> SD_TRANSFER_SPEED_RATE_UNIT_SHIFT)
#define SD_RD_TRANSFER_SPEED_TIME_VALUE(x) \
    (((x.transferSpeed) & SD_TRANSFER_SPEED_TIME_VALUE_MASK) >> SD_TRANSFER_SPEED_TIME_VALUE_SHIFT)

bool SDCard::setMaxFrequencyNormalMode()
{
    // Calculate max frequency card supported in non-high-speed mode.
    uint32_t maxFrequency = g_transferSpeedRateUnit[SD_RD_TRANSFER_SPEED_RATE_UNIT(_csd)] *
                            g_transferSpeedMultiplierFactor[SD_RD_TRANSFER_SPEED_TIME_VALUE(_csd)];

    _spi.setFrequency(maxFrequency);
    return true;
}

void SDCard::checkCapacity()
{
    uint32_t deviceSize;
    uint32_t deviceSizeMultiplier;
    uint32_t readBlockLength;

    if(0 != _csd.csdStructure)
    {
        // SD CSD structure v2.xx
        deviceSize = _csd.deviceSize;
        if (deviceSize >= 0xFFFFU) // Bigger than 32GB
        {
            // extended capacity
            _flags |= kSDSPI_SupportSdxcFlag;
        }
        else
        {
            _flags |= kSDSPI_SupportSdhcFlag;
        }
        deviceSizeMultiplier = 10U;
        deviceSize += 1U;
        readBlockLength = 9U;
    }
    else
    {
        // SD CSD structure v1.xx
        deviceSize = _csd.deviceSize + 1;
        deviceSizeMultiplier = _csd.deviceSizeMultiplier + 2;
        readBlockLength = _csd.readBlockLength;

        // Card maximum capacity is 2GB when CSD structure version is 1.0
        _flags |= kSDSPI_SupportSdscFlag;
    }
    if(readBlockLength != 9)
    {
        // Force to use 512-byte length block
        deviceSizeMultiplier += (readBlockLength - 9U);
        readBlockLength = 9U;
    }

    _blockSize = (1U << readBlockLength);
    _blockCount = (deviceSize << deviceSizeMultiplier);
}

bool SDCard::checkReadOnly()
{
    if((_csd.flags & kSD_CsdPermanentWriteProtectFlag) || (_csd.flags & kSD_CsdTemporaryWriteProtectFlag))
        return true;

    return false;
}

/*!
 * @brief Send GET-CID command
 *
 * @param card Card descriptor.
 * @retval kStatus_SDSPI_SendCommandFailed Send command failed.
 * @retval kStatus_SDSPI_ReadFailed Read data blocks failed.
 * @retval kStatus_Success Operate successfully.
 */
bool SDCard::sendCid()
{
    sdspi_command_t command = {0};

    command.index = kSDMMC_SendCid;
    command.responseType = kSDSPI_ResponseTypeR1;
    if(!sendCommand(&command, FSL_SDSPI_TIMEOUT))
        return false; // kStatus_SDSPI_SendCommandFailed;

    if(!read(_rawCid, sizeof(_rawCid)))
        return false; // kStatus_SDSPI_ReadFailed;

    decodeCid(_rawCid);

    return true;
}

/*!
 * @brief Send SEND_SCR command.
 *
 * @param card Card descriptor.
 * @retval kStatus_SDSPI_SendCommandFailed Send command failed.
 * @retval kStatus_SDSPI_ReadFailed Read data blocks failed.
 * @retval kStatus_Success Operate successfully.
 */
bool SDCard::sendScr()
{
    sdspi_command_t command = {0};

    if(!sendApplicationCmd())
        return false; // kStatus_SDSPI_SendApplicationCommandFailed;

    command.index = kSD_ApplicationSendScr;
    command.responseType = kSDSPI_ResponseTypeR1;
    if(!sendCommand(&command, FSL_SDSPI_TIMEOUT))
        return false; // kStatus_SDSPI_SendCommandFailed;

    if(!(read(_rawScr, sizeof(_rawScr))))
        return false; // kStatus_SDSPI_ReadFailed;

    decodeScr(_rawScr);

    return true;
}

void SDCard::decodeCsd(uint8_t *rawCsd)
{
    sd_csd_t *csd;

    csd = &_csd;
    csd->csdStructure = (rawCsd[0U] >> 6U);
    csd->dataReadAccessTime1 = rawCsd[1U];
    csd->dataReadAccessTime2 = rawCsd[2U];
    csd->transferSpeed = rawCsd[3U];
    csd->cardCommandClass = (((uint32_t)rawCsd[4U] << 4U) | ((uint32_t)rawCsd[5U] >> 4U));
    csd->readBlockLength = ((rawCsd)[5U] & 0xFU);
    if (rawCsd[6U] & 0x80U)
    {
        csd->flags |= kSD_CsdReadBlockPartialFlag;
    }
    if (rawCsd[6U] & 0x40U)
    {
        csd->flags |= kSD_CsdWriteBlockMisalignFlag;
    }
    if (rawCsd[6U] & 0x20U)
    {
        csd->flags |= kSD_CsdReadBlockMisalignFlag;
    }
    if (rawCsd[6U] & 0x10U)
    {
        csd->flags |= kSD_CsdDsrImplementedFlag;
    }

    /* Some fileds is different when csdStructure is different. */
    if (csd->csdStructure == 0U) /* Decode the bits when CSD structure is version 1.0 */
    {
        csd->deviceSize =
            ((((uint32_t)rawCsd[6] & 0x3U) << 10U) | ((uint32_t)rawCsd[7U] << 2U) | ((uint32_t)rawCsd[8U] >> 6U));
        csd->readCurrentVddMin = ((rawCsd[8U] >> 3U) & 7U);
        csd->readCurrentVddMax = (rawCsd[8U] >> 7U);
        csd->writeCurrentVddMin = ((rawCsd[9U] >> 5U) & 7U);
        csd->writeCurrentVddMax = (rawCsd[9U] >> 2U);
        csd->deviceSizeMultiplier = (((rawCsd[9U] & 3U) << 1U) | (rawCsd[10U] >> 7U));
        _blockCount = (csd->deviceSize + 1U) << (csd->deviceSizeMultiplier + 2U);
        _blockSize = (1U << (csd->readBlockLength));
        if(_blockSize != kBlockSize)
        {
            _blockCount = (_blockCount * _blockSize);
            _blockSize = kBlockSize;
            _blockCount = (_blockCount / _blockSize);
        }
    }
    else if (csd->csdStructure == 1U) /* Decode the bits when CSD structure is version 2.0 */
    {
        _blockSize = kBlockSize;
        csd->deviceSize =
            ((((uint32_t)rawCsd[7U] & 0x3FU) << 16U) | ((uint32_t)rawCsd[8U] << 8U) | ((uint32_t)rawCsd[9U]));
        if (csd->deviceSize >= 0xFFFFU)
        {
            _flags |= kSDSPI_SupportSdxcFlag;
        }
        _blockCount = ((csd->deviceSize + 1U) * 1024U);
    }
    else
    {
    }

    if ((rawCsd[10U] >> 6U) & 1U)
    {
        csd->flags |= kSD_CsdEraseBlockEnabledFlag;
    }
    csd->eraseSectorSize = (((rawCsd[10U] & 0x3FU) << 1U) | (rawCsd[11U] >> 7U));
    csd->writeProtectGroupSize = (rawCsd[11U] & 0x7FU);
    if (rawCsd[12U] >> 7U)
    {
        csd->flags |= kSD_CsdWriteProtectGroupEnabledFlag;
    }
    csd->writeSpeedFactor = ((rawCsd[12U] >> 2U) & 7U);
    csd->writeBlockLength = (((rawCsd[12U] & 3U) << 2U) | (rawCsd[13U] >> 6U));
    if ((rawCsd[13U] >> 5U) & 1U)
    {
        csd->flags |= kSD_CsdWriteBlockPartialFlag;
    }
    if (rawCsd[14U] >> 7U)
    {
        csd->flags |= kSD_CsdFileFormatGroupFlag;
    }
    if ((rawCsd[14U] >> 6U) & 1U)
    {
        csd->flags |= kSD_CsdCopyFlag;
    }
    if ((rawCsd[14U] >> 5U) & 1U)
    {
        csd->flags |= kSD_CsdPermanentWriteProtectFlag;
    }
    if ((rawCsd[14U] >> 4U) & 1U)
    {
        csd->flags |= kSD_CsdTemporaryWriteProtectFlag;
    }
    csd->fileFormat = ((rawCsd[14U] >> 2U) & 3U);
}

void SDCard::decodeCid(uint8_t *rawCid)
{
    sd_cid_t *cid = &(_cid);
    cid->manufacturerID = rawCid[0U];
    cid->applicationID = (((uint32_t)rawCid[1U] << 8U) | (uint32_t)(rawCid[2U]));
    memcpy(cid->productName, &rawCid[3U], SD_PRODUCT_NAME_BYTES);
    cid->productVersion = rawCid[8U];
    cid->productSerialNumber = (((uint32_t)rawCid[9U] << 24U) | ((uint32_t)rawCid[10U] << 16U) |
                                ((uint32_t)rawCid[11U] << 8U) | ((uint32_t)rawCid[12U]));
    cid->manufacturerData = ((((uint32_t)rawCid[13U] & 0x0FU) << 8U) | ((uint32_t)rawCid[14U]));
}

void SDCard::decodeScr(uint8_t *rawScr)
{
    sd_scr_t *scr = &(_scr);
    scr->scrStructure = ((rawScr[0U] & 0xF0U) >> 4U);
    scr->sdSpecification = (rawScr[0U] & 0x0FU);
    if (rawScr[1U] & 0x80U)
    {
        scr->flags |= kSD_ScrDataStatusAfterErase;
    }
    scr->sdSecurity = ((rawScr[1U] & 0x70U) >> 4U);
    scr->sdBusWidths = (rawScr[1U] & 0x0FU);
    if (rawScr[2U] & 0x80U)
    {
        scr->flags |= kSD_ScrSdSpecification3;
    }
    scr->extendedSecurity = ((rawScr[2U] & 0x78U) >> 3U);
    scr->commandSupport = (rawScr[3U] & 0x03U);
    scr->reservedForManufacturer = (((uint32_t)rawScr[4U] << 24U) | ((uint32_t)rawScr[5U] << 16U) |
                                    ((uint32_t)rawScr[6U] << 8U) | (uint32_t)rawScr[7U]);
}

#define IS_BLOCK_ACCESS(x) ((x)->_flags & kSDSPI_SupportHighCapacityFlag)

bool SDCard::readBlocks(uint8_t *buffer, unsigned startBlock, unsigned blockCount)
{
    sdspi_command_t command = {0};

    unsigned offset = startBlock;
    if (!IS_BLOCK_ACCESS(this))
        offset *= _blockSize;

    /* Send command and reads data. */
    command.argument = offset;
    command.responseType = kSDSPI_ResponseTypeR1;
    if (blockCount == 1)
    {
        command.index = kSDMMC_ReadSingleBlock;
        if(!sendCommand(&command, FSL_SDSPI_TIMEOUT))
            return false; //kStatus_SDSPI_SendCommandFailed;

        if(!read(buffer, _blockSize))
            return false; // kStatus_SDSPI_ReadFailed;
    }
    else
    {
        command.index = kSDMMC_ReadMultipleBlock;
        if(!sendCommand(&command, FSL_SDSPI_TIMEOUT))
            return false; //kStatus_SDSPI_SendCommandFailed;

        for(unsigned i = 0; i < blockCount; i++)
        {
            if(!read(buffer, _blockSize))
                return false; // kStatus_SDSPI_ReadFailed;

            buffer += _blockSize;
        }

        // Write stop transmission command after the last data block.
        if(!stopTransmission())
            return false; // kStatus_SDSPI_StopTransmissionFailed;
    }

    return true;
}

bool SDCard::read(uint8_t *buffer, uint32_t size)
{
    uint32_t elapsedTime;
    uint8_t response, i;
    uint8_t timingByte = 0xFFU; /* The byte need to be sent as read/write data block timing requirement */

    memset(buffer, 0xFFU, size);

    // Wait data token comming
    uint32_t startTime = SystemTick::getMilliseconds();
    do
    {
        _spi.exchange(&timingByte, &response, 1U);

        uint32_t currentTime = SystemTick::getMilliseconds();
        elapsedTime = (currentTime - startTime);
    } while ((response == 0xFFU) && (elapsedTime < 100U));

    /* Check data token and exchange data. */
    if (response != kSDSPI_DataTokenBlockRead)
        return false; // kStatus_SDSPI_ResponseError;

    _spi.exchange(buffer, buffer, size);

    // Get 16 bit CRC
    for(i = 0; i < 2; i++)
    {
        _spi.exchange(&timingByte, &response, 1);
    }

    return true;
}

bool SDCard::stopTransmission()
{
    sdspi_command_t command = {0};

    command.index = kSDMMC_StopTransmission;
    command.responseType = kSDSPI_ResponseTypeR1b;
    return sendCommand(&command, FSL_SDSPI_TIMEOUT);
}

#endif // SDCARD_KINETIS_DRIVER




#ifdef EXAMPLE

/*!
 * @brief Wait card to be ready state.
 *
 * @param host Host state.
 * @param milliseconds Timeout time in millseconds.
 * @retval kStatus_SDSPI_ExchangeFailed Exchange data over SPI failed.
 * @retval kStatus_SDSPI_ResponseError Response is error.
 * @retval kStatus_Success Operate successfully.
 */
static status_t SDSPI_WaitReady(sdspi_host_t *host, uint32_t milliseconds);

/*!
 * @brief Calculate CRC7
 *
 * @param buffer Data buffer.
 * @param length Data length.
 * @param crc The orginal crc value.
 * @return Generated CRC7.
 */
static uint32_t SDSPI_GenerateCRC7(uint8_t *buffer, uint32_t length, uint32_t crc);


/*!
 * @brief Send GET_INTERFACE_CONDITION command.
 *
 * This function checks card interface condition, which includes host supply voltage information and asks the card
 * whether it supports voltage.
 *
 * @param card Card descriptor.
 * @param pattern The check pattern.
 * @param response Buffer to save the command response.
 * @retval kStatus_SDSPI_SendCommandFailed Send command failed.
 * @retval kStatus_Success Operate successfully.
 */
static status_t SDSPI_SendInterfaceCondition(sdspi_card_t *card, uint8_t pattern, uint8_t *response);

/*!
 * @brief Send SEND_APPLICATION_COMMAND command.
 *
 * @param card Card descriptor.
 * @retval kStatus_SDSPI_SendCommandFailed Send command failed.
 * @retval kStatus_SDSPI_ResponseError Response is error.
 * @retval kStatus_Success Operate successfully.
 */
static status_t SDSPI_SendApplicationCmd(sdspi_card_t *card);

/*!
 * @brief Send GET_OPERATION_CONDITION command.
 *
 * @param card Card descriptor.
 * @param argument Operation condition.
 * @param response Buffer to save command response.
 * @retval kStatus_Timeout Timeout.
 * @retval kStatus_Success Operate successfully.
 */
static status_t SDSPI_ApplicationSendOperationCondition(sdspi_card_t *card, uint32_t argument, uint8_t *response);

/*!
 * @brief Send READ_OCR command to get OCR register content.
 *
 * @param card Card descriptor.
 * @retval kStatus_SDSPI_SendCommandFailed Send command failed.
 * @retval kStatus_SDSPI_ResponseError Response is error.
 * @retval kStatus_Success Operate successfully.
 */
static status_t SDSPI_ReadOcr(sdspi_card_t *card);

/*!
 * @brief Send SET_BLOCK_SIZE command.
 *
 * This function sets the block length in bytes for SDSC cards. For SDHC cards, it does not affect memory
 * read or write commands, always 512 bytes fixed block length is used.
 * @param card Card descriptor.
 * @param blockSize Block size.
 * @retval kStatus_SDSPI_SendCommandFailed Send command failed.
 * @retval kStatus_Success Operate successfully.
 */
static status_t SDSPI_SetBlockSize(sdspi_card_t *card, uint32_t blockSize);

/*!
 * @brief Read data from card
 *
 * @param host Host state.
 * @param buffer Buffer to save data.
 * @param size The data size to read.
 * @retval kStatus_SDSPI_ResponseError Response is error.
 * @retval kStatus_SDSPI_ExchangeFailed Exchange data over SPI failed.
 * @retval kStatus_Success Operate successfully.
 */
static status_t SDSPI_Read(sdspi_host_t *host, uint8_t *buffer, uint32_t size);

/*!
 * @brief Decode CSD register
 *
 * @param card Card descriptor.
 * @param rawCsd Raw CSD register content.
 */
static void SDSPI_DecodeCsd(sdspi_card_t *card, uint8_t *rawCsd);

/*!
 * @brief Send GET-CSD command.
 *
 * @param card Card descriptor.
 * @retval kStatus_SDSPI_SendCommandFailed Send command failed.
 * @retval kStatus_SDSPI_ReadFailed Read data blocks failed.
 * @retval kStatus_Success Operate successfully.
 */
static status_t SDSPI_SendCsd(sdspi_card_t *card);

/*!
 * @brief Set card to max frequence in normal mode.
 *
 * @param card Card descriptor.
 * @retval kStatus_SDSPI_SetFrequencyFailed Set frequency failed.
 * @retval kStatus_Success Operate successfully.
 */
static status_t SDSPI_SetMaxFrequencyNormalMode(sdspi_card_t *card);

/*!
 * @brief Check the capacity of the card
 *
 * @param card Card descriptor.
 */
static void SDSPI_CheckCapacity(sdspi_card_t *card);

/*!
 * @brief Decode raw CID register.
 *
 * @param card Card descriptor.
 * @param rawCid Raw CID register content.
 */
static void SDSPI_DecodeCid(sdspi_card_t *card, uint8_t *rawCid);


/*!
 * @brief Decode SCR register.
 *
 * @param card Card descriptor.
 * @param rawScr Raw SCR register content.
 */
static void SDSPI_DecodeScr(sdspi_card_t *card, uint8_t *rawScr);


/*!
 * @brief Send STOP_TRANSMISSION command to card to stop ongoing data transferring.
 *
 * @param card Card descriptor.
 * @retval kStatus_SDSPI_SendCommandFailed Send command failed.
 * @retval kStatus_Success Operate successfully.
 */
static status_t SDSPI_StopTransmission(sdspi_card_t *card);

/*!
 * @brief Write data to card
 *
 * @param host Host state.
 * @param buffer Data to send.
 * @param size Data size.
 * @param token The data token.
 * @retval kStatus_SDSPI_WaitReadyFailed Card is busy error.
 * @retval kStatus_SDSPI_ExchangeFailed Exchange data over SPI failed.
 * @retval kStatus_InvalidArgument Invalid argument.
 * @retval kStatus_SDSPI_ResponseError Response is error.
 * @retval kStatus_Success Operate successfully.
 */
static status_t SDSPI_Write(sdspi_host_t *host, uint8_t *buffer, uint32_t size, uint8_t token);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
static status_t SDSPI_Read(sdspi_host_t *host, uint8_t *buffer, uint32_t size)
{
    assert(host);
    assert(host->exchange);
    assert(buffer);
    assert(size);

    uint32_t startTime;
    uint32_t currentTime;
    uint32_t elapsedTime;
    uint8_t response, i;
    uint8_t timingByte = 0xFFU; /* The byte need to be sent as read/write data block timing requirement */

    memset(buffer, 0xFFU, size);

    /* Wait data token comming */
    startTime = host->getCurrentMilliseconds();
    do
    {
        if (kStatus_Success != host->exchange(&timingByte, &response, 1U))
        {
            return kStatus_SDSPI_ExchangeFailed;
        }

        currentTime = host->getCurrentMilliseconds();
        elapsedTime = (currentTime - startTime);
    } while ((response == 0xFFU) && (elapsedTime < 100U));

    /* Check data token and exchange data. */
    if (response != kSDSPI_DataTokenBlockRead)
    {
        return kStatus_SDSPI_ResponseError;
    }
    if (host->exchange(buffer, buffer, size))
    {
        return kStatus_SDSPI_ExchangeFailed;
    }

    /* Get 16 bit CRC */
    for (i = 0U; i < 2U; i++)
    {
        if (kStatus_Success != host->exchange(&timingByte, &response, 1U))
        {
            return kStatus_SDSPI_ExchangeFailed;
        }
    }

    return kStatus_Success;
}



static status_t SDSPI_Write(sdspi_host_t *host, uint8_t *buffer, uint32_t size, uint8_t token)
{
    assert(host);
    assert(host->exchange);

    uint8_t response;
    uint8_t i;
    uint8_t timingByte = 0xFFU; /* The byte need to be sent as read/write data block timing requirement */

    if (kStatus_Success != SDSPI_WaitReady(host, FSL_SDSPI_TIMEOUT))
    {
        return kStatus_SDSPI_WaitReadyFailed;
    }

    /* Write data token. */
    if (host->exchange(&token, NULL, 1U))
    {
        return kStatus_SDSPI_ExchangeFailed;
    }
    if (token == kSDSPI_DataTokenStopTransfer)
    {
        return kStatus_Success;
    }

    if ((!size) || (!buffer))
    {
        return kStatus_InvalidArgument;
    }

    /* Write data. */
    if (kStatus_Success != host->exchange(buffer, NULL, size))
    {
        return kStatus_SDSPI_ExchangeFailed;
    }

    /* Get the last two bytes CRC */
    for (i = 0U; i < 2U; i++)
    {
        if (host->exchange(&timingByte, NULL, 1U))
        {
            return kStatus_SDSPI_ExchangeFailed;
        }
    }

    /* Get the response token. */
    if (host->exchange(&timingByte, &response, 1U))
    {
        return kStatus_SDSPI_ExchangeFailed;
    }
    if ((response & SDSPI_DATA_RESPONSE_TOKEN_MASK) != kSDSPI_DataResponseTokenAccepted)
    {
        return kStatus_SDSPI_ResponseError;
    }

    return kStatus_Success;
}


void SDSPI_Deinit(sdspi_card_t *card)
{
    assert(card);

    memset(card, 0, sizeof(sdspi_card_t));
}

status_t SDSPI_ReadBlocks(sdspi_card_t *card, uint8_t *buffer, uint32_t startBlock, uint32_t blockCount)
{
    assert(card);
    assert(card->host);
    assert(buffer);
    assert(blockCount);

    uint32_t offset;
    uint32_t i;
    sdspi_command_t command = {0};
    sdspi_host_t *host;

    offset = startBlock;
    if (!IS_BLOCK_ACCESS(card))
    {
        offset *= card->blockSize;
    }

    /* Send command and reads data. */
    host = card->host;
    command.argument = offset;
    command.responseType = kSDSPI_ResponseTypeR1;
    if (blockCount == 1U)
    {
        command.index = kSDMMC_ReadSingleBlock;
        if (kStatus_Success != SDSPI_SendCommand(host, &command, FSL_SDSPI_TIMEOUT))
        {
            return kStatus_SDSPI_SendCommandFailed;
        }

        if (kStatus_Success != SDSPI_Read(host, buffer, card->blockSize))
        {
            return kStatus_SDSPI_ReadFailed;
        }
    }
    else
    {
        command.index = kSDMMC_ReadMultipleBlock;
        if (kStatus_Success != SDSPI_SendCommand(host, &command, FSL_SDSPI_TIMEOUT))
        {
            return kStatus_SDSPI_SendCommandFailed;
        }

        for (i = 0U; i < blockCount; i++)
        {
            if (kStatus_Success != SDSPI_Read(host, buffer, card->blockSize))
            {
                return kStatus_SDSPI_ReadFailed;
            }
            buffer += card->blockSize;
        }

        /* Write stop transmission command after the last data block. */
        if (kStatus_Success != SDSPI_StopTransmission(card))
        {
            return kStatus_SDSPI_StopTransmissionFailed;
        }
    }

    return kStatus_Success;
}

status_t SDSPI_WriteBlocks(sdspi_card_t *card, uint8_t *buffer, uint32_t startBlock, uint32_t blockCount)
{
    assert(card);
    assert(card->host);
    assert(buffer);
    assert(blockCount);

    uint32_t offset;
    uint32_t i;
    sdspi_host_t *host;
    sdspi_command_t command = {0};

    if (SDSPI_CheckReadOnly(card))
    {
        return kStatus_SDSPI_WriteProtected;
    }

    offset = startBlock;
    if (!IS_BLOCK_ACCESS(card))
    {
        offset *= card->blockSize;
    }

    /* Send command and writes data. */
    host = card->host;
    if (blockCount == 1U)
    {
        command.index = kSDMMC_WriteSingleBlock;
        command.argument = offset;
        command.responseType = kSDSPI_ResponseTypeR1;
        if (kStatus_Success != SDSPI_SendCommand(host, &command, FSL_SDSPI_TIMEOUT))
        {
            return kStatus_SDSPI_SendCommandFailed;
        }

        if (command.response[0U])
        {
            return kStatus_SDSPI_ResponseError;
        }

        if (kStatus_Success != SDSPI_Write(host, buffer, card->blockSize, kSDSPI_DataTokenSingleBlockWrite))
        {
            return kStatus_SDSPI_WriteFailed;
        }
    }
    else
    {
#if defined FSL_SDSPI_ENABLE_PRE_ERASE_ON_WRITE
        /* Pre-erase before writing data */
        if (kStatus_Success != SDSPI_SendApplicationCmd(card))
        {
            return kStatus_SDSPI_SendApplicationCommandFailed;
        }

        command.index = kSDAppSetWrBlkEraseCount;
        command.argument = blockCount;
        command.responseType = kSDSPI_ResponseTypeR1;
        if (kStatus_Success != SDSPI_SendCommand(host->base, &command, FSL_SDSPI_TIMEOUT))
        {
            return kStatus_SDSPI_SendCommandFailed;
        }

        if (req->response[0U])
        {
            return kStatus_SDSPI_ResponseError;
        }
#endif

        memset(&command, 0U, sizeof(sdspi_command_t));
        command.index = kSDMMC_WriteMultipleBlock;
        command.argument = offset;
        command.responseType = kSDSPI_ResponseTypeR1;
        if (kStatus_Success != SDSPI_SendCommand(host, &command, FSL_SDSPI_TIMEOUT))
        {
            return kStatus_SDSPI_SendCommandFailed;
        }

        if (command.response[0U])
        {
            return kStatus_SDSPI_ResponseError;
        }

        for (i = 0U; i < blockCount; i++)
        {
            if (kStatus_Success != SDSPI_Write(host, buffer, card->blockSize, kSDSPI_DataTokenMultipleBlockWrite))
            {
                return kStatus_SDSPI_WriteFailed;
            }
            buffer += card->blockSize;
        }
        if (kStatus_Success != SDSPI_Write(host, 0U, 0U, kSDSPI_DataTokenStopTransfer))
        {
            return kStatus_SDSPI_WriteFailed;
        }

        /* Wait the card programming end. */
        if (kStatus_Success != SDSPI_WaitReady(host, FSL_SDSPI_TIMEOUT))
        {
            return kStatus_SDSPI_WaitReadyFailed;
        }
    }

    return kStatus_Success;
}

#endif // EXAMPLE
