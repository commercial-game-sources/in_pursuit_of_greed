/***************************************************************************/
/*                                                                         */
/*                                                                         */
/* IPX Network Communication Driver for Greed                              */
/* Copyright (C) 1995 by Channel 7                                         */
/*                                                                         */
/* written by Robert Morgan                                                */
/*                                                                         */
/***************************************************************************/

#include "greednet.h"

/**** CONSTANTS ****/

#define MAXPACKETS 31	 // max outstanding packets before loss


/**** TYPES ****/

#pragma option -a-

// setupdata_t is used as greeddata_t during setup
typedef struct
{
 short nodesfound;
 short nodeswanted;
 } setupdata_t;

typedef unsigned char  byte;
typedef unsigned short word;

typedef struct IPXPacketStructure
{
 word PacketCheckSum;         /* high-low */
 word PacketLength;           /* high-low */
 byte PacketTransportControl;
 byte PacketType;
 byte dNetwork[4];            /* high-low */
 byte dNode[6];               /* high-low */
 byte dSocket[2];             /* high-low */
 byte sNetwork[4];            /* high-low */
 byte sNode[6];               /* high-low */
 byte sSocket[2];             /* high-low */
 } IPXPacket;

typedef struct
{
 byte network[4];             /* high-low */
 byte node[6];                /* high-low */
 } localadr_t;

typedef struct
{
 byte node[6];                /* high-low */
 } nodeadr_t;

typedef struct ECBStructure
{
 word Link[2];                /* offset-segment */
 word ESRAddress[2];          /* offset-segment */
 byte InUseFlag;
 byte CompletionCode;
 word ECBSocket;              /* high-low */
 byte IPXWorkspace[4];        /* N/A */
 byte DriverWorkspace[12];    /* N/A */
 byte ImmediateAddress[6];    /* high-low */
 word FragmentCount;          /* low-high */
 word fAddress[2];            /* offset-segment */
 word fSize;                  /* low-high */
 word fAddress2[2];
 word fSize2;
 } ECB;


// time is used by the communication driver to sequence packets returned
// to greed when more than one is waiting

typedef struct
{
 ECB       ecb;
 IPXPacket ipx;
 long      time;
 char      data[DATALENGTH];
 } packet_t;

#pragma option -a


/**** VARIABLES ****/

extern greedcom_t greedcom;
extern nodeadr_t  nodeadr[MAXPLAYERS+1];
extern int	  localnodenum, socketid, numpackets;
extern long	  localt, remotet;    // timestamp of last packet received
extern nodeadr_t  remoteadr;


/**** FUNCTIONS ****/

void           MS_Error(char *error, ...);
int            MS_CheckParm(char *check);
void interrupt IPX_NetISR(void);
void           IPX_InitNetwork(void);
void           IPX_ShutdownNetwork(void);
void           IPX_SendPacket(int destination);
int            IPX_GetPacket(void);
