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

class SDCard
{
public:
	enum {
		kBlockSize = 512, // SD Card standard block size.
	};

	enum CardType
	{
		cardtypeNone = 0,
		cardtypeMMC  = 1,
		cardtypeSDv1 = 2,
		cardtypeSDv2 = 4,
		cardtypeSdc  = 6,
	};

	SDCard();

private:
	Spi  _spi;
	Gpio _csPort;
};

#ifdef OLD

// Speed tokens for SPI.
#define SPI_SPEED_DEFAULT     SPI_BaudRatePrescaler_2   // 18MHz default SPI clock speed.
#define SPI_SPEED_SDCARD_MAX  SPI_BaudRatePrescaler_4   // 9MHz SPI clock speed for SD card.
#define SPI_SPEED_SDCARD_SLOW SPI_BaudRatePrescaler_256 // 140kHz SPI clock for SD card init stage.
#define SPI_SPEED_WIZCHIP     SPI_BaudRatePrescaler_2   // 18MHz SPI clock for Ethernet.

struct SpiInitParams
{
	SPI_TypeDef         *spiPort;
	void               (*rccFunc)(uint32_t, FunctionalState);
	uint32_t             rccPeriph;
	uint16_t             CPOL;          // SPI clock polarity: 0=idle low, 1=idle high.
	uint16_t             CPHA;          // SPI clock phase: 0 = sample on leading edge, 1 = sample on trailing edge of clock.
	uint16_t             BaudPrescaler; // System clock is divided by 2^BaudPrescaler
	struct GpioParams    pinClk;
	struct GpioParams    pinMOSI;
	struct GpioParams    pinMISO;
	GPIOSpeed_TypeDef    pinSpeed;

	DMA_TypeDef         *dmaDevice;    // Which DMA device to use for this SPI port.
	void               (*dmaRccFunc)(uint32_t, FunctionalState);
	uint32_t             dmaRccPeriph; //
	DMA_Channel_TypeDef *dmaChannelTx; // DMA channel for sending data.
	DMA_Channel_TypeDef *dmaChannelRx; // DMA channel for receiving data.
	uint32_t             dmaFlagTx;    // Transfer complete flag for DMA sending.
	uint32_t             dmaFlagRx;    // Transfer complete flag for DMA receiving.
};

const struct SpiInitParams SPI1CONF = {
	  SPI1,
	  RCC_APB2PeriphClockCmd,
	  RCC_APB2Periph_SPI1,
	  SPI_CPOL_Low,
	  SPI_CPHA_1Edge,
	  SPI_SPEED_SDCARD_SLOW,
	  { RCC_APB2PeriphClockCmd, RCC_APB2Periph_GPIOA, GPIOA, GPIO_Pin_5 },
	  { RCC_APB2PeriphClockCmd, RCC_APB2Periph_GPIOA, GPIOA, GPIO_Pin_7 },
	  { RCC_APB2PeriphClockCmd, RCC_APB2Periph_GPIOA, GPIOA, GPIO_Pin_6 },
	  GPIO_Speed_10MHz,

	  // DMA details.
	  DMA1,
	  RCC_AHBPeriphClockCmd,
	  RCC_AHBPeriph_DMA1,
	  DMA1_Channel3, DMA1_Channel2,
	  DMA1_FLAG_TC3, DMA1_FLAG_TC2
	};


#include <inttypes.h>
#include "SPI.h"

#define SD_BLOCK_SIZE 512

// Supported SD card types
#define SDCARD_NONE  0x00
#define SDCARD_MMC   0x01
#define SDCARD_SD1   0x02 // v1 interface - SDSC or MMC cards
#define SDCARD_SD2   0x04 // v2 interface - SDHC cards
#define SDCARD_SDC   (CT_SD1|CT_SD2)
#define SDCARD_BLOCK 0x08 // Block address mode for larger cards.

class SDCard
{
public:
	static SDCard *getInstance();
	SDCard()
		: _spi(0)
		, _cardType(SDCARD_NONE)
	{ }

	bool initCard();
	bool readSector(unsigned sector, uint8_t *buffer);
	bool writeSector(unsigned sector, const uint8_t *buffer);

private:
	SPI      *_spi;
	unsigned  _cardType;

	void     initHW();
	void     cardSelect();
	void     cardDeselect();
	bool     waitReady();
	bool     isHighCapacity() const;
	uint8_t  command(uint8_t cmd, uint32_t arg);
	uint8_t  appCommand(uint8_t cmd, uint32_t arg);
	unsigned initCardV1();
	unsigned initCardV2();
	unsigned startSequence();
	bool     isCardOK();
};

#endif // OLD

#endif // SDCARD_H_
