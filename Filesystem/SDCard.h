// *************************************************************************
// **
// ** SD Card driver for the Kinetis KL17 using SPI
// **
// **   by Adam Pierce <adam@siliconsparrow.com>
// **   created 5-Feb-2021
// **
// *************************************************************************

// SD Card details here: http://www.sdcard.org/developers/tech/sdcard/pls/Simplified_Physical_Layer_Spec.pdf

#ifndef SDCARD_H_
#define SDCARD_H_

#include "Spi.h"
#include "Gpio.h"

//#define SDCARD_KINETIS_DRIVER

#ifdef SDCARD_KINETIS_DRIVER

#define SD_PRODUCT_NAME_BYTES (5U)

class SDCard
{
public:
	enum {
		kBlockSize = 512, // SD Card standard block size.
	};

	static SDCard *instance();

	SDCard();

	bool     init();
	bool     getStatus();
	bool     readBlocks(uint8_t *buffer, unsigned startBlock, unsigned blockCount);
	uint32_t getBlockCount() const { return _blockCount; }
	uint32_t getBlockSize() const { return _blockSize; }
	uint32_t getEraseSectorSize() const { return _csd.eraseSectorSize; }

private:
	/*! @brief SD/MMC card common commands */
	typedef enum _sdmmc_command
	{
	    kSDMMC_GoIdleState = 0U,         /*!< Go Idle State */
	    kSDMMC_AllSendCid = 2U,          /*!< All Send CID */
	    kSDMMC_SetDsr = 4U,              /*!< Set DSR */
	    kSDMMC_SelectCard = 7U,          /*!< Select Card */
	    kSDMMC_SendCsd = 9U,             /*!< Send CSD */
	    kSDMMC_SendCid = 10U,            /*!< Send CID */
	    kSDMMC_StopTransmission = 12U,   /*!< Stop Transmission */
	    kSDMMC_SendStatus = 13U,         /*!< Send Status */
	    kSDMMC_GoInactiveState = 15U,    /*!< Go Inactive State */
	    kSDMMC_SetBlockLength = 16U,     /*!< Set Block Length */
	    kSDMMC_ReadSingleBlock = 17U,    /*!< Read Single Block */
	    kSDMMC_ReadMultipleBlock = 18U,  /*!< Read Multiple Block */
	    kSDMMC_SetBlockCount = 23U,      /*!< Set Block Count */
	    kSDMMC_WriteSingleBlock = 24U,   /*!< Write Single Block */
	    kSDMMC_WriteMultipleBlock = 25U, /*!< Write Multiple Block */
	    kSDMMC_ProgramCsd = 27U,         /*!< Program CSD */
	    kSDMMC_SetWriteProtect = 28U,    /*!< Set Write Protect */
	    kSDMMC_ClearWriteProtect = 29U,  /*!< Clear Write Protect */
	    kSDMMC_SendWriteProtect = 30U,   /*!< Send Write Protect */
	    kSDMMC_Erase = 38U,              /*!< Erase */
	    kSDMMC_LockUnlock = 42U,         /*!< Lock Unlock */
	    kSDMMC_ApplicationCommand = 55U, /*!< Send Application Command */
	    kSDMMC_GeneralCommand = 56U,     /*!< General Purpose Command */
	    kSDMMC_ReadOcr = 58U,            /*!< Read OCR */
	} sdmmc_command_t;

	/*! @brief SDSPI response type */
	typedef enum _sdspi_response_type
	{
	    kSDSPI_ResponseTypeR1 = 0U,  /*!< Response 1 */
	    kSDSPI_ResponseTypeR1b = 1U, /*!< Response 1 with busy */
	    kSDSPI_ResponseTypeR2 = 2U,  /*!< Response 2 */
	    kSDSPI_ResponseTypeR3 = 3U,  /*!< Response 3 */
	    kSDSPI_ResponseTypeR7 = 4U,  /*!< Response 7 */
	} sdspi_response_type_t;

	/*! @brief SDSPI command */
	typedef struct _sdspi_command
	{
	    uint8_t index;        /*!< Command index */
	    uint32_t argument;    /*!< Command argument */
	    uint8_t responseType; /*!< Response type */
	    uint8_t response[5U]; /*!< Response content */
	} sdspi_command_t;

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

	/*! @brief SD card CSD register */
	typedef struct _sd_csd
	{
	    uint8_t csdStructure;        /*!< CSD structure [127:126] */
	    uint8_t dataReadAccessTime1; /*!< Data read access-time-1 [119:112] */
	    uint8_t dataReadAccessTime2; /*!< Data read access-time-2 in clock cycles (NSAC*100) [111:104] */
	    uint8_t transferSpeed;       /*!< Maximum data transfer rate [103:96] */
	    uint16_t cardCommandClass;   /*!< Card command classes [95:84] */
	    uint8_t readBlockLength;     /*!< Maximum read data block length [83:80] */
	    uint16_t flags;              /*!< Flags in _sd_csd_flag */
	    uint32_t deviceSize;         /*!< Device size [73:62] */
	    /* Following fields from 'readCurrentVddMin' to 'deviceSizeMultiplier' exist in CSD version 1 */
	    uint8_t readCurrentVddMin;    /*!< Maximum read current at VDD min [61:59] */
	    uint8_t readCurrentVddMax;    /*!< Maximum read current at VDD max [58:56] */
	    uint8_t writeCurrentVddMin;   /*!< Maximum write current at VDD min [55:53] */
	    uint8_t writeCurrentVddMax;   /*!< Maximum write current at VDD max [52:50] */
	    uint8_t deviceSizeMultiplier; /*!< Device size multiplier [49:47] */

	    uint8_t eraseSectorSize;       /*!< Erase sector size [45:39] */
	    uint8_t writeProtectGroupSize; /*!< Write protect group size [38:32] */
	    uint8_t writeSpeedFactor;      /*!< Write speed factor [28:26] */
	    uint8_t writeBlockLength;      /*!< Maximum write data block length [25:22] */
	    uint8_t fileFormat;            /*!< File format [11:10] */
	} sd_csd_t;

	typedef struct _sd_scr
	{
	    uint8_t scrStructure;             /*!< SCR Structure [63:60] */
	    uint8_t sdSpecification;          /*!< SD memory card specification version [59:56] */
	    uint16_t flags;                   /*!< SCR flags in _sd_scr_flag */
	    uint8_t sdSecurity;               /*!< Security specification supported [54:52] */
	    uint8_t sdBusWidths;              /*!< Data bus widths supported [51:48] */
	    uint8_t extendedSecurity;         /*!< Extended security support [46:43] */
	    uint8_t commandSupport;           /*!< Command support bits [33:32] 33-support CMD23, 32-support cmd20*/
	    uint32_t reservedForManufacturer; /*!< reserved for manufacturer usage [31:0] */
	} sd_scr_t;

	enum _sd_scr_flag
	{
	    kSD_ScrDataStatusAfterErase = (1U << 0U), /*!< Data status after erases [55:55] */
	    kSD_ScrSdSpecification3 = (1U << 1U),     /*!< Specification version 3.00 or higher [47:47]*/
	};


	Spi      _spi;
	Gpio     _csPort; // GPIO port for the chip select.
	uint32_t _ocr;
	uint32_t _flags;
	sd_cid_t _cid;
	sd_csd_t _csd;
    sd_scr_t _scr;
	uint32_t _blockSize;
	uint32_t _blockCount;
    uint8_t  _rawCid[16U];      /*!< Raw CID content */
	uint8_t  _rawCsd[16U];      /*!< Raw CSD content */
    uint8_t  _rawScr[8U];       /*!< Raw SCR content */

	void     select();
	void     deselect();
	bool     goIdle();
	bool     sendCommand(sdspi_command_t *command, uint32_t timeout);
	bool     sendApplicationCmd();
	bool     waitReady(unsigned milliseconds);
	uint32_t generateCRC7(uint8_t *buffer, uint32_t length, uint32_t crc) const;
	bool     sendInterfaceCondition(uint8_t pattern, uint8_t *response);
	bool     applicationSendOperationCondition(uint32_t argument, uint8_t *response);
	bool     readOcr();
	bool     setBlockSize(uint32_t blockSize);
	bool     sendCsd();
	bool     setMaxFrequencyNormalMode();
	void     checkCapacity();
	bool     checkReadOnly();
	bool     sendCid();
	bool     sendScr();
	bool     read(uint8_t *buffer, uint32_t size);
	void     decodeCsd(uint8_t *rawCsd);
	void     decodeCid(uint8_t *rawCid);
	void     decodeScr(uint8_t *rawScr);
	bool     stopTransmission();
};

#else // SDCARD_KINETIS_DRIVER

	class SDCard
	{
	public:
		enum {
			kBlockSize = 512, // SD Card standard block size.
		};

		static SDCard *instance();

		SDCard();

		bool     init();
		bool     getStatus();
		bool     readBlocks(uint8_t *buffer, unsigned startBlock, unsigned blockCount);
		unsigned getBlockCount()      const;
		unsigned getBlockSize()       const;
		unsigned getEraseSectorSize() const;

	private:
		Spi      _spi;
		Gpio     _csPort; // GPIO port for the chip select.
		unsigned _cardType;

		void     select();
		void     deselect();
		bool     readSector(unsigned sector, uint8_t *buffer);
		bool     writeSector(unsigned sector, const uint8_t *buffer);
		bool     isHighCapacity() const;
		uint8_t  command(uint8_t cmd, uint32_t arg);
		uint8_t  appCommand(uint8_t cmd, uint32_t arg);
		bool     waitReady();
		unsigned startSequence();
		unsigned initCardV1();
		unsigned initCardV2();
		uint32_t crc7(uint8_t *buffer, uint32_t length) const;
	};

#endif // SDCARD_KINETIS_DRIVER





#ifdef EXAMPLE

#include "fsl_common.h"
#include "fsl_specification.h"

#define FSL_SDSPI_DEFAULT_BLOCK_SIZE (512U)

/*! @brief SDSPI API status */
enum _sdspi_status
{
    kStatus_SDSPI_SetFrequencyFailed = MAKE_STATUS(kStatusGroup_SDSPI, 0U), /*!< Set frequency failed */
    kStatus_SDSPI_ExchangeFailed = MAKE_STATUS(kStatusGroup_SDSPI, 1U),     /*!< Exchange data on SPI bus failed */
    kStatus_SDSPI_WaitReadyFailed = MAKE_STATUS(kStatusGroup_SDSPI, 2U),    /*!< Wait card ready failed */
    kStatus_SDSPI_ResponseError = MAKE_STATUS(kStatusGroup_SDSPI, 3U),      /*!< Response is error */
    kStatus_SDSPI_WriteProtected = MAKE_STATUS(kStatusGroup_SDSPI, 4U),     /*!< Write protected */
    kStatus_SDSPI_GoIdleFailed = MAKE_STATUS(kStatusGroup_SDSPI, 5U),       /*!< Go idle failed */
    kStatus_SDSPI_SendCommandFailed = MAKE_STATUS(kStatusGroup_SDSPI, 6U),  /*!< Send command failed */
    kStatus_SDSPI_ReadFailed = MAKE_STATUS(kStatusGroup_SDSPI, 7U),         /*!< Read data failed */
    kStatus_SDSPI_WriteFailed = MAKE_STATUS(kStatusGroup_SDSPI, 8U),        /*!< Write data failed */
    kStatus_SDSPI_SendInterfaceConditionFailed =
        MAKE_STATUS(kStatusGroup_SDSPI, 9U), /*!< Send interface condition failed */
    kStatus_SDSPI_SendOperationConditionFailed =
        MAKE_STATUS(kStatusGroup_SDSPI, 10U),                                    /*!< Send operation condition failed */
    kStatus_SDSPI_ReadOcrFailed = MAKE_STATUS(kStatusGroup_SDSPI, 11U),          /*!< Read OCR failed */
    kStatus_SDSPI_SetBlockSizeFailed = MAKE_STATUS(kStatusGroup_SDSPI, 12U),     /*!< Set block size failed */
    kStatus_SDSPI_SendCsdFailed = MAKE_STATUS(kStatusGroup_SDSPI, 13U),          /*!< Send CSD failed */
    kStatus_SDSPI_SendCidFailed = MAKE_STATUS(kStatusGroup_SDSPI, 14U),          /*!< Send CID failed */
    kStatus_SDSPI_StopTransmissionFailed = MAKE_STATUS(kStatusGroup_SDSPI, 15U), /*!< Stop transmission failed */
    kStatus_SDSPI_SendApplicationCommandFailed =
        MAKE_STATUS(kStatusGroup_SDSPI, 16U), /*!< Send application command failed */
};

/*! @brief SDSPI host state. */
typedef struct _sdspi_host
{
    uint32_t busBaudRate; /*!< Bus baud rate */

    status_t (*setFrequency)(uint32_t frequency);                   /*!< Set frequency of SPI */
    status_t (*exchange)(uint8_t *in, uint8_t *out, uint32_t size); /*!< Exchange data over SPI */
    uint32_t (*getCurrentMilliseconds)(void);                       /*!< Get current time in milliseconds */
} sdspi_host_t;

/*!
 * @brief SD Card Structure
 *
 * Define the card structure including the necessary fields to identify and describe the card.
 */
typedef struct _sdspi_card
{
    sdspi_host_t *host;       /*!< Host state information */
    uint32_t relativeAddress; /*!< Relative address of the card */
    uint32_t flags;           /*!< Flags defined in _sdspi_card_flag. */
    uint8_t rawCid[16U];      /*!< Raw CID content */
    uint8_t rawCsd[16U];      /*!< Raw CSD content */
    uint8_t rawScr[8U];       /*!< Raw SCR content */
    uint32_t ocr;             /*!< Raw OCR content */
    sd_cid_t cid;             /*!< CID */
    sd_csd_t csd;             /*!< CSD */
    sd_scr_t scr;             /*!< SCR */
    uint32_t blockCount;      /*!< Card total block number */
    uint32_t blockSize;       /*!< Card block size */
} sdspi_card_t;

/*************************************************************************************************
 * API

/*!
 * @brief Initializes the card on a specific SPI instance.
 *
 * This function initializes the card on a specific SPI instance.
 *
 * @param card Card descriptor
 * @retval kStatus_SDSPI_SetFrequencyFailed Set frequency failed.
 * @retval kStatus_SDSPI_GoIdleFailed Go idle failed.
 * @retval kStatus_SDSPI_SendInterfaceConditionFailed Send interface condition failed.
 * @retval kStatus_SDSPI_SendOperationConditionFailed Send operation condition failed.
 * @retval kStatus_Timeout Send command timeout.
 * @retval kStatus_SDSPI_NotSupportYet Not support yet.
 * @retval kStatus_SDSPI_ReadOcrFailed Read OCR failed.
 * @retval kStatus_SDSPI_SetBlockSizeFailed Set block size failed.
 * @retval kStatus_SDSPI_SendCsdFailed Send CSD failed.
 * @retval kStatus_SDSPI_SendCidFailed Send CID failed.
 * @retval kStatus_Success Operate successfully.
 */
status_t SDSPI_Init(sdspi_card_t *card);

/*!
 * @brief Deinitializes the card.
 *
 * This function deinitializes the specific card.
 *
 * @param card Card descriptor
 */
void SDSPI_Deinit(sdspi_card_t *card);

/*!
 * @brief Checks whether the card is write-protected.
 *
 * This function checks if the card is write-protected via CSD register.
 *
 * @param card Card descriptor.
 * @retval true Card is read only.
 * @retval false Card isn't read only.
 */
bool SDSPI_CheckReadOnly(sdspi_card_t *card);

/*!
 * @brief Reads blocks from the specific card.
 *
 * This function reads blocks from specific card.
 *
 * @param card Card descriptor.
 * @param buffer the buffer to hold the data read from card
 * @param startBlock the start block index
 * @param blockCount the number of blocks to read
 * @retval kStatus_SDSPI_SendCommandFailed Send command failed.
 * @retval kStatus_SDSPI_ReadFailed Read data failed.
 * @retval kStatus_SDSPI_StopTransmissionFailed Stop transmission failed.
 * @retval kStatus_Success Operate successfully.
 */
status_t SDSPI_ReadBlocks(sdspi_card_t *card, uint8_t *buffer, uint32_t startBlock, uint32_t blockCount);

/*!
 * @brief Writes blocks of data to the specific card.
 *
 * This function writes blocks to specific card
 *
 * @param card Card descriptor.
 * @param buffer the buffer holding the data to be written to the card
 * @param startBlock the start block index
 * @param blockCount the number of blocks to write
 * @retval kStatus_SDSPI_WriteProtected Card is write protected.
 * @retval kStatus_SDSPI_SendCommandFailed Send command failed.
 * @retval kStatus_SDSPI_ResponseError Response is error.
 * @retval kStatus_SDSPI_WriteFailed Write data failed.
 * @retval kStatus_SDSPI_ExchangeFailed Exchange data over SPI failed.
 * @retval kStatus_SDSPI_WaitReadyFailed Wait card to be ready status failed.
 * @retval kStatus_Success Operate successfully.
 */
status_t SDSPI_WriteBlocks(sdspi_card_t *card, uint8_t *buffer, uint32_t startBlock, uint32_t blockCount);

#endif // EXAMPLE

#endif // SDCARD_H_
