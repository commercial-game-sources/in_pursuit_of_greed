/***************************************************************************/
/*                                                                         */
/*                                                                         */
/* IPX Network Communication Driver for Greed                              */
/* Copyright (C) 1995 by Channel 7                                         */
/*                                                                         */
/* written by Robert Morgan                                                */
/*                                                                         */
/***************************************************************************/


/**** CONSTANTS ****/

#define	MAXPLAYERS   8	         // max players at any time on this socket
#define	CMD_SEND     1
#define	CMD_GET	     2
#define DATALENGTH   128          // maximum data packet size
#define GREEDCOM_ID  0xC7C7C7C7L  // greed packet id
#define NETIPX       1
#define NETSERIAL    2
#define NETMODEM     3


/**** TYPES ****/

typedef struct
{
 long	id;
 short	intnum;			// greed executes an int to send commands
 short  maxusage;
 short  nettype;
// communication between greed and the driver
 short	command;		// CMD_SEND or CMD_GET
 short	remotenode;             // dest for send, set by get (-1 = no packet)
 short  datalength;
// info common to all nodes
 short	numnodes;		// console is allways node 0
// info specific to this node
 short	consoleplayer;	        // 0-3 = player number
 short	numplayers;		// 1-4
// packet data to be sent
 char	data[DATALENGTH];
 } greedcom_t;

typedef enum {false,true} boolean;


/**** VARIABLES ****/

extern greedcom_t      greedcom;
extern void (interrupt *oldgreedvect)();
extern boolean	       vectorhooked;
extern int             maxusage;


/**** FUNCTIONS ****/

void LaunchGreed(void);

