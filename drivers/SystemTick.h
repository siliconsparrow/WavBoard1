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

class Timeout
{
public:
	Timeout(unsigned ticks)
		: _tFinish(SystemTick::count() + ticks)
	{ }

	bool isExpired() const
	{
		unsigned dt = _tFinish - SystemTick::count();
		return 0 != (0x80000000 & dt);
	}

private:
	unsigned _tFinish;
};

#endif /* DRIVERS_SYSTEMTICK_H_ */
