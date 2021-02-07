/*
 * Dma.h
 *
 *  Created on: 6 Feb 2021
 *      Author: adam
 */

#ifndef DMA_H_
#define DMA_H_

#include <stdint.h>

class Dma
{
public:
	enum DmaMuxChannel
	{
		muxNone         =  0,
		muxLPUART0rx    =  2,
		muxLPUART0tx    =  3,
		muxLPUART1rx    =  4,
		muxLPUART1tx    =  5,
		muxUART2rx      =  6,
		muxUART2tx      =  7,
		muxFlexIOch0    = 10,
		muxFlexIOch1    = 11,
		muxFlexIOch2    = 12,
		muxFlexIOch3    = 13,
		muxSPI0rx       = 16,
		muxSPI0tx       = 17,
		muxSPI1rx       = 18,
		muxSPI1tx       = 19,
		muxI2C0         = 22,
		muxI2C1         = 23,
		muxTPM0ch0      = 24,
		muxTPM0ch1      = 25,
		muxTPM0ch2      = 26,
		muxTPM0ch3      = 27,
		muxTPM0ch4      = 28,
		muxTPM0ch5      = 29,
		muxTMP1ch0      = 32,
		muxTMP1ch1      = 33,
		muxTMP2ch0      = 34,
		muxTMP2ch1      = 35,
		muxADC0         = 40,
		muxCMP0         = 42,
		muxPORTA        = 49,
		muxPORTB        = 50,
		muxPORTC        = 51,
		muxPORTD        = 52,
		muxPORTE        = 53,
		muxTPM0overflow = 54,
		muxTPM1overflow = 55,
		muxTPM2overflow = 56,

		// Not sure if there are ever used.
		muxDMAmux0      = 60,
		muxDMAmux1      = 61,
		muxDMAmux2      = 62,
		muxDMAmux3      = 63,
	};

	// DMA Flags
	#define DMA_INT_ENABLE (1<<31)
	#define DMA_INC_SRC    (1<<22)
	#define DMA_SRC_8BIT   (1<<20)
	#define DMA_SRC_16BIT  (2<<20)
	#define DMA_SRC_32BIT  0
	#define DMA_INC_DST    (1<<19)
	#define DMA_DST_8BIT   (1<<17)
	#define DMA_DST_16BIT  (2<<17)
	#define DMA_DST_32BIT  0

	#define DMA_MEM_TO_PERIPH (DMA_INC_SRC)
	#define DMA_PERIPH_TO_MEM (DMA_INC_DST)
	#define DMA_MEM_TO_MEM    (DMA_INC_SRC | DMA_INC_DST)

	Dma(unsigned channel, DmaMuxChannel muxChan = muxNone);

	void setMuxChannel(DmaMuxChannel muxChan);
	void abort();
	void startTransfer(void *srcAddr, void *destAddr, unsigned transferBytes, uint32_t flags);
	bool isCompleted() const;

private:
	unsigned _channel;
};

#endif /* DMA_H_ */
