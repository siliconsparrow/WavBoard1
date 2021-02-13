/*
 * Spi.h
 *
 *  Created on: 6 Feb 2021
 *      Author: adam
 */

#ifndef SPI_H_
#define SPI_H_

#include "Dma.h"
#include <stdint.h>

class Spi
{
public:

	enum {
		kDefaultFreq = 400000, // default to 400kHz
	};

	Spi();

	void setFrequency(uint32_t hz);

	// Single byte transfer.
	//void    send(uint8_t b) { xfer(b); }
	//uint8_t recv()          { return xfer(0xFF); }

	// Block transfer using DMA.
	//void send(const uint8_t *buffer, unsigned size);
	//void recv(uint8_t *buffer, unsigned size);

	void exchange(const uint8_t *send, uint8_t *recv, unsigned size);

private:
	Dma      _dmaTx;
	Dma      _dmaRx;
	uint32_t _c2;

//	uint8_t xfer(uint8_t b);
	uint8_t exchangeSingle(uint8_t send);
	void exchangeDma(const uint8_t *send, uint8_t *recv, unsigned size);
};

#endif // SPI_H_
