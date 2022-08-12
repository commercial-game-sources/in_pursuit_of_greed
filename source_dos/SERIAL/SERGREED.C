/***************************************************************************/
/*                                                                         */
/*                                                                         */
/* Serial Communication Driver for Greed                                   */
/* Copyright (C) 1995 by Channel 7                                         */
/*                                                                         */
/* written by Robert Morgan                                                */
/*                                                                         */
/***************************************************************************/


#include <STDARG.H>
#include <STDIO.H>
#include <CONIO.H>
#include <DOS.H>
#include <STDLIB.H>
#include <STRING.H>
#include <BIOS.H>
#include <TIME.H>
#include "sergreed.h"
#include "greednet.h"


/**** CONSTANTS ****/

#define VERSION "1.031"


/**** VARIABLES ****/

char packet[MAXPACKET];
int  packetlen, dupsuccess[3], readsuccess, badpackets, totalpackets;


/**** FUNCTIONS ****/

void MS_Error(char *error, ...)
{
 va_list argptr;

 if (modemactive)
  {
   printf("\n");
   printf("\nShutting down modem\n");
   outbyte(REG_MCONT,inbyte(REG_MCONT) & ~MCONT_DTR);
   delay(1250);
   outbyte(REG_MCONT,inbyte(REG_MCONT) | MCONT_DTR);
   MOD_Command("+++");
   delay(1250);
   MOD_Command(shutdown);
   delay(1250);
   }
 COM_ShutdownPort();
 if (greedcom.intnum)
  _dos_setvect(greedcom.intnum,oldgreedvect);
 va_start(argptr,error);
 vprintf(error,argptr);
 va_end(argptr);
 printf("\n");
 exit(0);
 }

    // readpacket ->  -1 = checksum error , 1 = ok, 0 = not finished
void interrupt NetISR(void)
{
 if (greedcom.command==CMD_GET)
  {
   readsuccess=COM_ReadPacket();
   if (readsuccess==0)
    greedcom.remotenode=-1;
   else // we got a packet
    {
     if (dupnum>1)
      {
       readsuccess=-1;
       dupnum=2;
       }

     dupsuccess[dupnum]=readsuccess;  // remember how good it was

     if (readsuccess==-1)
      badpackets++;
     totalpackets++;

     if (dupnum==0 && readsuccess==1) // first one ok
      {
       greedcom.remotenode=1;
       greedcom.datalength=packetlen;
       memcpy(&greedcom.data,&packet,packetlen);
       }
     else if (dupnum==1 && dupsuccess[0]==-1 && readsuccess==1)
      { // first one barfed, might as well get the second
       greedcom.remotenode=1;
       greedcom.datalength=packetlen;
       memcpy(&greedcom.data,&packet,packetlen);
       }
     }
   }
 else if (greedcom.command==CMD_SEND)
  COM_WritePacket((char *)&greedcom.data,greedcom.datalength);
 }


void Connect(void)
{
 int  oldsec, curtime, localstage, remotestage;
 char str[20];

 printf("Connecting with remote player");
 oldsec=-1;
 localstage=remotestage=0;
 do
  {
   while (_bios_keybrd(1))
    {
     if ((_bios_keybrd(0) & 0xff)==27)
      {
       printf("\n");
       MS_Error("\nCancel multiplayer game.");
       }
     }
   while (COM_ReadPacket())
    {
     packet[packetlen]=0;
     if (packetlen!=7 || strncmp(packet,"PLAY",4))
      goto badpacket;
     remotestage=packet[6] - '0';
     localstage=remotestage+1;

     if (packet[4]=='0'+greedcom.consoleplayer)
      MS_Error("\nOther player has same player number");

     oldsec=-1;
     }
badpacket:
   curtime=time(NULL);
   if (curtime!=oldsec)
    {
     oldsec=curtime;
     sprintf(str,"PLAY%i_%i",greedcom.consoleplayer,localstage);
     COM_WritePacket(str,strlen(str));
     printf(".");
     }
   } while (remotestage<1);
 printf("\nFound a player!\n");
 while (COM_ReadPacket());  // clear buffers
 }


void main(int argc,char **argv)
{
 char far *vector;
 int      p;

 ms_argc=argc;
 ms_argv=argv;

 setbuf(stdout,NULL);

 greedcom.numnodes=2;
 greedcom.numplayers=2;
 greedcom.nettype=NETSERIAL;

 printf("\n\n"
	"ษอออออออออออออออออออออออออออออออป\n"
	"บ  Greed Serial Setup (v%s)  บ\n"
	"ศอออออออออออออออออออออออออออออออผ\n\n",VERSION);
 if (MS_CheckParm("help"))
  {
   printf("Command Line Parameters:\n"
	  " /player1      = set local to player 1\n"
	  " /player2      = set local to player 2\n"
	  " /dial n       = dial number with modem\n"
	  " /answer       = wait for incomming call\n"
	  " /irq n        = irq of com port\n"
	  " /port n       = port number of com\n"
	  " /com2         = use com #2 (com #1 default)\n"
	  " /com3         = use com #3\n"
	  " /com4         = use com #4\n"
	  " /vector n     = interrupt vector (in hex)\n\n");
   exit(0);
   }

 // set which player
 if (MS_CheckParm("player1"))
  greedcom.consoleplayer=0;
 else if (MS_CheckParm("player2"))
  greedcom.consoleplayer=1;
 if (MS_CheckParm("dial"))
  greedcom.consoleplayer=0;
 else if (MS_CheckParm("answer"))
  greedcom.consoleplayer=1;
 printf("\nYou are player %i of 2.\n\n",greedcom.consoleplayer+1);

 // hook an interrupt vector
 p=MS_CheckParm("vector");
 if (p)
  sscanf(ms_argv[p+1],"%x",&greedcom.intnum);
 else
  {
   for (greedcom.intnum=0x60;greedcom.intnum<=0x66;greedcom.intnum++)
    {
     vector=*(char far * far *)(greedcom.intnum*4);
     if (!vector || (byte)*vector==0xCF)
      break;
     }
   if (greedcom.intnum==0x67)
    MS_Error("No NULL or IRET interrupts were found between 0x60 and 0x66.\n");
   }
 printf("Interrupt: 0x%X\n",greedcom.intnum);

 COM_InitPort();

 if (MS_CheckParm("dial"))
  MOD_Dial();
 else if (MS_CheckParm("answer"))
  MOD_Answer();

 Connect();
 LaunchGreed();
 MS_Error("");
 }

