/*
 * Gpio.cpp
 *
 *  Created on: 7 Feb 2021
 *      Author: adam
 */

#include "Gpio.h"
#include "SystemIntegration.h"

static SystemIntegration::PERIPHCLOCK GpioPeriphClock[] = {
	SystemIntegration::kCLOCK_PortA,
	SystemIntegration::kCLOCK_PortB,
	SystemIntegration::kCLOCK_PortC,
	SystemIntegration::kCLOCK_PortD,
	SystemIntegration::kCLOCK_PortE
};

Gpio::Gpio(GpioPort port)
{
	// Enable the port.
	SystemIntegration::enableClock(GpioPeriphClock[port]);

	// Get pointers to the peripheral.
	_gpio = (GPIO_Type *)(((uint8_t *)GPIOA) + (port * 0x40));
	_port = (PORT_Type *)(((uint8_t *)PORTA) + (port * 0x1000));
}

// Set pin data direction.
void Gpio::setPin(unsigned pinNum, Gpio::GpioDir direction)
{
	// Set the pin mode to ALT1.
	SystemIntegration::setPinAlt(_port, pinNum, SystemIntegration::ALT1);

	// Set the direction.
	unsigned mask = (1 << pinNum);
	switch(direction)
	{
	case INPUT:
		_gpio->PDDR &= ~mask;
		break;

	case OUTPUT:
		_gpio->PDDR |= mask;
		break;
	}
}
