#define F_CPU 8000000

#include <avr/io.h>
#include <util/delay.h>
#include "io_helper.h"

#define ApFET  5
#define AnFET  2
#define BpFET  4
#define BnFET  1
#define CpFET  3
#define CnFET  0
#define pwm_in 2

#define ApFET_port  PORTD
#define AnFET_port  PORTB
#define BpFET_port  PORTD
#define BnFET_port  PORTB
#define CpFET_port  PORTD
#define CnFET_port  PORTB
#define pwm_in_port PORTD

#define phase_time    20
#define pwm_off_time  10
#define pwm_iteration 20

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
	DDRD = 0b00111100;
	DDRB = 0b00000111;
	PORTD = 0b00000000;
	PORTB = 0b00000000;
    while(true) 
    {
		//doPhaseLoop();
		readInput(&pwm_in_port, pwm_in);
    }
}

