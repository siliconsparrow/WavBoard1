/*
 * Dma.cpp
 *
 *  Created on: 6 Feb 2021
 *      Author: adam
 */

#include "Dma.h"
#include "SystemIntegration.h"
#include "board.h"

IRQn_Type DmaIRQn[] = { DMA0_IRQn, DMA1_IRQn, DMA2_IRQn, DMA3_IRQn }; // Map of IRQ numbers for each DMA channel.

// Init the DMA and set the multiplexer for the desired channel.
Dma::Dma(unsigned channel, DmaMuxChannel muxChan)
	: _channel(channel)
{
	SystemIntegration::enableClock(SystemIntegration::kCLOCK_Dmamux0);
	DMAMUX0->CHCFG[_channel] = DMAMUX_CHCFG_ENBL_MASK | muxChan;
}

//void Dma::setMuxChannel(DmaMuxChannel muxChan)
//{
//	DMAMUX0->CHCFG[_channel] = DMAMUX_CHCFG_ENBL_MASK | muxChan;
//}

void Dma::abort()
{
	FLEXIO_DMA->DMA[_channel].DSR_BCR |= DMA_DSR_BCR_DONE_MASK;
	FLEXIO_DMA->DMA[_channel].DCR = 0;
}


void Dma::startTransfer(void *srcAddr, void *destAddr, unsigned transferBytes, uint32_t flags)
{
	// Reset the DMA channel.
	FLEXIO_DMA->DMA[_channel].DSR_BCR |= DMA_DSR_BCR_DONE_MASK;

	// Set source address
	FLEXIO_DMA->DMA[_channel].SAR = (uint32_t)srcAddr;

	// Set destination address
	FLEXIO_DMA->DMA[_channel].DAR = (uint32_t )destAddr;

	// Set transfer byte count
	FLEXIO_DMA->DMA[_channel].DSR_BCR = DMA_DSR_BCR_BCR(transferBytes);

	// Use cycle stealing (DMA transfer during unused clock cycles) if we are sending data to a peripheral.
	//if((flags & (DMA_INC_SRC | DMA_INC_DST)) == DMA_INC_SRC)
		flags |= DMA_DCR_CS_MASK;

	// Set DMA Control Register
	flags |= DMA_DCR_D_REQ_MASK; // Auto-clear the ERQ bit when done
	FLEXIO_DMA->DMA[_channel].DCR = flags;

	// Enable the interrupt handler if required.
	if(0 != (flags & DMA_DCR_EINT_MASK)) {
		NVIC_EnableIRQ(DmaIRQn[_channel]);
	}

	// Start shoveling bytes.
	FLEXIO_DMA->DMA[_channel].DCR |= DMA_DCR_ERQ_MASK; // | DMA_DCR_START_MASK;
}

bool Dma::isCompleted() const
{
	return 0 != (FLEXIO_DMA->DMA[_channel].DSR_BCR & DMA_DSR_BCR_DONE_MASK);
}
