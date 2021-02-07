/*
 * Spi.cpp
 *
 *  Created on: 6 Feb 2021
 *      Author: adam
 */

// This is a cut-down version of the official driver.

#include "Spi.h"
#include "SystemIntegration.h"
#include "board.h"

enum _spi_clock_polarity
{
    kSPI_ClockPolarityActiveHigh = 0x0U, // Active-high SPI clock (idles low).
    kSPI_ClockPolarityActiveLow          // Active-low SPI clock (idles high).
};

enum _spi_clock_phase
{
    kSPI_ClockPhaseFirstEdge = 0x0U, // First edge on SPSCK occurs at the middle of the first cycle of a data transfer.
    kSPI_ClockPhaseSecondEdge        // First edge on SPSCK occurs at the start of the first cycle of a data transfer.
};

enum _spi_shift_direction
{
    kSPI_MsbFirst = 0x0U, // Data transfers start with most significant bit.
    kSPI_LsbFirst         // Data transfers start with least significant bit.
};

#define SPI_PERIPH   SPI0
#define SPI_POLARITY kSPI_ClockPolarityActiveHigh
#define SPI_PHASE    kSPI_ClockPhaseFirstEdge
#define SPI_BITORDER kSPI_MsbFirst

// The SPI peripheral on the KL17 has a 4-deep FIFO, 16-bit transfers and DMA.
// I'll probably only use the DMA for larger transfers.

Spi::Spi()
	: _dmaTx(SPI_DMA_CHANNEL_TX, Dma::muxSPI0tx)
	, _dmaRx(SPI_DMA_CHANNEL_RX, Dma::muxSPI0rx)
	, _c2(0)
{
	// Set up I/O pins.
	SystemIntegration::enableClock(SPI_PORT_CLOCK);
	SystemIntegration::enableClock(SPI_PERIPH_CLOCK);
	SystemIntegration::setPinAlt(SPI_PORT, SPI_CLK_PIN_INDEX,  SPI_CLK_PIN_ALT);
	SystemIntegration::setPinAlt(SPI_PORT, SPI_MOSI_PIN_INDEX, SPI_MOSI_PIN_ALT);
	SystemIntegration::setPinAlt(SPI_PORT, SPI_MISO_PIN_INDEX, SPI_MISO_PIN_ALT);

	// Disable SPI before configuration
	SPI_PERIPH->C1 &= ~SPI_C1_SPE_MASK;

	// Configure clock polarity and phase, set SPI to master.
    SPI_PERIPH->C1 = SPI_C1_SPE_MASK | SPI_C1_MSTR(1U) | SPI_C1_CPOL(SPI_POLARITY) | SPI_C1_CPHA(SPI_PHASE) | SPI_C1_LSBFE(SPI_BITORDER);
    SPI_PERIPH->C2 = _c2; // Other configuration options.

    // Set the baud rate.
    setFrequency(kDefaultFreq);
}

// Baud rate is calculated from the peripheral clock which should be 24MHz.
// Find combination of prescaler and scaler resulting in baudrate closest to the requested value
void Spi::setFrequency(uint32_t freq)
{
	uint32_t min_diff = 0xFFFFFFFFU;

	// Set the maximum divisor bit settings for each of the following divisors
	uint32_t bestPrescaler = 7U;
	uint32_t bestDivisor = 8U;

	for(uint32_t prescaler = 0; (prescaler <= 7) && min_diff; prescaler++) {
		uint32_t rateDivisorValue = 2U;

		for(uint32_t rateDivisor = 0; (rateDivisor <= 8U) && min_diff; rateDivisor++) {
			// Calculate actual baud rate, note need to add 1 to prescaler
			uint32_t realBaudrate = ((BUS_CLOCK) / ((prescaler + 1) * rateDivisorValue));

			// Calculate the baud rate difference based on the conditional statement ,that states that the
			// calculated baud rate must not exceed the desired baud rate
			if (freq >= realBaudrate)
			{
				uint32_t diff = freq - realBaudrate;
				if (min_diff > diff)
				{
					// A better match found
					min_diff = diff;
					bestPrescaler = prescaler;
					bestDivisor = rateDivisor;
				}
			}

			// Multiply by 2 for each iteration, possible divisor values: 2, 4, 8, 16, ... 512
			rateDivisorValue *= 2U;
		}
	}

	// Write the best prescalar and baud rate scalar
	SPI_PERIPH->BR = SPI_BR_SPR(bestDivisor) | SPI_BR_SPPR(bestPrescaler);
}

// Blocking send using DMA.
void Spi::send(const uint8_t *buffer, unsigned size)
{
	// Start DMA transfer
	//_dma.abort();
	_dmaTx.startTransfer((void *)buffer, (void *)&SPI_PERIPH->DL, size, DMA_MEM_TO_PERIPH | DMA_SRC_8BIT | DMA_DST_8BIT);

	// Put SPI into DMA Transmit mode.
	SPI_PERIPH->C2 = _c2 | SPI_C2_TXDMAE_MASK;

	// Block until transfer is complete.
	while(!_dmaTx.isCompleted())
		;
}

// Blocking receive using DMA.
void Spi::recv(uint8_t *buffer, unsigned size)
{
	// Set up DMA transfer. For SPI you need to send dummy data to generate clock.
	//	_dma.abort();
	uint8_t dummy = 0xFF;
	_dmaTx.startTransfer((void *)&dummy, (void *)&SPI_PERIPH->DL, size, DMA_SRC_8BIT | DMA_DST_8BIT);
	_dmaRx.startTransfer((void *)&SPI_PERIPH->DL, (void *)buffer, size, DMA_PERIPH_TO_MEM | DMA_SRC_8BIT | DMA_DST_8BIT);

//	// Reset the DMA channel.
//	FLEXIO_DMA->DMA[SPI_DMA_CHANNEL].DSR_BCR |= DMA_DSR_BCR_DONE_MASK;
//	FLEXIO_DMA->DMA[SPI_DMA_CHANNEL].DCR = 0;
//
//	// Set source address
//	FLEXIO_DMA->DMA[SPI_DMA_CHANNEL].SAR = (uint32_t)&SPI_PERIPH->DL;
//
//	// Set destination address
//	FLEXIO_DMA->DMA[SPI_DMA_CHANNEL].DAR = (uint32_t)buffer;
//
//	// Set transfer byte count
//	FLEXIO_DMA->DMA[SPI_DMA_CHANNEL].DSR_BCR = DMA_DSR_BCR_BCR(size);
//
//	// Set DMA Control Register
//	FLEXIO_DMA->DMA[SPI_DMA_CHANNEL].DCR = DMA_INC_DST | DMA_SRC_8BIT | DMA_DST_8BIT | DMA_DCR_D_REQ_MASK | DMA_DCR_CS_MASK | DMA_DCR_ERQ_MASK;
//
//	// Start shoveling bytes.
//	//FLEXIO_DMA->DMA[SPI_DMA_CHANNEL].DCR |= DMA_DCR_START_MASK; // DMA_DCR_ERQ_MASK;
//

	// Put SPI into DMA Transmit/Receive mode.
	SPI_PERIPH->C2 = _c2 | SPI_C2_RXDMAE_MASK | SPI_C2_TXDMAE_MASK;
	//SPI_PERIPH->DL = 0xFF;

	// TEST - try a manual start.
//	FLEXIO_DMA->DMA[SPI_DMA_CHANNEL].DCR |= DMA_DCR_START_MASK;

//	// Block until transfer is complete.
	while(!_dmaRx.isCompleted())
		;
}



#ifdef EXAMPLE
    spi_slave_config_t slaveConfig = {0};
    spi_transfer_t xfer = {0};
    uint32_t i = 0U;
    uint32_t err = 0U;

    /* Init source buffer */
    for (i = 0U; i < BUFFER_SIZE; i++)
    {
        srcBuff[i] = i;
    }

    /* SPI slave transfer */
    xfer.rxData = destBuff;
    xfer.dataSize = BUFFER_SIZE;
    xfer.txData = NULL;
    SPI_SlaveTransferNonBlocking(EXAMPLE_SPI_SLAVE, &slaveHandle, &xfer);

    /* SPI master start transfer */
    xfer.txData = srcBuff;
    xfer.rxData = NULL;
    xfer.dataSize = BUFFER_SIZE;
    SPI_MasterTransferBlocking(SPI0, &xfer);

    while (slaveFinished != true)
    {
    }

    /* Check the data received */
    for (i = 0U; i < BUFFER_SIZE; i++)
    {
        if (destBuff[i] != srcBuff[i])
        {
            PRINTF("\r\nThe %d data is wrong, the data received is %d \r\n", i, destBuff[i]);
            err++;
        }
    }
    if (err == 0U)
    {
        PRINTF("\r\nSPI transfer finished!\r\n");
    }

    while (1)
    {
    }




    status_t SPI_MasterTransferBlocking(SPI_Type *base, spi_transfer_t *xfer)
    {
        assert(xfer);

        uint8_t bytesPerFrame = 1U;

        /* Check if the argument is legal */
        if ((xfer->txData == NULL) && (xfer->rxData == NULL))
        {
            return kStatus_InvalidArgument;
        }

    #if defined(FSL_FEATURE_SPI_16BIT_TRANSFERS) && FSL_FEATURE_SPI_16BIT_TRANSFERS
        /* Check if 16 bits or 8 bits */
        bytesPerFrame = ((base->C2 & SPI_C2_SPIMODE_MASK) >> SPI_C2_SPIMODE_SHIFT) + 1U;
    #endif

        /* Disable SPI and then enable it, this is used to clear S register */
        base->C1 &= ~SPI_C1_SPE_MASK;
        base->C1 |= SPI_C1_SPE_MASK;

    #if defined(FSL_FEATURE_SPI_HAS_FIFO) && FSL_FEATURE_SPI_HAS_FIFO

        /* Disable FIFO, as the FIFO may cause data loss if the data size is not integer
           times of 2bytes. As SPI cannot set watermark to 0, only can set to 1/2 FIFO size or 3/4 FIFO
           size. */
        if (FSL_FEATURE_SPI_FIFO_SIZEn(base) != 0)
        {
            base->C3 &= ~SPI_C3_FIFOMODE_MASK;
        }

    #endif /* FSL_FEATURE_SPI_HAS_FIFO */

        /* Begin the polling transfer until all data sent */
        while (xfer->dataSize > 0)
        {
            /* Data send */
            while ((base->S & SPI_S_SPTEF_MASK) == 0U)
            {
            }
            SPI_WriteNonBlocking(base, xfer->txData, bytesPerFrame);
            if (xfer->txData)
            {
                xfer->txData += bytesPerFrame;
            }

            while ((base->S & SPI_S_SPRF_MASK) == 0U)
            {
            }
            SPI_ReadNonBlocking(base, xfer->rxData, bytesPerFrame);
            if (xfer->rxData)
            {
                xfer->rxData += bytesPerFrame;
            }

            /* Decrease the number */
            xfer->dataSize -= bytesPerFrame;
        }

        return kStatus_Success;
    }


#endif // EXAMPLE
