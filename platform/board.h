// I/O definitions for the WAV board.

#ifndef _BOARD_H_
#define _BOARD_H_

#include "MKL17Z644.h"

#define CORE_CLOCK   48000000 // CPU core clocked at 48MHz
#define BUS_CLOCK    24000000 // Peripherals clocked at 24MHz
#define SYSTICK_FREQ      100 // Number of systick interrupts per second.

// Convert milliseconds to system ticks.
// No longer neccessary now that SystemTick converts to ms internally.
//#define MS_TO_TICKS(ms) (((ms)*(SYSTICK_FREQ))/1000)


// Definitions for the FlexIO-bases I2S driver.
#define I2S_CLK_TMR_INDEX 0 // Use TIMER0 to generate I2S clock.
#define I2S_CLK_PIN_INDEX 2 // Output I2S clock on PTD2.

#define I2S_WS_TMR_INDEX 1 // Use TIMER1 to generate I2S frame sync signal.
#define I2S_WS_PIN_INDEX 3 // Output frame sync signal on PTD3.

#define I2S_SHIFTER_INDEX 0     // Use SHIFTER0 to output the I2S data.
#define I2S_SHIFTER_PIN_INDEX 0 // Output I2S data on PTD0.

// Options for clock source selection on FLEXIO.
#define FLEXIO_CLK_SRC_DISABLE  0
#define FLEXIO_CLK_SRC_IRC48M   1
#define FLEXIO_CLK_SRC_OSCERCLK 2
#define FLEXIO_CLK_SRC_MCGIRCLK 3

#define FLEXIO_DMA         DMA0
#define FLEXIO_DMA_CHANNEL 0
#define FLEXIO_DMA_IRQN    DMA0_IRQn

// Definitions for the SPI interface.
#define SPI_PERIPH_CLOCK   SystemIntegration::kCLOCK_Spi0  // Peripheral clock to activate.
#define SPI_PORT_CLOCK     SystemIntegration::kCLOCK_PortE // Port clock to activate.
#define SPI_DMA_CHANNEL_TX 1
#define SPI_DMA_CHANNEL_RX 2
#define SPI_PORT           PORTE
#define SPI_CS_PIN_ALT     SystemIntegration::ALT2
#define SPI_CLK_PIN_INDEX  17                      // SPI clock out on pin 6 (PTE17, alt2)
#define SPI_CLK_PIN_ALT    SystemIntegration::ALT2
#define SPI_MOSI_PIN_INDEX 18                      // SPI data out on pin 7 (PTE18, alt2)
#define SPI_MOSI_PIN_ALT   SystemIntegration::ALT2
#define SPI_MISO_PIN_INDEX 19                      // SPI data in on pin 8 (PTE19, alt2)
#define SPI_MISO_PIN_ALT   SystemIntegration::ALT2

// SD Card chip select out on pin 5 (PTE16, alt2)
#define SDCARD_CS_PORT Gpio::portE
#define SDCARD_CS_PIN  16
#define SDCARD_READONLY

#endif
