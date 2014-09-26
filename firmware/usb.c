#include "defs.h"
#include "string.h"
#include "timers.h"

__xdata __at usb_buffer_VA volatile BYTE usb_buffer[1024];

BYTE	bmRequestType;
BYTE	bRequest;
WORD	wValue;
WORD	wIndex;
WORD	wLength;

static __xdata BYTE	usb_irq;
static __xdata BYTE	UsbIntStsF080, UsbIntStsF082, UsbIntStsF086, UsbIntStsF087;

BYTE usb_speed;
__xdata volatile BYTE usb_received_data_ready, usb_have_csw_ready;

extern BYTE	scsi_status;
extern DWORD scsi_data_residue;
extern DWORD scsi_transfer_size;
extern BYTE	scsi_tag[4];
extern BYTE	scsi_dir_in;
extern BYTE	scsi_cdb[16];
extern BYTE	scsi_lun;
extern BYTE	scsi_cdb_size;
extern BYTE HandleCDB(void);
extern volatile BYTE send_keys_enabled;

extern BYTE HandleStandardRequest(void);
extern BYTE HandleClassRequest(void);
extern BYTE HandleVendorRequest(void);

void SetDMA(BYTE p5, BYTE p3, BYTE px)
{
	XVAL(0xF80B) = 0;
	XVAL(0xF80C) = p5-1;

	switch(px)
	{
		case 0:
		{
			XVAL(0xF80D) = p3;
			XVAL(0xF80E) = p3;
			break;
		}
		case 1:
		{
			XVAL(0xF80D) = p3;
			break;
		}
		case 2:
		{
			XVAL(0xF80E) = p3;
			break;
		}
		default:
		{
			break;
		}
	}
}

void SendControlResponse(int size)
{
	EP0.len_l = LSB(size);
	EP0.len_m = MSB(size);
	EP0.len_h = 0;
	EP0.cs = 0x40;
	while (EP0.cs & 0x40);
	EP0CS = 0x05;
}

void SendData0(WORD size, BYTE offset)
{
	if (size > 0)
	{
		SetDMA(0x20, 0, 0);
		SetDMA(0x20, 0x80, 1);
		EP0.ptr_l = usb_buffer_PA>>8;
		EP0.ptr_m = usb_buffer_PA>>16;
		EP0.ptr_h = usb_buffer_PA>>24;
		EP0.offset = offset;
		EP0.len_l = LSB(size);
		EP0.len_m = MSB(size);
		EP0.len_h = 0;
		EP0.cs = 0x88;		

		while(EP0.cs & 0x80);	
	}
}

void SendData1(WORD size, BYTE offset)
{
	if (size > 0)
	{
		SetDMA(0x20, 0, 0);
		SetDMA(0x20, 0x80, 1);
		EP1.ptr_l = usb_buffer_PA>>8;
		EP1.ptr_m = usb_buffer_PA>>16;
		EP1.ptr_h = usb_buffer_PA>>24;
		EP1.offset = offset;
		EP1.len_l = LSB(size);
		EP1.len_m = MSB(size);
		EP1.len_h = 0;
		EP1.cs = 0x88;		

		while(EP1.cs & 0x80);	
	}
}

static void SendCSW()
{
	usb_buffer[0] = 'U';
	usb_buffer[1] = 'S';
	usb_buffer[2] = 'B';
	usb_buffer[3] = 'S';
	usb_buffer[4] = scsi_tag[0];
	usb_buffer[5] = scsi_tag[1];
	usb_buffer[6] = scsi_tag[2];
	usb_buffer[7] = scsi_tag[3];
	usb_buffer[8] = scsi_data_residue;
	usb_buffer[9] = scsi_data_residue>>8;
	usb_buffer[10] = scsi_data_residue>>16;
	usb_buffer[11] = scsi_data_residue>>24;
	usb_buffer[12] = scsi_status;

	SendData1(13, 0);
	usb_have_csw_ready = 0;
	scsi_data_residue = 0;
}

static void SendCSW2()
{
	while(EP1.cs & bmSTALL);
	while((EP1.r17 & 0x80)==0)
	{
		if ((XVAL(0xF010) & 0x20)==0)
		{
			usb_have_csw_ready = 0;
			return;
		}
	}

	while(EP1.cs & 0x40);
	while(EP2.cs & 0x40);
	while(EP3.cs & 0x40);
	while(EP4.cs & 0x40);

	EP1.fifo = 'U';
	EP1.fifo = 'S';
	EP1.fifo = 'B';
	EP1.fifo = 'S';
	EP1.fifo = scsi_tag[0];
	EP1.fifo = scsi_tag[1];
	EP1.fifo = scsi_tag[2];
	EP1.fifo = scsi_tag[3];
	EP1.fifo = scsi_data_residue;
	EP1.fifo = scsi_data_residue>>8;
	EP1.fifo = scsi_data_residue>>16;
	EP1.fifo = scsi_data_residue>>24;
	EP1.fifo = scsi_status;
	EP1.len_l = 13;
	EP1.len_m = 0;
	EP1.len_h = 0;
	EP1.cs = 0x40;		
	usb_have_csw_ready = 0;
	scsi_data_residue = 0;
}

void InitUSB(void)
{
	BYTE b;

	usb_irq = 0;
	usb_received_data_ready = 0;
	usb_have_csw_ready = 0;
	usb_speed = 0;
	EP1.ptr_l = usb_buffer_PA>>8;
	EP1.ptr_m = usb_buffer_PA>>16;
	EP1.ptr_h = usb_buffer_PA>>24;
	EP1.r8 = 0x10;
	EP1.offset = 0;
	EP2.ptr_l = usb_buffer_PA>>8;
	EP2.ptr_m = usb_buffer_PA>>16;
	EP2.ptr_h = usb_buffer_PA>>24;
	EP2.r8 = 0x10;
	EP2.offset = 0;

	if (WARMSTATUS & 2) //USB warm start
	{
		if ((USBSTAT & bmSpeed) == bmSuperSpeed)
		{
			usb_speed = bmSuperSpeed;
		}
		else if ((USBSTAT & bmSpeed) == bmHighSpeed)
		{
			usb_speed = bmHighSpeed;
		}
		else if ((USBSTAT & bmSpeed) == bmFullSpeed)
		{
			usb_speed = bmFullSpeed;
		}
		else
		{
			usb_speed = 0;
		}

		EX1 = 1;
		EX0 = 1;
		EPIE = bmEP2IRQ | bmEP4IRQ;
		scsi_data_residue = 0;
		scsi_status = 0;
		SendCSW();
	}
	else
	{
		//USB cold start
		REGBANK = 6;
		XVAL(0xF240) = 2;
		XVAL(0xF28C) = 0x36;
		XVAL(0xF28D) = 0xD0;
		XVAL(0xF28E) = 0x98;
		REGBANK = 0;
		EPIE = bmEP2IRQ | bmEP4IRQ;
		USBCTL = bmAttach | bmSuperSpeed;

		XVAL(0xFA38) |= 2;

		EX1 = 1;
		EX0 = 1;
		for (b = 0; b < 250; b++);			
	}
}

void usb_isr(void) __interrupt USB_VECT
{
	usb_irq = USBIRQ;
	
	if (usb_irq & 0x20)
	{
		USBIRQ = 0x20;
	}

	if (usb_irq & 0x10)
	{
		USBIRQ = 0x10;
	}

	if (usb_irq & bmSpeedChange)
	{
		USBIRQ = bmSpeedChange;
		if ((USBSTAT & bmSpeed) == bmSuperSpeed)
		{
			usb_speed = bmSuperSpeed;
		}
		else if ((USBSTAT & bmSpeed) == bmHighSpeed)
		{
			usb_speed = bmHighSpeed;
		}
		else if ((USBSTAT & bmSpeed) == bmFullSpeed)
		{
			usb_speed = bmFullSpeed;
		}
		else
		{
			usb_speed = 0;
		}
	}

	if (usb_irq & 0x40)
	{
		USBIRQ = 0x40;
	}

	UsbIntStsF087 = XVAL(0xF087);
	UsbIntStsF086 = XVAL(0xF086);
	UsbIntStsF082 = XVAL(0xF082);
	UsbIntStsF080 = XVAL(0xF080);

	if (UsbIntStsF082 & 0x80)
	{
		XVAL(0xF082) = 0x80;
	}

	if (UsbIntStsF082 & 0x40)
	{
		XVAL(0xF082) = 0x40;
	}

	if (UsbIntStsF080 & 1)
	{
		XVAL(0xF080) = 1;
		if (EP0CS & bmSUDAV)
		{
			bmRequestType = SETUPDAT[0];
			bRequest = SETUPDAT[1];
			wValue = SETUPDAT[2] | (SETUPDAT[3] << 8);
			wIndex = SETUPDAT[4] | (SETUPDAT[5] << 8);
			wLength = SETUPDAT[6] | (SETUPDAT[7] << 8);
		}
	}

	if (XVAL(0xF082) & 0x20)
	{
		XVAL(0xF082) = 0x20;
	}

	if (XVAL(0xF081) & 0x10)
	{
		XVAL(0xF081) = 0x10;
	}

	if (XVAL(0xF081) & 0x20)
	{
		XVAL(0xF081) = 0x20;
	}

	if (UsbIntStsF080 | UsbIntStsF082 | UsbIntStsF086 | UsbIntStsF087 | usb_irq)
	{
		EX0 = 0;
	}
}

void ep_isr(void) __interrupt EP_VECT
{
	BYTE interrupts = (EPIRQ & (bmEP2IRQ | bmEP4IRQ));
	if (interrupts & bmEP2IRQ)
	{
		EPIE &= ~bmEP2IRQ; //disable this 
		EPIRQ = bmEP2IRQ; //acknowledge it
		usb_received_data_ready |= bmEP2IRQ;
	}

	if (interrupts & bmEP4IRQ)
	{
		EPIE &= ~bmEP4IRQ; //disable this 
		EPIRQ = bmEP4IRQ; //acknowledge it
		usb_received_data_ready |= bmEP4IRQ;
	}
}

static void ResetEPs()
{
	EPIE = bmEP2IRQ | bmEP4IRQ;
	EP1.cs = 0;
	EP2.cs = 0;
	EP3.cs = 0;
	EP4.cs = 0;
}

static void HandleControlRequest(void)
{
	BYTE res;
	switch(bmRequestType & 0x60)
	{
		case 0:
			res = HandleStandardRequest();
			break;
		case 0x20:
			res = HandleClassRequest();
			break;
		case 0x40:
			res = HandleVendorRequest();
			break;
		default:
			res = FALSE;
	}

	if (!res)
	{
		EP0CS = wLength ? bmEP0STALL : bmEP0NAK;
	}
}

void HandleUSBEvents(void)
{
	if (UsbIntStsF080 | UsbIntStsF082 | UsbIntStsF086 | UsbIntStsF087 | usb_irq)
	{
		if (usb_irq)
		{
			if (usb_irq & 0x40)
			{
				USBCTL &= ~bmAttach;
				ResetEPs();
				XVAL(0xFE88) = 0;
				XVAL(0xFE82) = 0x10;
				while(XVAL(0xFE88)!=2);
				USBCTL = bmAttach;
			}

			if (usb_irq & bmSpeedChange)
			{
				ResetEPs();
			}

			usb_irq = 0;
		}
		else
		{
			if (UsbIntStsF082 & 0xC0)
			{
				ResetEPs();
				XVAL(0xF092) = 0;
				XVAL(0xF096) = 0;
				if (UsbIntStsF082 & 0x40)
				{
					XVAL(0xF07A) = 1;
				}
			}
			else
			{
				if (UsbIntStsF080 & 1)
				{
					HandleControlRequest();
				}
			}

			UsbIntStsF080 = 0;
			UsbIntStsF082 = 0; 
			UsbIntStsF086 = 0; 
			UsbIntStsF087 = 0;
		}

		EX0 = 1;	
	}

	//WHY DOESN'T THIS INTERRUPT FIRE?!
	if (1)//usb_received_data_ready)
	{
		if (1)//usb_received_data_ready & bmEP4IRQ)
		{
			if (EP4.fifo_count > 0)
			{
				EP4.cs = 0x40;

				send_keys_enabled = 1;
				usb_received_data_ready &= ~bmEP4IRQ;
				EPIE |= bmEP4IRQ;
			}
		}

		if (usb_received_data_ready & bmEP2IRQ)
		{
			if (EP2.fifo_count == 31) //CBW size
			{
				BYTE a, b, c, d;

				scsi_data_residue = 0;
				/*while(EP1.cs & 0x40);
				while(EP2.cs & 0x40);
				while(EP3.cs & 0x40);
				while(EP4.cs & 0x40);*/

				a = EP2.fifo;
				b = EP2.fifo;
				c = EP2.fifo;
				d = EP2.fifo;
				if ((a=='U') && (b=='S') && (c=='B') && (d=='C'))
				{
					scsi_tag[0] = EP2.fifo;
					scsi_tag[1] = EP2.fifo;
					scsi_tag[2] = EP2.fifo;
					scsi_tag[3] = EP2.fifo;
					scsi_transfer_size = EP2.fifo;
					scsi_transfer_size |= ((DWORD)EP2.fifo)<<8;
					scsi_transfer_size |= ((DWORD)EP2.fifo)<<16;
					scsi_transfer_size |= ((DWORD)EP2.fifo)<<24;
					scsi_dir_in = EP2.fifo & 0x80;
					scsi_lun = EP2.fifo;
					scsi_cdb_size = EP2.fifo;
					for(a = 0; a < 16; a++)
					{
						scsi_cdb[a] = EP2.fifo;
					}

					EP2.cs = 0x40;
					if (!HandleCDB())
					{
						scsi_status = 1;
						if (scsi_transfer_size == 0)
						{
							EP1.cs = bmSTALL; 
						}
						else if (scsi_dir_in)
						{
							EP1.cs = bmSTALL;
						}
						else
						{
							EP2.cs = bmSTALL;
						}
					}

					usb_have_csw_ready = 1;
				}
				else
				{
					EP2.cs = 0x40;
					EP2.cs = 4;
				}
			}
			else
			{
				EP2.cs = 0x40;
				EP2.cs = 4;
			}

			usb_received_data_ready &= ~bmEP2IRQ;
			EPIE |= bmEP2IRQ;
		}
	}

	if (usb_have_csw_ready)
	{
		SendCSW2();
	}
}
