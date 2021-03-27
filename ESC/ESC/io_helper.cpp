#include "io_helper.h"

void writeOutput(volatile uint8_t *port, uint8_t pin, bool value)
{
	if(value) *port |= (1<<(pin));
	else *port &= ~(1<<(pin));
}

bool readInput(volatile uint8_t *port, uint8_t pin)
{
	return *port & (1 << pin);
}