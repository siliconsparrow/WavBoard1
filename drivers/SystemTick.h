/*
 * SystemTick.h
 *
 *  Created on: 7 Feb 2021
 *      Author: adam
 */

#ifndef DRIVERS_SYSTEMTICK_H_
#define DRIVERS_SYSTEMTICK_H_

class SystemTick
{
public:
	static void     init();
	static unsigned count();
	static void     delayTicks(unsigned ticks);
};

#endif /* DRIVERS_SYSTEMTICK_H_ */
