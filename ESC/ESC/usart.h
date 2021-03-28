#ifndef USART
#define USART

#include "global_definitions.h"
#include <avr/io.h>

void USART_init(uint16_t speed, bool enable_Rx_interrupt=true, bool enable_Tx_interrupt=false, bool enable_speed_multiplier=true);
void USART_transmit(uint8_t data);
void USART_transmit_array(char data[], uint16_t len);

#endif