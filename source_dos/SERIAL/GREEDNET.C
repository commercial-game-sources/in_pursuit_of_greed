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
#include <STRING.H>
#include <CTYPE.H>
#include <STDIO.H>
#include <PROCESS.H>
#include "sergreed.h"
#include "greednet.h"


/**** VARIABLES ****/

greedcom_t      greedcom;
int             vectorhooked, ms_argc;
void (interrupt *oldgreedvect)();
char            **ms_argv;


/**** FUNCTIONS ****/

int MS_CheckParm(char *check)
{
 char *parm;
 int  i;

 for (i=1;i<ms_argc;i++)
  {
   parm=ms_argv[i];
   while (!isalpha(*parm))     // skip - / \ etc.. in front of parm
    if (!*parm++)
     break;                    // hit end of string without an alphanum
   if (!stricmp(check,parm))
    return i;
   }
 return 0;
 }


void LaunchGreed(void)
{
 char *av[30];
 char adrstring1[10], adrstring2[10];
 long flatadr;

 greedcom.id=GREEDCOM_ID;                    // prepare for greed
 if (modemactive)
  greedcom.nettype=NETMODEM;
 oldgreedvect=_dos_getvect(greedcom.intnum); // set the interrupt vector
 _dos_setvect(greedcom.intnum,NetISR);
 vectorhooked=1;
 flatadr=(long)FP_SEG(&greedcom)*16+(long)FP_OFF(&greedcom);
 sprintf(adrstring1,"%lu",flatadr);
 memcpy(av,ms_argv,(ms_argc+1)*2);
 av[ms_argc]="/net";
 av[ms_argc+1]=adrstring1;
 av[ms_argc+2]="/serial";
 flatadr=(long)FP_SEG(&que)*16+(long)FP_OFF(&que);
 sprintf(adrstring2,"%lu",flatadr);
 av[ms_argc+3]=adrstring2;
 av[ms_argc+4]=NULL;
 printf("\nExecuting Greed.\n");
 spawnvp(P_WAIT,"GREED.EXE",av);
 printf("\nReturned from Greed.\n");
 printf("Maximum Usage: %i/%i\n",greedcom.maxusage,QUESIZE);
 if (badpackets)
  printf("Bad Packets: %i/%i\n",badpackets,totalpackets);
 }
