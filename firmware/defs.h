#ifndef DEFS_H
#define DEFS_H

#define	MSB(word)      (BYTE)(((WORD)(word) >> 8) & 0xff)
#define LSB(word)      (BYTE)((WORD)(word) & 0xff)

#define XVAL(addr)     (*( __xdata volatile unsigned char  *)(addr))
#define IVAL(addr)     (*( __idata volatile unsigned char  *)(addr))

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
#define TRUE	1
#define FALSE	0

#define BANK0_PA	0x008000UL
#define BANK1_VA 	0x4000U
#define BANK1_PA 	0x00C000UL
#define BANK2_VA 	0x6000U
#define BANK2_PA 	0x00E000UL

#define usb_buffer_PA	0x008000UL
#define usb_buffer_VA	0x0000U

#define USB_VECT	0
#define TMR0_VECT	1
#define EP_VECT		2
#define TMR1_VECT   	3
#define COM0_VECT   	4

#define bmAttach		0x80
#define bmSpeed			7
#define bmSuperSpeed	4
#define bmHighSpeed		0
#define bmFullSpeed		1
#define bmSpeedChange	0x80
#define bmEP2IRQ		2
#define bmEP4IRQ		8
#define bmEP0ACK		1
#define bmEP0NAK		2
#define bmEP0IN			4
#define bmEP0STALL		8
#define bmSUDAV			0x80
#define bmSTALL			2

#define bmNandReady	1

#define bmNandDma0	0
#define bmNandDma1	0x80
#define bmNandDmaRead	0
#define bmNandDmaWrite	0x40

#define bmDmaCmd	7
#define bmDmaCopy	2
#define bmDmaFill	4
#define bmDmaWidth8	0
#define bmDmaWidth16	0x40
#define bmDmaWidth32	0x80

#define bmPRAM	1

__sfr __at (0x80) P0  ;
__sfr __at (0x90) P1  ;
__sfr __at (0xA0) P2  ;
__sfr __at (0xB0) P3  ;
__sfr __at (0xD0) PSW ;
__sfr __at (0xE0) ACC ;
__sfr __at (0xF0) B   ;
__sfr __at (0x81) SP  ;
__sfr __at (0x82) DPL ;
__sfr __at (0x83) DPH ;
__sfr __at (0x87) PCON;
__sfr __at (0x88) TCON;
__sfr __at (0x89) TMOD;
__sfr __at (0x8A) TL0 ;
__sfr __at (0x8B) TL1 ;
__sfr __at (0x8C) TH0 ;
__sfr __at (0x8D) TH1 ;
__sfr __at (0xA8) IE  ;
__sfr __at (0xB8) IP  ;
__sfr __at (0x98) SCON;
__sfr __at (0x99) SBUF;

/*  BIT Register  */
/*  PSW   */
__sbit __at (0xD7) CY   ;
__sbit __at (0xD6) AC   ;
__sbit __at (0xD5) F0   ;
__sbit __at (0xD4) RS1  ;
__sbit __at (0xD3) RS0  ;
__sbit __at (0xD2) OV   ;
__sbit __at (0xD0) P    ;
                 
/*  TCON  */
__sbit __at (0x8F) TF1 ;
__sbit __at (0x8E) TR1 ;
__sbit __at (0x8D) TF0 ;
__sbit __at (0x8C) TR0 ;
__sbit __at (0x8B) IE1 ;
__sbit __at (0x8A) IT1 ;
__sbit __at (0x89) IE0 ;
__sbit __at (0x88) IT0 ;
           
/*  IE        */
__sbit __at (0xAF) EA  ;
__sbit __at (0xAC) ES  ;
__sbit __at (0xAB) ET1 ;
__sbit __at (0xAA) EX1 ;
__sbit __at (0xA9) ET0 ;
__sbit __at (0xA8) EX0 ;
                 
/*  IP        */ 
__sbit __at (0xBC) PS  ;
__sbit __at (0xBB) PT1 ;
__sbit __at (0xBA) PX1 ;
__sbit __at (0xB9) PT0 ;
__sbit __at (0xB8) PX0 ;
                 
/*  P3       */  
__sbit __at (0xB7) RD  ;
__sbit __at (0xB6) WR  ;
__sbit __at (0xB5) T1  ;
__sbit __at (0xB4) T0  ;
__sbit __at (0xB3) INT1;
__sbit __at (0xB2) INT0;
__sbit __at (0xB1) TXD ;
__sbit __at (0xB0) RXD ;

/*  SCON  */
__sbit __at (0x9F) SM0 ;
__sbit __at (0x9E) SM1 ;
__sbit __at (0x9D) SM2 ;
__sbit __at (0x9C) REN ;
__sbit __at (0x9B) TB8 ;
__sbit __at (0x9A) RB8 ;
__sbit __at (0x99) TI  ;
__sbit __at (0x98) RI  ;

__xdata __at 0xF000 volatile BYTE REGBANK;
__xdata __at 0xF008 volatile BYTE USBCTL;
__xdata __at 0xF009 volatile BYTE USBSTAT;
__xdata __at 0xF027 volatile BYTE USBIRQ;
__xdata __at 0xF020 volatile BYTE EPIRQ;
__xdata __at 0xF030 volatile BYTE EPIE;
__xdata __at 0xF048 volatile BYTE EP0CS;
__xdata __at 0xF0B8 volatile BYTE SETUPDAT[8];

typedef struct
{
	BYTE	r0, r1, r2, r3, r4;
	BYTE	ptr_l, ptr_m, ptr_h;
	BYTE	r8, r9;
	BYTE	offset;
	BYTE	rB;
	BYTE	len_l, len_m, len_h;
	BYTE	rF, r10, r11, r12;
	BYTE	cs;
	BYTE	r14, r15, r16, r17, r18, r19;
	BYTE	fifo_count;
	BYTE	r1B;
	BYTE	fifo;
} EPREGS;

__xdata __at 0xF1C0 volatile EPREGS EP0;
__xdata __at 0xF200 volatile EPREGS EP1;
__xdata __at 0xF240 volatile EPREGS EP2;
__xdata __at 0xF280 volatile EPREGS EP3;
__xdata __at 0xF2C0 volatile EPREGS EP4;

__xdata __at 0xF608 volatile BYTE NANDCSOUT;
__xdata __at 0xF618 volatile BYTE NANDCSDIR;

__xdata __at 0xF900 volatile BYTE DMASRCL;
__xdata __at 0xF901 volatile BYTE DMASRCM;
__xdata __at 0xF902 volatile BYTE DMASRCH;
__xdata __at 0xF904 volatile BYTE DMADSTL;	
__xdata __at 0xF905 volatile BYTE DMADSTM;
__xdata __at 0xF906 volatile BYTE DMADSTH;
__xdata __at 0xF908 volatile BYTE DMASIZEL;	
__xdata __at 0xF909 volatile BYTE DMASIZEM;
__xdata __at 0xF90A volatile BYTE DMASIZEH;
__xdata __at 0xF90C volatile BYTE DMAFILL0;	
__xdata __at 0xF90D volatile BYTE DMAFILL1;	
__xdata __at 0xF90E volatile BYTE DMAFILL2;	
__xdata __at 0xF90F volatile BYTE DMAFILL3;	
__xdata __at 0xF930 volatile BYTE DMACMD;

__xdata __at 0xFA14 volatile BYTE GPIO0DIR;
__xdata __at 0xFA15 volatile BYTE GPIO0OUT;
__xdata __at 0xFA38 volatile BYTE WARMSTATUS;

__xdata __at 0xFA40 volatile BYTE BANK0PAL;
__xdata __at 0xFA41 volatile BYTE BANK0PAH;
__xdata __at 0xFA42 volatile BYTE BANK1VA;
__xdata __at 0xFA43 volatile BYTE BANK1PAL;
__xdata __at 0xFA44 volatile BYTE BANK1PAH;
__xdata __at 0xFA45 volatile BYTE BANK2VA;
__xdata __at 0xFA46 volatile BYTE BANK2PAL;
__xdata __at 0xFA47 volatile BYTE BANK2PAH;
__xdata __at 0xFA48 volatile BYTE PRAMCTL; //bit 0 set means run from PRAM

#endif
