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
	void    send(uint8_t b) { xfer(b); }
	uint8_t recv()          { return xfer(0xFF); }

	// Block transfer using DMA.
	void send(const uint8_t *buffer, unsigned size);
	void recv(uint8_t *buffer, unsigned size);

private:
	Dma      _dmaTx;
	Dma      _dmaRx;
	uint32_t _c2;

	uint8_t xfer(uint8_t b);
};

#ifdef EXAMPLE
// Prototype required for the Kinetis example SDMMC code in fsl_sdspi.c
/*! @brief SDSPI host state. */
typedef struct _sdspi_host
{
    uint32_t busBaudRate; /*!< Bus baud rate */

    status_t (*setFrequency)(uint32_t frequency);                   /*!< Set frequency of SPI */
    status_t (*exchange)(uint8_t *in, uint8_t *out, uint32_t size); /*!< Exchange data over SPI */
    uint32_t (*getCurrentMilliseconds)(void);                       /*!< Get current time in milliseconds */
} sdspi_host_t;
#endif // EXAMPLE

#endif // SPI_H_
