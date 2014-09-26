#ifndef DEFS_H
#define DEFS_H

#define	MSB(word)      (BYTE)(((WORD)(word) >> 8) & 0xff)
#define LSB(word)      (BYTE)((WORD)(word) & 0xff)

#define XVAL(addr)     (*( __xdata volatile unsigned char  *)(addr))
#define IVAL(addr)     (*( __idata volatile unsigned char  *)(addr))

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef __bit          BOOL;
typedef __bit bit;
#define TRUE	1
#define FALSE	0

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

// ------------------------------------------------------------------------------------------------
// *                                        SFRs
// * ------------------------------------------------------------------------------------------------
//  BYTE Register 
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

// ------------------------------------------------------------------------------------------------
//                                        Xdata F000-F3FF USB Registers
// ------------------------------------------------------------------------------------------------
// some banking registers switching
// value: 0-7, default: 0
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
	BYTE	r0,r1,r2,r3,r4;
	BYTE	ptr_l, ptr_m, ptr_h; //buffer ptr = buf_pa>>8
	BYTE	r8,r9;
	BYTE	ofs; // buffer offset, data addr will be ptr<<8 + ofs*0x200
	BYTE	rB;
	BYTE	len_l, len_m, len_h; //C,D,E
	BYTE	rF,r10,r11,r12;
	BYTE	cs; //13
	BYTE	r14,r15,r16,r17,r18,r19;
	BYTE	fifo_count;
	BYTE	r1B;
	BYTE	fifo; //1C
} EPREGS;

__xdata __at 0xF1C0 volatile EPREGS EP0;
__xdata __at 0xF200 volatile EPREGS EP1;
__xdata __at 0xF240 volatile EPREGS EP2;
__xdata __at 0xF280 volatile EPREGS EP3;
__xdata __at 0xF2C0 volatile EPREGS EP4;

typedef struct
{
	BYTE raw_cmd;	
	BYTE u1[3];
	BYTE raw_addr;
	BYTE u5[3];
	BYTE raw_data;
	BYTE r9;
	BYTE uA[2];
	BYTE rC;
	BYTE uD[3];
	BYTE r10;
	BYTE u11[7];
	BYTE r18;
	BYTE u19[5];
	BYTE status; // .0 - R/nB
	BYTE u1F[0x19];
	BYTE r38, r39, r3A;
	BYTE u3B;
	BYTE r3C, r3D;
	BYTE u3E[2];
	BYTE r40;
	BYTE dma_size; // DMA size in KB
	BYTE r42;
	BYTE dma_mode; // DMA modes
	BYTE u44[3];
	BYTE r47;
	BYTE u48[0x14];
	BYTE r5C; // nand command
	// DMA command. |=1 to go, wait until .0 cleared
	BYTE dma_cmd;
	BYTE u61[0x0B];
	BYTE dma1_page; // DMA1 start page. Autoincrements
	BYTE u6D[3];
	BYTE dma0_page; // DMA0 start page. Autoincrements
	BYTE u71[3];
	// DMA1 PA. This pseudo reg sets dma1_page actually. RAZ
	BYTE dma1_ptr0, dma1_ptr1, dma1_ptr2, dma1_ptr3; 
	// DMA0 PA. This pseudo reg sets w_dma_page actually. RAZ
	BYTE dma0_ptr0, dma0_ptr1, dma0_ptr2, dma0_ptr3; 
	BYTE u7C[4];
	BYTE r80;
	BYTE u81[0x1B];
	BYTE page_size_l, page_size_h; // 9C full page size with spare
	BYTE r9E, r9F;
	BYTE uA0[0x4C];
	BYTE rEC;
	BYTE uED[0x13];
} NANDREGS;

__xdata __at 0xF400 volatile NANDREGS NFC0;
__xdata __at 0xF500 volatile NANDREGS NFC1;

__xdata __at 0xF608 volatile BYTE NANDCSOUT;
__xdata __at 0xF618 volatile BYTE NANDCSDIR;
//F638, F639 - scrambler control
//F638 | 18 & 7F - turn off descrambler
__xdata __at 0xF700 volatile NANDREGS NFCX;

// DMA copy source / fill destination physical address
__xdata __at 0xF900 volatile BYTE DMASRCL;
__xdata __at 0xF901 volatile BYTE DMASRCM;
__xdata __at 0xF902 volatile BYTE DMASRCH;

// DMA copy destination physical address
__xdata __at 0xF904 volatile BYTE DMADSTL;	
__xdata __at 0xF905 volatile BYTE DMADSTM;
__xdata __at 0xF906 volatile BYTE DMADSTH;

// DMA copy size in bytes (always in bytes, regardless of cmd width)
__xdata __at 0xF908 volatile BYTE DMASIZEL;	
__xdata __at 0xF909 volatile BYTE DMASIZEM;
__xdata __at 0xF90A volatile BYTE DMASIZEH;

// DMA fill value
__xdata __at 0xF90C volatile BYTE DMAFILL0;	
__xdata __at 0xF90D volatile BYTE DMAFILL1;	
__xdata __at 0xF90E volatile BYTE DMAFILL2;	
__xdata __at 0xF90F volatile BYTE DMAFILL3;	

// DMA command
__xdata __at 0xF930 volatile BYTE DMACMD;

// ------------------------------------------------------------------------------------------------
//                                        Xdata FA00-FAFF SYSTEM Registers
// ------------------------------------------------------------------------------------------------
__xdata __at 0xFA14 volatile BYTE GPIO0DIR;
__xdata __at 0xFA15 volatile BYTE GPIO0OUT;
__xdata __at 0xFA38 volatile BYTE WARMSTATUS;

// XDATA banking
// XDATA consists of 3 mapped areas: BANK0,1,2
// BANK0 start is fixed at 0000, BANK1,2 starts at variable addresses
// in case of overlapped addresses a bank with higher number has higher priority

// xdata BANK0 mapping registers
// maps XDATA VA 0000 to PA=BANK0PAH:L<<9, size - up to BANK1/2
__xdata __at 0xFA40 volatile BYTE BANK0PAL;
__xdata __at 0xFA41 volatile BYTE BANK0PAH;

// xdata BANK1 mapping registers
// maps XDATA VA=(BANK1VA & 0xFE)<<8 to PA=BANK1PAH:L<<9, size - up to BANK2
__xdata __at 0xFA42 volatile BYTE BANK1VA;
__xdata __at 0xFA43 volatile BYTE BANK1PAL;
__xdata __at 0xFA44 volatile BYTE BANK1PAH;

// xdata BANK2 mapping registers
// maps XDATA VA=(BANK2VA & 0xFE)<<8 to PA=BANK2PAH:L<<9, size - up to F000 
__xdata __at 0xFA45 volatile BYTE BANK2VA;
__xdata __at 0xFA46 volatile BYTE BANK2PAL;
__xdata __at 0xFA47 volatile BYTE BANK2PAH;

// PRAM/PROM switching with restart
// value: bit0 =0 - run from PROM, =1 - run from PRAM. Changing this bit causes CPU restart!
__xdata __at 0xFA48 volatile BYTE PRAMCTL;

#endif
