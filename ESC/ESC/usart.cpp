#include "usart.h"

void USART_init(uint16_t speed, bool enable_Rx_interrupt, bool enable_Tx_interrupt, bool enable_speed_multiplier)
{
	UBRRH = (uint8_t)(speed>>8);
	UBRRL = (uint8_t)(speed);
	if(enable_Rx_interrupt) UCSRB |= (1<<RXCIE); //enable interrupt on Rx
	if(enable_Tx_interrupt) UCSRB |= (1<<TXCIE); //enable interrupt on Tx
	if(enable_speed_multiplier) UCSRA |= (1<<U2X); //enable speed multiplier
	UCSRC = (1<<URSEL)|(1<<USBS)|(1<<UCSZ1)|(1<<UCSZ0); //URSEL for writing to UCSRC; USBS - 1 stop-bit; UCSZ0,1 - 8-bit package
	UCSRB = (1<<RXEN)|(1<<TXEN); //enable Rx and Tx
}

void USART_transmit(uint8_t data)
{
	while (!(UCSRA & (1<<UDRE))); //wait while Tx buffer frees
	UDR = data;
}

void USART_transmit_array(char data[], uint16_t len)
{
	for(int i = 0; i < len; i++)
	{
		USART_transmit(data[i]);
	}
}