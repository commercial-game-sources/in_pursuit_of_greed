/***************************************************************************/
/*                                                                         */
/*                                                                         */
/* Serial Communication Driver for Greed                                   */
/* Copyright (C) 1995 by Channel 7                                         */
/*                                                                         */
/* written by Robert Morgan                                                */
/*                                                                         */
/***************************************************************************/

#include <DOS.H>
#include <CONIO.H>
#include <STDIO.H>
#include <STRING.H>
#include "greednet.h"
#include "sergreed.h"


/**** CONSTANTS ****/

int ISA_IRQ[]  = { 4, 3, 4, 3 };
int ISA_PORT[] = { 0x3f8, 0x2f8, 0x3e8, 0x2e8 };


/**** VARIABLES ****/

enum {UART_8250, UART_16550} uart_type;

void (interrupt *oldirqvect)();
char            localbuffer[MAXPACKET];
ques_t          que;
int             uart, irq, irqintnum, dupnum=-1;
int             comport, maxcount, inescape, newpacket, checksum=-1, readchecksum;


/**** FUNCTIONS ****/

void COM_GetUart(void)
{
 int       p;
 short far *BiosPort;

 BiosPort=MK_FP(0x40,0);
 if (MS_CheckParm("com2"))
  comport=2;
 else if (MS_CheckParm("com3"))
  comport=3;
 else if (MS_CheckParm("com4"))
  comport=4;
 else
  comport=1;
 irq=ISA_IRQ[comport-1];            // set IBM defaults
 uart=BiosPort[comport-1];          // check the BIOS settings
 if (uart==0)
  uart=ISA_PORT[comport-1];
 p=MS_CheckParm("port");
 if (p)
  sscanf(ms_argv[p+1],"%x",&uart);
 p=MS_CheckParm("irq");
 if (p)
  sscanf(ms_argv[p+1],"%i",&irq);
 if (irq>7)
  MS_Error("IRQ must be less than 8!");
 printf("IRQ: %i\nCOM: %i\nPort: 0x%X\n",irq,comport,uart);
 }


void COM_BaudSet(long baudrate)
{
 CLI();
 outbyte(REG_LCONT,inbyte(REG_LCONT)|LCONT_DLAB);
 outword(uart,115200/baudrate);
 outbyte(REG_LCONT,inbyte(REG_LCONT)&(LCONT_DLAB-1));
 STI();
 }


void COM_ParmSet(int parity,int stop,int databits)
{
 byte parmbyte;

 parmbyte=databits-5;
 if (stop==1)
  parmbyte|=LCONT_STOP_BITS;
 if (parity!=PARITY_NONE)
  parmbyte|=LCONT_PARITY_ENABLE;
 if (parity==PARITY_EVEN)
  parmbyte|=LCONT_EVEN_PARITY_SELECT;
 outbyte(REG_LCONT,parmbyte);
 }


void COM_InitPort(void)
{
 int mcr; //, temp;

 COM_GetUart();                      // get uart type and settings
 printf("Baud: %i\n",SERIALBAUD);
 COM_BaudSet(SERIALBAUD);            // init com port settings
 COM_ParmSet(PARITY_NONE,0,8);
// outbyte(REG_FCONT,FCR_FIFO_ENABLE); // check for FIFO (16550 series)
// temp=inbyte(REG_INT_ID);
// printf("UART: ");
/* if (temp & 0xC0)
  {
   uart_type=UART_16550;
   maxcount=14;
   if ((temp & 0xC0)==0xC0)
    printf("16550A\n");
   else
    printf("16550\n");
   outbyte(REG_FCONT,FCR_FIFO_ENABLE + FCR_TRIGGER_16);
   }
 else
  {  */
   uart_type=UART_8250;
   maxcount=1;
//   outbyte(REG_FCONT,0);
//   printf("8250.  Warning, this chip may not be suited for high speed modems!\n");
//   }
 que.uart=uart;
 que.uarttype=uart_type;
 outbyte(REG_INT_EN,0);               // prepare for interrupts
 mcr=inbyte(REG_MCONT);
 mcr|=MCONT_OUT2;
 mcr&=~MCONT_LOOPBACK;
 outbyte(REG_MCONT,mcr);

 inbyte(REG_TX);                      // clear any pending interrupts
 inbyte(REG_INT_ID);

 irqintnum=irq + 8;
 oldirqvect=_dos_getvect(irqintnum);  // hook the irq vector
 que.irqintnum=irqintnum;
 que.intseg=FP_SEG(ISR_8250);
 que.intofs=FP_OFF(ISR_8250);
 _dos_setvect(irqintnum,ISR_8250);

 outbyte(0x21,inbyte(0x21) & ~(1<<irq)); // enable irq
 CLI();
// inbyte(REG_MCONT);
 outbyte(REG_INT_EN,IER_RX_DATA_READY + IER_TX_HOLDING_REGISTER_EMPTY);
 outbyte(0x20,0xC2);   // enable interrupts through the interrupt controller
 outbyte(REG_MCONT,inbyte(REG_MCONT) | MCONT_DTR);
 STI();
 }


void COM_ShutdownPort(void)
{
 outbyte(REG_INT_EN,0);
 outbyte(REG_MCONT,0);
 outbyte(0x21,inbyte(0x21) | (1<<irq));
 _dos_setvect(irqintnum,oldirqvect);
 COM_BaudSet(9600);                  // reset com port to default
 COM_ParmSet(PARITY_NONE,0,8);
 }


int COM_ReadByte(void)
{
 int c;

 if (que.in.tail>=que.in.head) return -1;
 c=que.in.data[que.in.tail&QUESIZE];
 que.in.tail++;
 return c;
 }


void interrupt ISR_8250(void)
{
 int count;

 STI();
 while (1)
  {
   switch(inbyte(REG_INT_ID) & 7)
    {
     case IIR_RX: // receive
//      do
  //     {
	que.in.data[que.in.head&QUESIZE]=inbyte(REG_RX);
	que.in.head++;
	que.rreceived++;
//	} while (uart_type==UART_16550 && inbyte(REG_LSTAT)&LSTAT_DATA_READY);
      continue;
     case IIR_TX: // transmit
      for (count=0;count<maxcount && que.out.tail<que.out.head;count++)
       {
	outbyte(REG_TX,que.out.data[que.out.tail&QUESIZE]);
	que.out.tail++;
	que.rsent++;
	}
      break;
     case IIR_MSTAT:
      inbyte(REG_MSTAT);
      break;
     case IIR_LSTAT:
      inbyte(REG_LSTAT);
      break;
     default:  // done
      outbyte(0x20,0x20);
      return;
     }
   }
 }


void COM_WriteBuffer(char *buffer, unsigned int count)
{
 while (count--)
  {
   que.out.data[que.out.head&QUESIZE]=*buffer++;
   que.out.head++;
   }
 if (inbyte(REG_LSTAT)&LSTAT_THRE)
  {
   outbyte(uart,que.out.data[que.out.tail&QUESIZE]);
   que.out.tail++;
   }
 }


int COM_ReadPacket(void)
{
 int c, usage;

 usage=que.in.head-que.in.tail;
 if (usage>QUESIZE - 4)   // check for buffer overflow
  {
   que.in.tail=que.in.head;
   newpacket=true;
   return 0;
   }
 if (usage>greedcom.maxusage)
  greedcom.maxusage=usage;

 if (newpacket)
  {
   packetlen=0;
   newpacket=0;
   checksum=-1;
   dupnum=-1;
   }
 do
  {
   if (que.in.tail>=que.in.head) return 0; // haven't read a complete packet
   c=que.in.data[que.in.tail&QUESIZE];
   que.in.tail++;

   if (checksum==-1)
    {
     checksum=c;           // get the checksum
     readchecksum=0;       // reset the read checksum to zero
     continue;
     }
   if (dupnum==-1)
    {
     dupnum=c;
     continue;
     }
   readchecksum+=c;        // add to the read checksum
   if (inescape)
    {
     inescape=false;
     if (c==FRAMETERM)
      {
       newpacket=1;
       if ((readchecksum&255)!=checksum) return -1; // bad checksum
       return 1;           // got a good packet
       }
     }
   else if (c==FRAMECHAR)
    {
     inescape=true;
     continue;             // don't know if it a terminator or a FRAMECHAR
     }
   if (packetlen>=MAXPACKET) continue; // oversize packet
   packet[packetlen]=c;
   packetlen++;
   } while (1);
 }


void COM_WritePacket(char *buffer, int len)
{
 int b, checksum;

 if (len>MAXPACKET)
  return;
 b=2;                    // byte 0 is checksum, byte 1 is duplication number
 checksum=0;
 while (len--)
  {
   if (*buffer==FRAMECHAR)
    {
     checksum+=FRAMECHAR;
     localbuffer[b++]=FRAMECHAR; // escape it for literal
     }
   checksum+=*buffer;
   localbuffer[b++]=*buffer++;
   }
 localbuffer[b++]=FRAMECHAR;
 localbuffer[b++]=FRAMETERM;
 checksum+=FRAMECHAR;
 localbuffer[0]=checksum&255;

 localbuffer[1]=0;              // duplication count
 COM_WriteBuffer(localbuffer,b);
 }
