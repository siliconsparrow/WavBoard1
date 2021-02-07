/*
 * SystemIntegration.h
 *
 *  Created on: 7Jan.,2018
 *      Author: adam
 */

#ifndef SYSTEMINTEGRATION_H_
#define SYSTEMINTEGRATION_H_

#include "board.h"

#define CLK_GATE_REG_OFFSET_SHIFT 16U
#define CLK_GATE_REG_OFFSET_MASK 0xFFFF0000U
#define CLK_GATE_BIT_SHIFT_SHIFT 0U
#define CLK_GATE_BIT_SHIFT_MASK 0x0000FFFFU

#define CLK_GATE_ABSTRACT_REG_OFFSET(x) (((x)&CLK_GATE_REG_OFFSET_MASK) >> CLK_GATE_REG_OFFSET_SHIFT)
#define CLK_GATE_ABSTRACT_BITS_SHIFT(x) (((x)&CLK_GATE_BIT_SHIFT_MASK) >> CLK_GATE_BIT_SHIFT_SHIFT)

#define CLK_GATE_DEFINE(reg_offset, bit_shift)                                  \
    ((((reg_offset) << CLK_GATE_REG_OFFSET_SHIFT) & CLK_GATE_REG_OFFSET_MASK) | \
     (((bit_shift) << CLK_GATE_BIT_SHIFT_SHIFT) & CLK_GATE_BIT_SHIFT_MASK))

class SystemIntegration
{
public:
	enum ALT
	{
		ALT0 = (0<<8),
		ALT1 = (1<<8),
		ALT2 = (2<<8),
		ALT3 = (3<<8),
		ALT4 = (4<<8),
		ALT5 = (5<<8),
		ALT6 = (6<<8),
		ALT7 = (7<<8)
	};

	static void setPinAlt(PORT_Type *port, unsigned pin, ALT alt);

	enum PERIPHCLOCK
	{
	    kCLOCK_IpInvalid = 0U,
	    kCLOCK_I2c0 = CLK_GATE_DEFINE(0x1034U, 6U),
	    kCLOCK_I2c1 = CLK_GATE_DEFINE(0x1034U, 7U),
	    kCLOCK_Uart2 = CLK_GATE_DEFINE(0x1034U, 12U),
	    kCLOCK_Cmp0 = CLK_GATE_DEFINE(0x1034U, 19U),
	    kCLOCK_Vref0 = CLK_GATE_DEFINE(0x1034U, 20U),
	    kCLOCK_Spi0 = CLK_GATE_DEFINE(0x1034U, 22U),
	    kCLOCK_Spi1 = CLK_GATE_DEFINE(0x1034U, 23U),

	    kCLOCK_Lptmr0 = CLK_GATE_DEFINE(0x1038U, 0U),
	    kCLOCK_PortA = CLK_GATE_DEFINE(0x1038U, 9U),
	    kCLOCK_PortB = CLK_GATE_DEFINE(0x1038U, 10U),
	    kCLOCK_PortC = CLK_GATE_DEFINE(0x1038U, 11U),
	    kCLOCK_PortD = CLK_GATE_DEFINE(0x1038U, 12U),
	    kCLOCK_PortE = CLK_GATE_DEFINE(0x1038U, 13U),
	    kCLOCK_Lpuart0 = CLK_GATE_DEFINE(0x1038U, 20U),
	    kCLOCK_Lpuart1 = CLK_GATE_DEFINE(0x1038U, 21U),
	    kCLOCK_Flexio0 = CLK_GATE_DEFINE(0x1038U, 31U),

	    kCLOCK_Ftf0 = CLK_GATE_DEFINE(0x103CU, 0U),
	    kCLOCK_Dmamux0 = CLK_GATE_DEFINE(0x103CU, 1U),
	    kCLOCK_Crc0 = CLK_GATE_DEFINE(0x103CU, 18U),
	    kCLOCK_Pit0 = CLK_GATE_DEFINE(0x103CU, 23U),
	    kCLOCK_Tpm0 = CLK_GATE_DEFINE(0x103CU, 24U),
	    kCLOCK_Tpm1 = CLK_GATE_DEFINE(0x103CU, 25U),
	    kCLOCK_Tpm2 = CLK_GATE_DEFINE(0x103CU, 26U),
	    kCLOCK_Adc0 = CLK_GATE_DEFINE(0x103CU, 27U),
	    kCLOCK_Rtc0 = CLK_GATE_DEFINE(0x103CU, 29U),

	    kCLOCK_Dma0 = CLK_GATE_DEFINE(0x1040U, 8U),
	} clock_ip_name_t;

	static void enableClock(PERIPHCLOCK clk);
};

#endif /* SYSTEMINTEGRATION_H_ */
