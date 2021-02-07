/*
 * SystemTick.cpp
 *
 *  Created on: 7 Feb 2021
 *      Author: adam
 */

#include "SystemTick.h"
#include "board.h"

volatile unsigned g_systickCount = 0;

// Configure the SysTick timer.
void SystemTick::init()
{
	SysTick_Config(CORE_CLOCK / SYSTICK_FREQ);
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

extern "C" void SysTick_Handler()
{
	g_systickCount++;
}

unsigned SystemTick::count()
{
	return g_systickCount;
}

void SystemTick::delayTicks(unsigned ticks)
{
	unsigned tStart = g_systickCount;
	while(g_systickCount - tStart < ticks)
		;
}
