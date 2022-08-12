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
#include "ipx.h"


/**** CONSTANTS ****/

#define MAXLONG          0x7FFFFFFFL
#define IPX_OPENSOCKET   0x0000
#define IPX_CLOSESOCKET  0x0001
#define IPX_SENDPACKET   0x0003
#define IPX_LISTEN       0x0004
#define IPX_GETLOCALADDR 0x0009
#define IPX_POLLDRIVER   0x000A
#define IPX_LONGEVITY    0x0000
#define IPXINT           Call_IPX()


/**** VARIABLES ****/

packet_t       packets[MAXPACKETS];
nodeadr_t      nodeadr[MAXPLAYERS+1];	// first is local, last is broadcast
nodeadr_t      remoteadr;		// set by each GetPacket
localadr_t     localadr;		// set at startup
int            socketid=0x1234;    	// 0x1234 is the C7 socket
int            numpackets;              // num of packets being used
int            IPXInstalled;
long           localt, remotet;	   	// for time stamp in packets
void far       (*Call_IPX)(void);


/**** FUNCTIONS ****/

unsigned short ShortSwap(unsigned short i)
{
 return (i<<8) + ((i>>8)&255);
 }


int IPX_OpenSocket(short socketNumber)
{
 int socket;

 _BX=IPX_OPENSOCKET;
 _AL=IPX_LONGEVITY;
 _DX=socketNumber;
 IPXINT;
 socket=_DX; // _DX is lost after the next few lines, must store in variable
 if (_AL)
  {
   if (_AL==0xFF)
    printf("Socket 0x%X already open.\n",ShortSwap(socket));
   else
    MS_Error("Error.. IPX_OpenSocket: 0x%X",_AL);
   }
 return socket;
 }


void IPX_CloseSocket(short socketNumber)
{
 _BX=IPX_CLOSESOCKET;
 _DX=socketNumber;
 IPXINT;
 }


void IPX_ListenForPacket(ECB *ecb)
{
 _SI=FP_OFF(ecb);
 _ES=FP_SEG(ecb);
 _BX=IPX_LISTEN;
 IPXINT;
 if (_AL)
  MS_Error("Error.. IPX_ListenForPacket: 0x%X",_AL);
 }


void IPX_GetLocalAddress(void)
{
 int i;

 _SI=FP_OFF(&localadr);
 _ES=FP_SEG(&localadr);
 _BX=IPX_GETLOCALADDR;
 IPXINT;
 printf("Local Address: 0x");
 for(i=0;i<6;i++)
  printf("%X",localadr.node[i]&255);
 printf("\n");
 }


void IPX_InitNetwork(void)
{
 int          i, j;
 union REGS   regs;
 struct SREGS sregs;

// look for an IPX network
 printf("Detecting IPX: ");
 _AX=0x7a00;
 geninterrupt(0x2F);
 if (_AL!=0xFF)
  MS_Error("IPX not detected\n");
 Call_IPX=MK_FP(_ES,_DI);
 IPXInstalled=1;
 printf("Found\n");
// allocate a socket for sending and receiving
 socketid=IPX_OpenSocket(ShortSwap(socketid));
 IPX_GetLocalAddress();
// set up several receiving ECBs
 memset(packets,0,MAXPACKETS*sizeof(packet_t));
 for (i=1;i<numpackets;i++)
  {
   packets[i].ecb.ECBSocket=socketid;
   packets[i].ecb.FragmentCount=1;
   packets[i].ecb.fAddress[0]=FP_OFF(&packets[i].ipx);
   packets[i].ecb.fAddress[1]=FP_SEG(&packets[i].ipx);
   packets[i].ecb.fSize=sizeof(packet_t) - sizeof(ECB);
   IPX_ListenForPacket(&packets[i].ecb);
   }
// set up a sending ECB
 memset(&packets[0],0,sizeof(packets[0]));
 packets[0].ecb.ECBSocket=socketid;
 packets[0].ecb.FragmentCount=2;
 packets[0].ecb.fAddress[0]=FP_OFF(&packets[0].ipx);
 packets[0].ecb.fAddress[1]=FP_SEG(&packets[0].ipx);
 packets[0].ecb.fAddress2[0]=FP_OFF(&greedcom.data);
 packets[0].ecb.fAddress2[1]=FP_SEG(&greedcom.data);
 packets[0].ecb.fSize=sizeof(IPXPacket) + 4; // packet size + timer size
// set up a sending IPX Header
 for (j=0;j<4;j++)
  packets[0].ipx.dNetwork[j]=localadr.network[j];
 packets[0].ipx.dSocket[0]=socketid&255;
 packets[0].ipx.dSocket[1]=socketid>>8;
 packets[0].ipx.PacketType=4;  // packet exchange packet
// known local node at 0
 for (i=0;i<6;i++)
  nodeadr[0].node[i]=localadr.node[i];
// broadcast node at MAXPLAYERS
 for (j=0;j<6;j++)
  nodeadr[MAXPLAYERS].node[j]=0xff;
 }


void IPX_ShutdownNetwork(void)
{
 if (IPXInstalled)
  IPX_CloseSocket(socketid);
 }


/* SendPacket
   A destination of MAXPLAYERS is a broadcast */
void IPX_SendPacket(int destination)
{
 union REGS   r;
 struct SREGS s;
 int          j;

 while (packets[0].ecb.InUseFlag)
  {
   _BX=IPX_POLLDRIVER;
   IPXINT;
   }
// set the time stamp
 packets[0].time=localt;
// set the address
 for (j=0;j<6;j++)
  packets[0].ipx.dNode[j] =
   packets[0].ecb.ImmediateAddress[j] =
   nodeadr[destination].node[j];
// set the data length of the packet
 packets[0].ecb.fSize2=greedcom.datalength;
// send the packet
 _SI=FP_OFF(&packets[0]);
 _ES=FP_SEG(&packets[0]);
 _BX=IPX_SENDPACKET;
 IPXINT;
 }


/* GetPacket
   Returns false if no packet is waiting */
int IPX_GetPacket(void)
{
 int      i, packetnum, usage;
 long     besttic;
 packet_t *packet;

// if multiple packets are waiting, return them in order by time
 besttic=MAXLONG;
 greedcom.remotenode=-1;
 usage=0;
 for (i=1;i<numpackets;i++)
  {
   if (packets[i].ecb.InUseFlag)
    continue;
   usage++;                       // see how many we got waiting
   if (packets[i].time<besttic)
    {
     besttic=packets[i].time;
     packetnum=i;
     }
   }
 if (besttic==MAXLONG)
  return 0;  // no packets
 if (usage>greedcom.maxusage)
  greedcom.maxusage=usage; // find max load
 packet=&packets[packetnum];
 if (besttic==-1 && localt!=-1)
  {
   IPX_ListenForPacket(&packet->ecb);
   return 0;			      // setup broadcast from other game
   }
 remotet=besttic;
// got a bad packet
 if (packet->ecb.CompletionCode)
  MS_Error("Error.. IPX_GetPacket: ecb.ComletionCode=0x%X",packet->ecb.CompletionCode);
// set remoteadr to the sender of the packet
 memcpy(&remoteadr, packet->ipx.sNode,sizeof(remoteadr));
 for (i=0;i<greedcom.numnodes;i++)
  if (!memcmp(&remoteadr,&nodeadr[i],sizeof(remoteadr)))
   break;
 if (i<greedcom.numnodes)
  greedcom.remotenode=i;
 else if (localt!=-1)
  {	// this really shouldn't happen
   IPX_ListenForPacket(&packet->ecb);
   return 0;
   }
// copy out the data
 greedcom.datalength=ShortSwap(packet->ipx.PacketLength) - 34;
 memcpy(&greedcom.data,&packet->data,greedcom.datalength);
// repost the ECB
 IPX_ListenForPacket(&packet->ecb);
 return 1;
 }


