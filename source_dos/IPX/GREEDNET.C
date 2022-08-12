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
#include <STRING.H>
#include <DOS.H>
#include <PROCESS.H>
#include <CTYPE.H>
#include "ipx.h"


/**** VARIABLES ****/

greedcom_t      greedcom;
boolean	        vectorhooked;
void (interrupt *oldgreedvect)();


/**** FUNCTIONS ****/

int MS_CheckParm(char *check)
{
 char *parm;
 int  i;

 for (i=1;i<_argc;i++)
  {
   parm=_argv[i];
   while (!isalpha(*parm))	// skip - / \ etc.. in front of parm
   if (!*parm++)
    break;   	// hit end of string without an alphanum
   if (!stricmp(check,parm))
    return i;
   }
 return 0;
 }


void LaunchGreed(void)
{
 char *av[40];
 char adrstring[10];
 long flatadr;

// prepare for greed
 greedcom.id=GREEDCOM_ID;
// set the interrupt vector
 oldgreedvect=_dos_getvect(greedcom.intnum);
 _dos_setvect(greedcom.intnum,IPX_NetISR);
 vectorhooked=true;
// build the argument list for greed, adding a -net &greedcom
 flatadr=(long)FP_SEG(&greedcom)*16+(long)FP_OFF(&greedcom);
 sprintf(adrstring,"%lu",flatadr);
 memcpy(av,_argv,(_argc+1)*2);
/* av[_argc]=av[1];
 av[1]="/TR=RSI GREED.EXE";
 av[_argc+1]="/net";
 av[_argc+2]=adrstring;
 av[_argc+3]=NULL;
 printf("Executing Greed.\n");
 spawnvp(P_WAIT,"WD.EXE",av); */
 av[_argc]="/net";
 av[_argc+1]=adrstring;
 av[_argc+2]=NULL;
 printf("Executing Greed.\n");
 spawnvp(P_WAIT,"GREED",av);
 printf("\nReturned from Greed.\n");
 printf("Maximum Usage: %i/%i\n",greedcom.maxusage,numpackets-1);
 }
