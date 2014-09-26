#include "defs.h"
#include "usb.h"
#include "timers.h"

static const BYTE deviceDescriptor[] = { 0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
                                         0xFE, 0x13, 0x01, 0x52, 0x10, 0x01, 0x00, 0x00, 0x00, 0x01 };
static const BYTE configDescriptor[] = { 0x09, 0x02, sizeof(configDescriptor) & 0xFF, sizeof(configDescriptor) >> 8, 0x02, 0x01, 0x00, 0x80, 0x4B,
                                         0x09, 0x04, 0x00, 0x00, 0x03, 0x08, 0x06, 0x50, 0x00,
										 0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00,
										 0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,
										 0x07, 0x05, 0x83, 0x03, 0x08, 0x00, 0x00,
										 0x09, 0x04, 0x01, 0x00, 0x02, 0x03, 0x01, 0x01, 0x00,
										 0x09, 0x21, 0x01, 0x01, 0x00, 0x01, 0x22,
										 sizeof(HIDreportDescriptor) & 0xFF,
										 sizeof(HIDreportDescriptor) >> 8,
										 0x07, 0x05, 0x83, 0x03, 0x08, 0x00, 0x01,
										 //This is a dummy endpoint to make the descriptor != 0x40, because the controller is stupid.
										 0x07, 0x05, 0x04, 0x03, 0x08, 0x00, 0x01 };
static const BYTE HIDreportDescriptor[] = { 0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x05, 0x07,
											0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01,
											0x95, 0x08, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x81, 0x01,
											0x95, 0x05, 0x75, 0x01, 0x05, 0x08, 0x19, 0x01, 0x29, 0x05,
											0x91, 0x02,	0x95, 0x01, 0x75, 0x03, 0x91, 0x01, 0x95, 0x06,
											0x75, 0x08, 0x15, 0x00, 0x25, 0x65, 0x05, 0x07, 0x19, 0x00,
											0x29, 0x65, 0x81, 0x00, 0xC0 };
static const BYTE deviceQualifierDescriptor[] = { 0x0A, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00 };

void EP0ACK()
{
	EP0CS = bmEP0ACK;
}

static BYTE SetAddress()
{
	BYTE ret = FALSE;

	if (wValue < 0x7F)
	{
		EP0ACK();
		ret = TRUE;
	}

	return ret;
}

static BYTE GetDescriptor()
{
	BYTE type = (wValue >> 8) & 0xFF;
	BYTE i, total;
	BYTE ret = FALSE;

	switch (type)
	{
		case 0x01:
		{
			for (i = 0; i < 0x12; i++)
			{
				EP0.fifo = deviceDescriptor[i];
			}

			SendControlResponse(wLength < 0x12 ? wLength : 0x12);
			ret = TRUE;

			break;
		}
		case 0x02:
		{
			total = wLength < sizeof(configDescriptor) ? wLength : sizeof(configDescriptor);
			for (i = 0; i < total; i++)
			{
				EP0.fifo = configDescriptor[i];
			}

			SendControlResponse(total);
			ret = TRUE;

			break;
		}
		case 0x06:
		{
			for (i = 0; i < sizeof(deviceQualifierDescriptor); i++)
			{
				EP0.fifo = deviceQualifierDescriptor[i];
			}
			
			SendControlResponse(wLength < sizeof(deviceQualifierDescriptor) ? wLength : sizeof(deviceQualifierDescriptor));
			ret = TRUE;
			
			break;
		}
		case 0x22:
		{
			for (i = 0; i < sizeof(HIDreportDescriptor); i++)
			{
				EP0.fifo = HIDreportDescriptor[i];
			}
			
			SendControlResponse(wLength < sizeof(HIDreportDescriptor) ? wLength : sizeof(HIDreportDescriptor));
			ret = TRUE;
		
			break;
		}
		default:
		{
			break;
		}
	}

	return ret;
}

static BYTE SetConfiguration()
{
	BYTE ret = FALSE;

	if (wValue <= 1)
	{
		EP0ACK();
		ret = TRUE;
	}

	return ret;
}

BYTE HandleStandardRequest()
{
	switch(bRequest)
	{
		case 0x05:
		{
			return SetAddress();
		}
		case 0x06:
		{
			return GetDescriptor();
		}
		case 0x09:
		{
			return SetConfiguration();
		}
		default:
		{
			return FALSE;
		}
	}
}

static BYTE GetMaxLUN()
{
	EP0.fifo = 0x00;
	SendControlResponse(wLength < 0x01 ? wLength : 0x01);

	return TRUE;
}

BYTE HandleClassRequest()
{
	switch(bRequest)
	{
		case 0x09:
		{
			EP0CS = 0x05;
			return TRUE;
		}
		case 0x0A:
		{
			EP0ACK();
			return TRUE;
		}
		case 0xFE:
		{
			return GetMaxLUN();
		}
		default:
		{
			return FALSE;
		}
	}
}

BYTE HandleVendorRequest()
{
	return FALSE;
}
