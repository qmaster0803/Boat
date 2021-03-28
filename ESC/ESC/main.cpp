#include "global_definitions.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include "io_helper.h"
#include "usart.h"

#define phase_time    30
#define pwm_off_time  15
#define pwm_iteration 20

bool stopped = true;
bool isInputOn = false;
uint16_t timer_value = 0;
int doSend = 0;

void doPhaseLoop()
{
	//step 1
	for(int i = 0; i < pwm_iteration; i++)
	{
		PORTD = 0b00000000;
		PORTB = 0b00000000;
		_delay_us(pwm_off_time);
		writeOutput(&ApFET_port, ApFET, true);
		writeOutput(&BnFET_port, BnFET, true);
		_delay_us(phase_time);
	}
	//step 2
	for(int i = 0; i < pwm_iteration; i++)
	{
		PORTD = 0b00000000;
		PORTB = 0b00000000;
		_delay_us(pwm_off_time);
		writeOutput(&CpFET_port, CpFET, true);
		writeOutput(&BnFET_port, BnFET, true);
		_delay_us(phase_time);
	}
	//step 3
	for(int i = 0; i < pwm_iteration; i++)
	{
		PORTD = 0b00000000;
		PORTB = 0b00000000;
		_delay_us(pwm_off_time);
		writeOutput(&CpFET_port, CpFET, true);
		writeOutput(&AnFET_port, AnFET, true);
		_delay_us(phase_time);
	}
	//step 4
	for(int i = 0; i < pwm_iteration; i++)
	{
		PORTD = 0b00000000;
		PORTB = 0b00000000;
		_delay_us(pwm_off_time);
		writeOutput(&BpFET_port, BpFET, true);
		writeOutput(&AnFET_port, AnFET, true);
		_delay_us(phase_time);
	}
	//step 5
	for(int i = 0; i < pwm_iteration; i++)
	{
		PORTD = 0b00000000;
		PORTB = 0b00000000;
		_delay_us(pwm_off_time);
		writeOutput(&BpFET_port, BpFET, true);
		writeOutput(&CnFET_port, CnFET, true);
		_delay_us(phase_time);
	}
	//step 6
	for(int i = 0; i < pwm_iteration; i++)
	{
		PORTD = 0b00000000;
		PORTB = 0b00000000;
		_delay_us(pwm_off_time);
		writeOutput(&ApFET_port, ApFET, true);
		writeOutput(&CnFET_port, CnFET, true);
		_delay_us(phase_time);
	}
}

int main(void)
{
	USART_init(8); //115200 baud

	DDRD = 0b00111000;
	DDRB = 0b00000111;
	PORTD = 0b00000000;
	PORTB = 0b00000000;
	
	//enabling INT0 interrupt (rising mode)
	//MCUCR |= (1<<ISC00); 
	//GICR |= (1<<INT0);
	//enable Timer1 overflow interrupt (used to stop motor)
	TIMSK |= (1<<TOIE1);
	//sei();
	
    while(true) 
    {
		doPhaseLoop();
		//readInput(&pwm_in_port, pwm_in);
    }
}

ISR(INT0_vect)
{
	if(isInputOn) //back
	{
		timer_value = TCNT1;
		doSend++;
		if(false)
		{
			char buffer[8] = {0};
			itoa(timer_value, buffer, 10);
			USART_transmit_array(buffer, 8);
			USART_transmit(0x0d);
			USART_transmit(0x0a);
			doSend = 0;
		}
		isInputOn = false;
	}
	else //front
	{
		//reset Timer0
		TCCR1A = 0b00000000;
		TCCR1B = 0b00000000;
		TCNT1H = 0x00;
		TCNT1L = 0x00;
		//start timer0
		TCCR1B = 0b00000010;
		isInputOn = true;
	}
}

ISR(TIMER0_OVF_vect) //debug
{
	USART_transmit('O');
	stopped = true;
}