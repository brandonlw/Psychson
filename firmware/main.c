#include "defs.h"
#include "timers.h"
#include "usb.h"

extern void usb_isr(void) __interrupt USB_VECT;
extern void ep_isr(void) __interrupt EP_VECT;
extern void tmr0isr(void) __interrupt TMR0_VECT;
extern void tmr1isr(void) __interrupt TMR1_VECT;

#define KEY_DELAY 8192
#define KEY_BUFFER_SIZE 0x2000
static const BYTE keyData[KEY_BUFFER_SIZE] = { /*0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
	0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xC3,
	0x15, 0x08, 0x00, 0xFF, 0x00, 0xF5, 0x11, 0x00, 0x12, 0x00, 0x17, 0x00, 0x08, 0x00, 0x13,
	0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0xFF, 0x00, 0xF5, 0x28, 0x00, 0x00, 0xFF, 0x00, 0xFF,
	0x00, 0xF0, 0x0B, 0x02, 0x08, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x12, 0x00, 0x2C, 0x00, 0x1A,
	0x02, 0x12, 0x00, 0x15, 0x00, 0x0F, 0x00, 0x07, 0x00, 0x1E, 0x02, 0x1E, 0x02, 0x1E, 0x02,
	0x28, 0x00*/ 0x12, 0x34, 0x56, 0x78 };
int key_index = 0;
volatile BYTE send_keys_enabled = 0;
DWORD wait_counter = KEY_DELAY;
DWORD wait_tick;

void InitHardware()
{
	//Set up RAM mapping just beyond our own code
	BANK0PAL = BANK0_PA>>9;
	BANK0PAH = BANK0_PA>>17;
	BANK1VA  = BANK1_VA>>8;
	BANK1PAL = BANK1_PA>>9;
	BANK1PAH = BANK1_PA>>17;
	BANK2VA  = BANK2_VA>>8;
	BANK2PAL = BANK2_PA>>9;
	BANK2PAH = BANK2_PA>>17;

	XVAL(0xF809) = 7;
	XVAL(0xF80A) = 0x1F;
	XVAL(0xF810) = 0x60;
	XVAL(0xF811) = 0;
	XVAL(0xF08F) = 0;

	XVAL(0xFA6F) = 0x1F;
	XVAL(0xFA60) = 2;
	XVAL(0xFA61) = 0;
	XVAL(0xFA64) = 0;
	XVAL(0xFA65) = 0;
	XVAL(0xFA66) = 0;
	XVAL(0xFA67) = 0;
	XVAL(0xFA62) = 0x0F;
	XVAL(0xFA6F) = 0x1F;

	GPIO0DIR &= 0xFD;
	GPIO0OUT |= 2;

	XVAL(0xFA21) = 7;
	XVAL(0xFA21) &= 0xFB;

	XVAL(0xFA68) &= 0xF7;
	XVAL(0xFA69) &= 0xF7;
	XVAL(0xFA6A) &= 0xF7;
	XVAL(0xFA6B) &= 0xF7;

	XVAL(0xFE00) = 0;
	XVAL(0xFE00) = 0x80;

	XVAL(0xFA50) = 0x20;

	XVAL(0xFE01) = 0;
	XVAL(0xFE02) = 0x45;

	TMOD = 0x11;
	TH0 = 0xF0;
	TL0 = 0x5F;
	TH1 = 0xF0;
	TL1 = 0x5F;
	IP = 1;
	TCON = 0x10;
	SCON = 0;
	IE = 0x80;
}

void DoUSBRelatedInit()
{
	if (WARMSTATUS & 2)
	{
		return;
	}

	REGBANK = 5;
	XVAL(0xF210) = 0xFF;
	XVAL(0xF211) = 2;
	XVAL(0xF212) = 3;
	XVAL(0xF213) = 0x24;
	REGBANK = 0;
	XVAL(0xFA6B) = 0xFF;
	while((XVAL(0xF014) & 3)==0);
}

void SendKey(BYTE code, BYTE modifiers)
{
	int i;

	EP3.cs = 0;
	while (EP3.cs & 0x40);

	EP3.fifo = modifiers;
	EP3.fifo = 0;
	EP3.fifo = code;
	for (i = 0; i < 5; i++)
	{
		EP3.fifo = 0;
	}

	EP3.len_l = 8;
	EP3.len_m = 0;
	EP3.len_h = 0;
	EP3.cs = 0x40;
}

void main()
{
	InitHardware();
	DoUSBRelatedInit();
	InitUSB();
	InitTicks();
	InitLED();
	LEDBlink();

	while (1)
	{
		HandleUSBEvents();

		if (wait_tick++ >= KEY_DELAY)
		{
			if (wait_counter < KEY_DELAY)
			{
				wait_counter++;
			}
		}
		
		if (send_keys_enabled && wait_counter >= KEY_DELAY)
		{
			if (keyData[key_index])
			{
				//Send this key, with some padding before, since something's wonky with endpoint 3
				SendKey(0x00, 0x00);
				SendKey(0x00, 0x00);
				SendKey(0x00, 0x00);
				SendKey(0x00, 0x00);
				SendKey(keyData[key_index], keyData[key_index + 1]);
				SendKey(0x00, 0x00);
			}
			else
			{
				//Wait a while
				wait_counter = 0;
				wait_tick = 0;
			}

			//Move to next key
			key_index += 2;
			
			//Are we done?
			if (key_index >= sizeof(keyData))
			{
				send_keys_enabled = 0;
			}
		}
	}
}
