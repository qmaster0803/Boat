#ifndef IO_HELPER
#define IO_HELPER

#include <avr/io.h>

void writeOutput(volatile uint8_t *port, uint8_t pin, bool value);
bool readInput(volatile uint8_t *port, uint8_t pin);

#endif