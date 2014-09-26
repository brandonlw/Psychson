#include "defs.h"
#include "timers.h"

static BYTE 	tmr0count, led_ticks, led_timer, led_tick_threshold;
static BYTE 	tmr1count;
static WORD	tmr1reload;

void tmr1isr(void) __interrupt TMR1_VECT
{
	TR1 = 0;
	TH1 = MSB(tmr1reload);
	TL1 = LSB(tmr1reload);
	tmr1count++;
	TR1 = 1;
}

void InitTicks()
{
	if (XVAL(0xFA60) == 0x0F)
	{
		tmr1reload = 0xF63C;
	}
	else
	{
		tmr1reload = 0-(2500/(XVAL(0xFA60)+2));
	}

	tmr1count = 0;
	TR1 = 0;
	ET1 = 1;
	TMOD = TMOD & 0x0F | 0x10;
}

BYTE GetTickCount(void)
{
	return tmr1count;
}

void tmr0isr(void) __interrupt TMR0_VECT
{
	//approx. 10 times per second
	TR0 = 0;
	TL0 = 0xE6;
	TH0 = 0x96;
	TR0 = 1;

	if ((GPIO0OUT & 2) == 0) //turned off
	{
		return;
	}

	tmr0count++;
	led_ticks++;
	if (led_ticks < led_tick_threshold)
	{
		return;
	}

	led_ticks = 0;
	if (led_timer >= 31)
	{
		GPIO0OUT = 1;
		led_timer = 0;		
		return;
	}

	if (led_timer >= 10)
	{
		GPIO0OUT = ~GPIO0OUT;
		led_timer++;
		return;
	}

	if (led_timer == 0)
	{
		return;
	}

	if (GPIO0OUT & 1)
	{
		GPIO0OUT &= 0xFE;
	}
	else
	{
		GPIO0OUT |= 1;
	}
}

void SetLEDThreshold(int threshold)
{
	led_tick_threshold = threshold;
}

void InitLED(void)
{
	led_tick_threshold = 100;
	tmr0count = 0;
	GPIO0OUT = 3;
	led_ticks = 0;
	led_timer = 0;
	EA = 1;
	ET0 = 1;
	TR0 = 1;
}

void LEDBlink(void)
{
	GPIO0OUT = 2;
	led_timer = 1;
}

void LEDOff(void)
{
	GPIO0OUT = 3;
	led_timer = 0;
}
