#include "defs.h"
#include "string.h"
#include "usb.h"

#define PREVENT_ALLOW_MEDIUM_REMOVAL	0x1E
#define TEST_UNIT_READY					0x00
#define INQUIRY							0x12
#define READ_FORMAT_CAPACITIES			0x23
#define MODE_SENSE						0x1A
#define REQUEST_SENSE					0x03

#define VENDOR_BOOT						0xBF
#define VENDOR_INFO						0x05
#define VENDOR_CHIPID					0x56
#define CUSTOM_XPEEK					0x06
#define CUSTOM_XPOKE					0x07
#define CUSTOM_IPEEK					0x08
#define CUSTOM_IPOKE					0x09

BYTE 	scsi_status;
DWORD	scsi_data_residue;
DWORD	scsi_transfer_size;
BYTE	scsi_tag[4];
BYTE	scsi_dir_in;
BYTE	scsi_lun;
BYTE 	scsi_cdb[16];
BYTE	scsi_cdb_size;

BYTE HandleCDB()
{
	//Default to returning a bad status
	scsi_status = 1;

	switch(scsi_cdb[0])
	{
		case PREVENT_ALLOW_MEDIUM_REMOVAL:
		{
			scsi_status = 0;
			return 1;
		}
		case TEST_UNIT_READY:
		{
			return 1;
		}
		case INQUIRY:
		{
			memset(usb_buffer, 0, 36);
			usb_buffer[1] = 0x80; //removable media
			usb_buffer[3] = 0x01; //because the UFI spec says so
			usb_buffer[4] = 0x1F; //additional length
			SendData1(36, 0);
			scsi_status = 0;
			return 1;
		}
		case READ_FORMAT_CAPACITIES:
		{
			memset(usb_buffer, 0, 12);
			usb_buffer[3] = 0x08; //capacity list length
			usb_buffer[6] = 0x10; //number of blocks (sectors) (dummy 2MB)
			usb_buffer[8] = 0x03;
			usb_buffer[10] = 0x02; //block length (512 bytes/sector)
			SendData1(12, 0);
			scsi_status = 0;
			return 1;
		}
		case MODE_SENSE:
		{
			memset(usb_buffer, 0, 8);
			usb_buffer[0] = 0x03;
			usb_buffer[2] = 0x80;
			SendData1(4, 0);
			scsi_status = 0;
			return 1;
		}
		case REQUEST_SENSE:
		{
			memset(usb_buffer, 0, 18);
			usb_buffer[0] = 0x70;
			usb_buffer[2] = 0x02;
			usb_buffer[7] = 10;
			usb_buffer[12] = 0x3A;
			SendData1(18, 0);
			scsi_status = 0;
			return 1;
		}
		//Vendor-specific requests
		case 0x06:
		case 0xC6:
		case 0xC7:
		{
			switch(scsi_cdb[1])
			{
				case CUSTOM_XPEEK:
				{
					usb_buffer[0] = XVAL((scsi_cdb[2] << 8) | scsi_cdb[3]);
					SendData1(1, 0);
					break;
				}
				case CUSTOM_XPOKE:
				{
					XVAL((scsi_cdb[2] << 8) | scsi_cdb[3]) = scsi_cdb[4];
					SendData1(1, 0);
					break;
				}
				case CUSTOM_IPEEK:
				{
					usb_buffer[0] = IVAL(scsi_cdb[2]);
					SendData1(1, 0);
					break;
				}
				case CUSTOM_IPOKE:
				{
					IVAL(scsi_cdb[2]) = scsi_cdb[3];
					SendData1(1, 0);
					break;
				}
				case VENDOR_CHIPID:
				{
					int i;
					memset(usb_buffer, 0x00, 0x200);
					
					//Set raw command mode
					XVAL(0xF480) = 0x00;
					XVAL(0xF618) = 0xFF;
					
					//Select chip 0
					XVAL(0xF608) = 0xFE;
					
					//Reset it
					XVAL(0xF400) = 0xFF;
					while (!(XVAL(0xF41E) & 0x01));
					
					//Send read chip ID command
					XVAL(0xF400) = 0x90;
					XVAL(0xF404) = 0x00;
					for (i = 0; i < 6; i++)
					{
						usb_buffer[i] = XVAL(0xF408);
					}
					
					SendData1(0x200, 0);
					scsi_status = 0;
					return 1;
				}
				case VENDOR_INFO: //get info
				{
					int i;

					memset(usb_buffer, 0x00, 0x210);
					usb_buffer[0x094] = 0x00;
					usb_buffer[0x095] = 0x99;
					usb_buffer[0x096] = 0x53;
					usb_buffer[0x17A] = 'V';
					usb_buffer[0x17B] = 'R';
					usb_buffer[0x17E] = 0x23;
					usb_buffer[0x17F] = 0x03;
					usb_buffer[0x200] = 'I';
					usb_buffer[0x201] = 'F';
					SendData1(0x210, 0);
					scsi_status = 0;
					return 1;
				}
				case VENDOR_BOOT:
				{
					//This transfers control to boot mode and will not return.
					XVAL(0xFA14) = 0x07;
					XVAL(0xF747) &= 0xEF;
					XVAL(0xFA15) = 0x06;
					XVAL(0xFA38) |= 0x01;
					XVAL(0xF08F) = 0x00;
					XVAL(0xFA68) &= 0xF7;
					XVAL(0xFA6A) &= 0xF7;
					XVAL(0xFA48) &= 0xFE;
					break;
				}
				default:
				{
					//Not handling it, then
					return 0;
				}
			}
		}
		default:
		{
			//Not handling it, then
			return 0;
		}
	}
}
