/*
 * SystemIntegration.cpp
 *
 *  Created on: 7Jan.,2018
 *      Author: adam
 */

#include "SystemIntegration.h"
#include "board.h"

void SystemIntegration::setPinAlt(PORT_Type *port, unsigned pin, SystemIntegration::ALT alt)
{
	port->PCR[pin] |= alt;
}

void SystemIntegration::enableClock(PERIPHCLOCK clk)
{
	uint32_t regAddr = SIM_BASE + CLK_GATE_ABSTRACT_REG_OFFSET((uint32_t)clk);
	(*(volatile uint32_t *)regAddr) |= (1U << CLK_GATE_ABSTRACT_BITS_SHIFT((uint32_t)clk));
}
