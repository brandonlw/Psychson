#include "defs.h"
#include "equates.h"

#define FEATURE_CHANGE_PASSWORD
//#define FEATURE_EXPOSE_HIDDEN_PARTITION
//#define FEATURE_PREVENT_BOOT

#define NUM_LBAS	0xE6EA40UL //this needs to be even! (round down)

//SCSI command codes
#define SCSI_06						0x06
#define SCSI_06_XPEEK				0x06
#define SCSI_06_XPOKE				0x07
#define SCSI_06_IPEEK				0x08
#define SCSI_06_IPOKE				0x09
#define SCSI_06_BOOT				0xBF
#define SCSI_START_STOP_UNIT		0x1B
#define SCSI_READ_FORMAT_CAPACITIES	0x23
#define SCSI_READ_CAPACITY			0x25
#define SCSI_READ_SECTOR			0x28
#define SCSI_WRITE_SECTOR			0x2A

void memset(BYTE* s, BYTE c, int size)
{
	int i;
	for (i = 0; i < size; i++)
	{
		*s = c;
		s++;
	}
}

void SendData(int size)
{
	int i;

	while(EP1.cs & bmSTALL);
	while((EP1.r17 & 0x80)==0)
	{
		if ((XVAL(0xF010) & 0x20)==0)
		{
			return;
		}
	}

	while(EP1.cs & 0x40);
	while(EP2.cs & 0x40);
	while(EP3.cs & 0x40);
	while(EP4.cs & 0x40);

	for (i = 0; i < size; i++)
	{
		EP1.fifo = EPBUF[i];
	}

	EP1.len_l = size & 0xFF;
	EP1.len_m = (size >> 8) & 0xFF;
	EP1.len_h = 0;
	EP1.cs = 0x40;		
}

void SendCSW(void)
{
	memset(EPBUF, 0, 13);
	EPBUF[0] = 'U';
	EPBUF[1] = 'S';
	EPBUF[2] = 'B';
	EPBUF[3] = 'S';
	EPBUF[4] = scsi_tag[3];
	EPBUF[5] = scsi_tag[2];
	EPBUF[6] = scsi_tag[1];
	EPBUF[7] = scsi_tag[0];
	SendData(13);
}

//Disconnects and then re-enumerates.
void RecycleUSBConnection(void)
{
	USBCTL &= ~bmAttach;
	EPIE = bmEP2IRQ;
	EP1.cs = 0;
	EP2.cs = 0;
	XVAL(0xFE88) = 0;
	XVAL(0xFE82) = 0x10;
	while (XVAL(0xFE88) != 2);
	USBCTL = bmAttach;
}

#ifdef FEATURE_EXPOSE_HIDDEN_PARTITION

//HACK: We're using an unused bit of SYSTEM register 0xFA38 to hold the hidden status,
//	since we don't yet know what RAM is safe to use.
BOOL IsHiddenAreaVisible(void)
{
	return WARMSTATUS & 0x80;
}

//HACK: We're using an unused bit of SYSTEM register 0xFA38 to hold the hidden status,
//	since we don't yet know what RAM is safe to use.
void SetHiddenAreaVisibility(BOOL visible)
{
	if (visible)
	{
		WARMSTATUS |= 0x80;
	}
	else
	{
		WARMSTATUS &= 0x7F;
	}
}

void WaitTenSeconds(void)
{
	WORD i, j;

	for (i = 0; i < 65535; i++)
	{
		for (j = 0; j < 1000; j++)
		{
			//Do nothing
		}
	}
}

#endif

/*
void HandleControlRequest(void)
{
	if (bmRequestType & 0x20)
	{
		//Handle class request
	}
	else if (bmRequestType & 0x40)
	{
		//Handle vendor request
	}
	else
	{
		//Handle standard request
	}
}
*/

/*
void EndpointInterrupt(void)
{
	__asm
		push ACC
		push DPH
		push DPL
		//If no interrupts fired, get out
		mov	DPTR, #EPIRQ
		movx A, @DPTR
		jz 000001$
		//Let the firmware know these events happened, so it can handle them
		mov	B, A
		mov	DPTR, #FW_EPIRQ
		movx A, @DPTR
		orl	A, B
		movx @DPTR, A
		//Disable those interrupts so they don't fire again until we're done with them
		mov	A, #0xFF
		xrl	A, B
		mov	DPTR, #EPIE
		movx @DPTR, A
		//Acknowledge the interrupts
		mov	A, B
		mov	DPTR, #EPIRQ
		movx @DPTR, A
000001$:pop	DPL
		pop	DPH
		pop	ACC
		reti
	__endasm;
}

void HandleEndpointInterrupt(void)
{
	//Handle incoming endpoint data
}
*/

#if defined(FEATURE_EXPOSE_HIDDEN_PARTITION) || defined(FEATURE_PREVENT_BOOT)

void HandleCDB(void)
{
	unsigned long lba;

	switch(scsi_cdb[0])
	{
		case SCSI_06:
		{
			switch (scsi_cdb[1])
			{
				case SCSI_06_XPEEK:
				{
					EPBUF[0] = XVAL((scsi_cdb[2] << 8) | scsi_cdb[3]);
					SendData(1);
					break;
				}
				case SCSI_06_XPOKE:
				{
					XVAL((scsi_cdb[2] << 8) | scsi_cdb[3]) = scsi_cdb[4];
					SendData(1);
					break;
				}
				case SCSI_06_IPEEK:
				{
					EPBUF[0] = IVAL(scsi_cdb[2]);
					SendData(1);
					break;
				}
				case SCSI_06_IPOKE:
				{
					IVAL(scsi_cdb[2]) = scsi_cdb[3];
					SendData(1);
					break;
				}
#ifdef FEATURE_PREVENT_BOOT
				case SCSI_06_BOOT:
				{
					break;
				}
#endif
				default:
				{
					__asm
						ljmp #DEFAULT_CDB_HANDLER
					__endasm;
				}
			}
			break;
		}
		case SCSI_READ_SECTOR: //TODO: we should handle the other READ(X) commands as well
		{
#ifdef FEATURE_EXPOSE_HIDDEN_PARTITION
			//Get the passed-in LBA
			lba = ((unsigned long)(scsi_cdb[2]) << 24) & 0xFF000000;
			lba |= ((unsigned long)(scsi_cdb[3]) << 16) & 0xFF0000;
			lba |= (scsi_cdb[4] << 8) & 0xFF00;
			lba |= scsi_cdb[5];

			//Shift it if necessary
			if (IsHiddenAreaVisible())
			{
				lba += NUM_LBAS / 2;
			}

			//Save it
			scsi_cdb[2] = (lba >> 24) & 0xFF;
			scsi_cdb[3] = (lba >> 16) & 0xFF;
			scsi_cdb[4] = (lba >> 8) & 0xFF;
			scsi_cdb[5] = lba & 0xFF;
#endif
			//Let the firmware do its thing
			__asm
				ljmp #DEFAULT_READ_SECTOR_HANDLER
			__endasm;
		}
#ifdef FEATURE_EXPOSE_HIDDEN_PARTITION
		case SCSI_START_STOP_UNIT:
		{
			//Are we being stopped?
			if (scsi_cdb[4] == 0x02)
			{
				//Yes, set the other section as the visible one
				SetHiddenAreaVisibility(!IsHiddenAreaVisible());

				//Send the CSW
				SendCSW();
				
				//Wait and re-enumerate
				WaitTenSeconds();
				RecycleUSBConnection();
			}
			else
			{
				//No, let things continue normally
				__asm
					ljmp #DEFAULT_CDB_HANDLER
				__endasm;
			}
			break;
		}
		case SCSI_READ_FORMAT_CAPACITIES:
		{
			lba = NUM_LBAS / 2;

			memset(EPBUF, 0, 12);
			EPBUF[3] = 0x08; //capacity list length
			EPBUF[4] = lba >> 24;
			EPBUF[5] = lba >> 16;
			EPBUF[6] = lba >> 8;
			EPBUF[7] = lba & 0xFF;
			EPBUF[8] = 0x02; //descriptor code (formatted media)
			EPBUF[10] = 0x02; //block length (512 bytes/sector)
			SendData(12);
			break;
		}
		case SCSI_READ_CAPACITY:
		{
			lba = (NUM_LBAS / 2) - 1;
		
			memset(EPBUF, 0, 8);
			EPBUF[0] = lba >> 24;
			EPBUF[1] = lba >> 16;
			EPBUF[2] = lba >> 8;
			EPBUF[3] = lba & 0xFF;
			EPBUF[6] = 0x02; //block length (512 bytes/sector)
			SendData(8);
			break;
		}
		case SCSI_WRITE_SECTOR: //TODO: we should handle the other WRITE(x) commands as well
		{
			//Get the passed-in LBA
			lba = ((unsigned long)(scsi_cdb[2]) << 24) & 0xFF000000;
			lba |= ((unsigned long)(scsi_cdb[3]) << 16) & 0xFF0000;
			lba |= (scsi_cdb[4] << 8) & 0xFF00;
			lba |= scsi_cdb[5];
			
			//Shift it if necessary
			if (IsHiddenAreaVisible())
			{
				lba += NUM_LBAS / 2;
			}
			
			//Save it
			scsi_cdb[2] = (lba >> 24) & 0xFF;
			scsi_cdb[3] = (lba >> 16) & 0xFF;
			scsi_cdb[4] = (lba >> 8) & 0xFF;
			scsi_cdb[5] = lba & 0xFF;

			//Let the firmware do its thing
			__asm
				ljmp #DEFAULT_CDB_HANDLER
			__endasm;
		}
#endif
		default:
			__asm
				ljmp #DEFAULT_CDB_HANDLER
			__endasm;
	}
}

#endif

//Called in the firmware's infinite loop.
/*
void LoopDo(void)
{
}
*/

#ifdef FEATURE_CHANGE_PASSWORD

void SetPassword(BYTE* address)
{
	int i;
	for (i = 0; i < 16; i++)
	{
		*(address + i) = 'A';
	}
}

void PasswordReceived()
{
	if (EPBUF[0])
	{
		SetPassword(EPBUF);

	}
	
	if (EPBUF[0x10])
	{
		SetPassword(EPBUF + 0x10);
	}
}

#endif
