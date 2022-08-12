/***************************************************************************/
/*                                                                         */
/*                                                                         */
/* Raven 3D Engine                                                         */
/* Copyright (C) 1995 by Softdisk Publishing                               */
/*                                                                         */
/* Original Design:                                                        */
/*  John Carmack of id Software                                            */
/*                                                                         */
/* Enhancements by:                                                        */
/*  Robert Morgan of Channel 7............................Main Engine Code */
/*  Todd Lewis of Softdisk Publishing......Tools,Utilities,Special Effects */
/*  John Bianca of Softdisk Publishing..............Low-level Optimization */
/*  Carlos Hasan..........................................Music/Sound Code */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <i86.h>
#include <string.h>
#include "setup.h"


/**** VARIABLES ****/

unsigned __near __minreal = 40*1024;

static netargs args;
netargs far    *netstuff;
long           strucoff;


/**** FUNCTIONS ****/

void i2a(char *output,long input,int n)
{
 int i=n;

 for (i=n;i>0;i--)
  {
   output[i-1]=input%10 + 48;
   input/=10;
   }
 }


void itohex(char *output,char *input)
{
 int           i;
 unsigned long n=0;
 int           m=1;

 for (i=3;i>=0;i--)
  {
   n+=(input[i]-48)*m;
   m*=16;
   }
 i2a(output,n,4);
 }


void main()
{
 int  exit_code;
 char strptr[]="           ";
 char charnum[]="  ";
 char ctemp[7], str2[5], dial[12];

 netstuff=&args;
 netstuff->numplayers=2;
 strucoff=FP_SEG(netstuff);
 strucoff*=16;
 strucoff+=FP_OFF(netstuff);
 i2a(strptr,strucoff,10);
 exit_code=spawnlp(P_WAIT,"set.exe","set.exe",strptr,NULL);

 i2a(&charnum[0],(char)netstuff->character,1);

 if (exit_code==4) // run greed
  spawnlp(P_OVERLAY,"greed.exe","greed.exe");

 if (exit_code==5) // ipx network
  {
   sprintf(ctemp,"%s",&netstuff->netsocket[0]);
   itohex(&ctemp[0],&ctemp[0]);
   sprintf(strptr,"%d",netstuff->numplayers);
   spawnlp(P_OVERLAY,"netgreed.exe","netgreed.exe","/players",strptr,"/socket",&ctemp[0],"/char",charnum,NULL);
   }

 if (exit_code>5) // for all serial play
  switch(netstuff->comport)
   {
    case 1:
     strcpy(&str2[0],"/com1");
     break;
    case  2:
     strcpy(&str2[0],"/com2");
     break;
    case  3:
     strcpy(&str2[0],"/com3");
     break;
    case  4:
     strcpy(&str2[0],"/com4");
     break;
    }

 if (exit_code==6) // dial with modem
  {
   if (netstuff->serplayers==1)
    strcpy(&strptr[0],"/player1");
   else
    strcpy(&strptr[0],"/player2");
   sprintf(&dial[0],"%s",&netstuff->dialnum[0]);
   spawnlp(P_OVERLAY,"sergreed.exe","sergreed.exe",strptr,"/dial",&dial[0],str2,"/char",charnum,NULL);
   }
 if (exit_code==7) // answer with modem
  {
   if (netstuff->serplayers==1)
    strcpy(&strptr[0],"/player1");
   else
    strcpy(&strptr[0],"/player2");
   spawnlp(P_OVERLAY,"sergreed.exe","sergreed.exe",strptr,"/answer",str2,"/char",charnum,NULL);
   }
 if (exit_code==8) // serial cable (or direct modem)
  {
   if (netstuff->serplayers==1)
    strcpy(&strptr[0],"/player1");
   else
    strcpy(&strptr[0],"/player2");
   spawnlp(P_OVERLAY,"sergreed.exe","sergreed.exe",strptr,str2,"/char",charnum,NULL);
   }
 }

