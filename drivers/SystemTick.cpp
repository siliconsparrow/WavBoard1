/*
 * SystemTick.cpp
 *
 *  Created on: 7 Feb 2021
 *      Author: adam
 */

#include "SystemTick.h"
#include "board.h"

volatile unsigned g_systickCount = 0;

#define SYSTICK_MS_PER_INTERRUPT (1000/SYSTICK_FREQ)

// Configure the SysTick timer.
void SystemTick::init()
{
	SysTick_Config(CORE_CLOCK / SYSTICK_FREQ);
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

extern "C" void SysTick_Handler()
{
	g_systickCount += SYSTICK_MS_PER_INTERRUPT;
}

unsigned SystemTick::getMilliseconds()
{
	return g_systickCount;
}

void SystemTick::delay(unsigned ms)
{
	unsigned tStart = g_systickCount;
	while(g_systickCount - tStart < ms)
		;
}
