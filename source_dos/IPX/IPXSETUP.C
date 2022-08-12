/***************************************************************************/
/*                                                                         */
/*                                                                         */
/* IPX Network Communication Driver for Greed                              */
/* Copyright (C) 1995 by Channel 7                                         */
/*                                                                         */
/* written by Robert Morgan                                                */
/*                                                                         */
/***************************************************************************/

#include <STDIO.H>
#include <DOS.H>
#include <STRING.H>
#include <STDARG.H>
#include <BIOS.H>
#include <CONIO.H>
#include <STDLIB.H>
#include <TIME.H>
#include "ipx.h"


/**** CONSTANTS ****/

#define VERSION             "1.026"
#define PEL_WRITE_ADR       0x3c8
#define PEL_DATA            0x3c9
#define I_ColorBlack(r,g,b) {                       \
                             outp(PEL_WRITE_ADR,0); \
                             outp(PEL_DATA,r);      \
			     outp(PEL_DATA,g);      \
			     outp(PEL_DATA,b);      \
			     }


/**** VARIABLES ****/

int	    numnetnodes;
setupdata_t nodesetup[MAXPLAYERS];


/**** FUNCTIONS ****/

void MS_Error(char *error, ...)
{
 va_list argptr;

 if (vectorhooked)
  _dos_setvect(greedcom.intnum,oldgreedvect);
 IPX_ShutdownNetwork();
 va_start(argptr,error);
 vprintf(error,argptr);
 va_end(argptr);
 printf("\n");
 exit(1);
 }


void interrupt IPX_NetISR(void)
{
 if (greedcom.command==CMD_GET)
  IPX_GetPacket();    // get called more often
 else if (greedcom.command==CMD_SEND)
  {
   localt++;
   IPX_SendPacket(greedcom.remotenode);
   }
 }


/*
 LookForNodes
 Finds all the nodes for the game and works out player numbers among them
 Exits with nodesetup[0..numnodes] and nodeadr[0..numnodes] filled in
*/
void LookForNodes(void)
{
 time_t      curtime, oldsec;
 setupdata_t *setup, *dest;
 int	     i, total, console;

// wait until we get [numnetnodes] packets, then start playing
// the playernumbers are assigned by netid
 printf("\nNeed %i players.\nLooking",numnetnodes-1);
 oldsec=-1;
 setup=(setupdata_t *)&greedcom.data;
 localt=-1;		// in setup time, not game time
// build local setup info
 nodesetup[0].nodesfound=1;
 nodesetup[0].nodeswanted=numnetnodes;
 greedcom.numnodes=1;
 while (1)
  {
   // check for aborting
   while (_bios_keybrd(1))
    {
     if ((_bios_keybrd(0) & 0xff)==27)
      {
       printf("\n");
       MS_Error("\nCancel multiplayer game.");
       }
     }
   // listen to the network
   while (IPX_GetPacket())
    {
     if (greedcom.remotenode==-1)
      dest=&nodesetup[greedcom.numnodes];
     else
      dest=&nodesetup[greedcom.remotenode];
     if (remotet!=-1)
      {
       // an early game packet, not a setup packet
       if (greedcom.remotenode==-1)
	{
	 printf("\n");
	 MS_Error("\nAnother game is in progress on this socket or unknown game packet");
	 }
       // if it allready started, it must have found all nodes
       dest->nodesfound=dest->nodeswanted;
       continue;
       }
     // update setup ingo
     memcpy(dest,setup,sizeof(*dest));
     if (greedcom.remotenode!=-1)
      continue; // already know that node address
     // this is a new node
     memcpy(&nodeadr[greedcom.numnodes],&remoteadr,sizeof(nodeadr[greedcom.numnodes]));
     // if this node has a lower address, take all startup info
     if (memcmp(&remoteadr,&nodeadr[0],sizeof(&remoteadr))<0) ;
     greedcom.numnodes++;
     printf("\nFound a player!\n");
     if (greedcom.numnodes<numnetnodes)
      printf("Need %i players.\nLooking",numnetnodes-greedcom.numnodes);
     }
  // we are done if all nodes have found all other nodes
   for (i=0;i<greedcom.numnodes;i++)
    if (nodesetup[i].nodesfound!=nodesetup[i].nodeswanted)
     break;
   if (i==nodesetup[0].nodeswanted)
    break;  // got them all
  // send out a broadcast packet every second
   curtime=time(NULL);
   if (curtime==oldsec)
    continue;
   oldsec=curtime;
   if (greedcom.numnodes<numnetnodes)
    printf(".",curtime);
   nodesetup[0].nodesfound=greedcom.numnodes;
   greedcom.datalength=sizeof(*setup);
   memcpy(&greedcom.data,&nodesetup[0],sizeof(*setup));
   IPX_SendPacket(MAXPLAYERS);	// send to all
   }
// count players
 total=0;
 console=0;
 for (i=0;i<numnetnodes;i++)
  {
   total++;
   if (total>MAXPLAYERS)
    MS_Error("Too many players! (players>%i)",MAXPLAYERS);
   if (memcmp(&nodeadr[i],&nodeadr[0],sizeof(nodeadr[0]))<0)
    console++;
   }
 if (!total)
  MS_Error("Must have at least 1 player!");
 greedcom.consoleplayer=console;
 greedcom.numplayers=total;
 printf("\nYou are player %i of %i.\n",console+1,total);
 }


void cdecl main(void)
{
 int      i, p, sid;
 char far *vector;

// determine game parameters
 numnetnodes=2;
 greedcom.nettype=NETIPX;
 printf("\n\n"
	"ษออออออออออออออออออออออออออออออออออออป\n"
	"บ  Greed IPX Network Setup (v%s)  บ\n"
	"ศออออออออออออออออออออออออออออออออออออผ\n\n",VERSION);
 if (MS_CheckParm("help"))
  {
   printf("Command Line Parameters:\n"
	  " /players n    = number of players\n"
	  " /socket n     = socket number\n"
	  " /vector n     = interrupt vector (in hex)\n"
	  " /socketh n    = socket number (in hex)\n\n");
   exit(0);
   }
 i=MS_CheckParm("players");
 if (i && i<_argc-1)
  numnetnodes=atoi(_argv[i+1]);
 printf("Players: %i\n",numnetnodes);

 numpackets=MAXPACKETS;

 i=MS_CheckParm("socket");
 if (i>0 && i<_argc-1)
  {
   sid=atoi(_argv[i+1]);
   if (sid>0) socketid=sid;
   }
 i=MS_CheckParm("socketh");
 if (i>0 && i<_argc-1)
  {
   sscanf(_argv[i+1],"%x",&sid);
   if (sid>0) socketid=sid;
   }

 printf("Socket: 0x%X (%i)\n",socketid,socketid);
 // hook an interrupt vector
 p=MS_CheckParm("vector");
 if (p)
  sscanf(_argv[p+1],"0x%x",&greedcom.intnum);
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
// make sure the network exists and create a bunch of buffers
 IPX_InitNetwork();
// get addresses of all nodes
 LookForNodes();
 localt=0;	       // no longer in setup
 LaunchGreed();        // spawn the program and pass it the net info
 MS_Error("");
 }


