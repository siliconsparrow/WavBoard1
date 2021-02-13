/*
 * Gpio.h
 *
 *  Created on: 7 Feb 2021
 *      Author: adam
 */

#ifndef GPIO_H_
#define GPIO_H_

#include "board.h"

class Gpio
{
public:
	enum GpioDir  { INPUT, OUTPUT };
	enum GpioPort { portA, portB, portC, portD, portE };

	Gpio(GpioPort port);

	void setPinMode(unsigned pin, GpioDir direction);

	inline void set(unsigned mask)    { _gpio->PSOR = mask; }
	inline void clr(unsigned mask)    { _gpio->PCOR = mask; }
	inline void toggle(unsigned mask) { _gpio->PTOR = mask; }
	inline void setPin(unsigned pin)    { _gpio->PSOR = (1 << pin); }
	inline void clrPin(unsigned pin)    { _gpio->PCOR = (1 << pin); }
	inline void togglePin(unsigned pin) { _gpio->PTOR = (1 << pin); }

private:
	GPIO_Type *_gpio;
	PORT_Type *_port;
};

#endif /* GPIO_H_ */
