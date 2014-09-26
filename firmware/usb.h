#ifndef _USB_H_INCLUDED
#define _USB_H_INCLUDED

void InitUSB(void);
void HandleUSBEvents(void);
void SendControlResponse(int size);
void SendData0(WORD size, BYTE offset);
void SendData1(WORD size, BYTE offset);
void SetDMA(BYTE p5, BYTE p3, BYTE px);

extern BYTE	bmRequestType;
extern BYTE	bRequest;
extern WORD	wValue;
extern WORD	wIndex;
extern WORD	wLength;

extern BYTE	usb_speed;
extern __xdata __at usb_buffer_VA volatile BYTE usb_buffer[1024];
extern __xdata volatile BYTE usb_received_data_ready;
extern __xdata volatile BYTE usb_have_csw_ready;

#endif
