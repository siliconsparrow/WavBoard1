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
	static unsigned getMilliseconds();
	static void     delay(unsigned ms);
};

class Timeout
{
public:
	Timeout(unsigned ms)
		: _tFinish(SystemTick::getMilliseconds() + ms)
	{ }

	bool isExpired() const
	{
		unsigned dt = _tFinish - SystemTick::getMilliseconds();
		return 0 != (0x80000000 & dt);
	}

private:
	unsigned _tFinish;
};

#endif /* DRIVERS_SYSTEMTICK_H_ */
