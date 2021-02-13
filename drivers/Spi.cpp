/*
 * Spi.cpp
 *
 *  Created on: 6 Feb 2021
 *      Author: adam
 */

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

#ifdef OLD
// Send and receive a single byte (blocking).
uint8_t Spi::xfer(uint8_t b)
{
	// Wait for port to be ready.
	while(0 == (SPI_PERIPH->S & SPI_S_SPTEF_MASK))
		;

	SPI_PERIPH->DL = b;

	// Block until transfer complete.
	while(0 == (SPI_PERIPH->S & SPI_S_SPRF_MASK))
		;

	return SPI_PERIPH->DL;
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

	// Disable DMA.
	SPI_PERIPH->C2 = _c2;
}

// Blocking receive using DMA.
void Spi::recv(uint8_t *buffer, unsigned size)
{
	// Set up DMA transfer. For SPI you need to send dummy data to generate clock.
	//	_dma.abort();
	uint8_t dummy = 0;
	_dmaTx.startTransfer((void *)&dummy, (void *)&SPI_PERIPH->DL, size, DMA_SRC_8BIT | DMA_DST_8BIT);
	_dmaRx.startTransfer((void *)&SPI_PERIPH->DL, (void *)buffer, size, DMA_PERIPH_TO_MEM | DMA_SRC_8BIT | DMA_DST_8BIT);

	// Put SPI into DMA Transmit/Receive mode.
	SPI_PERIPH->C2 = _c2 | SPI_C2_RXDMAE_MASK | SPI_C2_TXDMAE_MASK;

//	// Block until transfer is complete.
	while(!_dmaRx.isCompleted())
		;

	// Disable DMA.
	SPI_PERIPH->C2 = _c2;
}
#endif // OLD

void Spi::exchange(const uint8_t *send, uint8_t *recv, unsigned size)
{
	// Wait for port to be ready.
	while(0 == (SPI_PERIPH->S & SPI_S_SPTEF_MASK))
		;

	for(unsigned i = 0; i < size; i++) {
		SPI_PERIPH->DL = send[i];
		while(0 == (SPI_PERIPH->S & SPI_S_SPRF_MASK))
			;
		recv[i] = SPI_PERIPH->DL;
	}

	/*

	if(size == 1)
		*recv = exchangeSingle(*send);
	else
		exchangeDma(send, recv, size);
*/
}

uint8_t Spi::exchangeSingle(uint8_t send)
{
	// Wait for port to be ready.
	while(0 == (SPI_PERIPH->S & SPI_S_SPTEF_MASK))
		;

	SPI_PERIPH->DL = send;

	// Block until transfer complete.
	while(0 == (SPI_PERIPH->S & SPI_S_SPRF_MASK))
		;

	return SPI_PERIPH->DL;
}

void Spi::exchangeDma(const uint8_t *send, uint8_t *recv, unsigned size)
{
	// Set up DMA transfer. For SPI you need to send dummy data to generate clock.
	//	_dma.abort();
	_dmaTx.startTransfer((void *)send, (void *)&SPI_PERIPH->DL, size, DMA_MEM_TO_PERIPH | DMA_SRC_8BIT | DMA_DST_8BIT);
	_dmaRx.startTransfer((void *)&SPI_PERIPH->DL, (void *)recv, size, DMA_PERIPH_TO_MEM | DMA_SRC_8BIT | DMA_DST_8BIT);

	// Put SPI into DMA Transmit/Receive mode.
	SPI_PERIPH->C2 = _c2 | SPI_C2_RXDMAE_MASK | SPI_C2_TXDMAE_MASK;

	// Block until transfer is complete.
	while(!_dmaRx.isCompleted())
		;

	// Disable DMA.
	SPI_PERIPH->C2 = _c2;
}
