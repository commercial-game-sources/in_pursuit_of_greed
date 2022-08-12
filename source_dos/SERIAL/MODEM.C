/***************************************************************************/
/*                                                                         */
/*                                                                         */
/* Serial Communication Driver for Greed                                   */
/* Copyright (C) 1995 by Channel 7                                         */
/*                                                                         */
/* written by Robert Morgan                                                */
/*                                                                         */
/***************************************************************************/

#include <STDIO.H>
#include <BIOS.H>
#include <STRING.H>
#include <DOS.H>
#include "greednet.h"
#include "sergreed.h"


/**** CONSTANTS ****/

#define INITSTR "ATZ\r"
#define CALLSTR "ATDT"
#define HANGSTR "ATH\r"
#define PRINTCONFIGERROR()                                      \
	 printf("MODEM.CFG not found, using defaults\n"         \
		"MODEM.CFG format:\n"                           \
		" Line 1: Modem Init String (ATZ)\n"            \
		" Line 2: Modem Dial String (ATDT or ATDP)\n"   \
		" Line 3: Modem Hangup String (ATH)\n\n");
#define CONNECTERROR                                                       \
	 "The connection MUST be made at %s baud, no error correction, "   \
	 "no compression!\n"                                               \
	 "Check your modem initialization string!"


/**** VARIABLES ****/

boolean modemactive;
char    startup[MAXMODEMSTRLEN];  // modem init string
char    dialstr[MAXMODEMSTRLEN];  // modem dial prefix
char    shutdown[MAXMODEMSTRLEN]; // modem deinit string
char    response[80];             // modem response to commands


/**** FUNCTIONS ****/

void MOD_Command(char *str)
{
 printf("Command : %s\n",str);
 COM_WriteBuffer(str,strlen(str));
 COM_WriteBuffer("\r",1);
 }


void MOD_Response(char *resp)
{
 int c, rp;

 do
  {
   printf("Response: ");
   rp=0;
   do
    {
     while (_bios_keybrd(1))
      {
       if ((_bios_keybrd(0) & 0xff)==27)
	{
	 printf("\n");
	 MS_Error("\nModem response aborted.");
	 }
       }
     c=COM_ReadByte();
     if (c==-1)
      continue;
     if ((c=='\n' && rp>0) || rp==79)
      {
       response[rp]=0;
       printf("%s\n",response);
       break;
       }
     if (c>=' ')
      {
       response[rp]=c;
       rp++;
       }
     } while (1);
   } while (strncmp(response,resp,strlen(resp)));
 }


void MOD_CheckCommand(char *s)
{
 int i;

 i=0;
 while (s[i])
  {
   if (s[i]=='\n')
    s[i]=' ';
   i++;
   }
 }


void MOD_Init(void)
{
 FILE *f;

 f=fopen("MODEM.CFG","r");
 if (f==NULL)
  {
   PRINTCONFIGERROR();
   strcpy(startup,INITSTR);
   strcpy(dialstr,CALLSTR);
   strcpy(shutdown,HANGSTR);
   }
 else if (!fgets(startup,MAXMODEMSTRLEN,f) ||
  !fgets(dialstr,MAXMODEMSTRLEN,f) ||
  !fgets(shutdown,MAXMODEMSTRLEN,f))
  {
   PRINTCONFIGERROR();
   strcpy(startup,INITSTR);
   strcpy(dialstr,CALLSTR);
   strcpy(shutdown,HANGSTR);
   }
 fclose(f);
 printf("Initializing Modem... If modem does not respond,\n"
	"check the initialization string, irq, and port settings.\n");
 MOD_CheckCommand(startup);
 MOD_CheckCommand(dialstr);
 MOD_CheckCommand(shutdown);
 MOD_Command(startup);
 MOD_Response("OK");
 modemactive=true;
 }


void MOD_Dial(void)
{
 char cmd[80];
 int  p;

 MOD_Init();
 p=MS_CheckParm("dial");
 printf("\nDialing %s...\n",ms_argv[p+1]);
 sprintf(cmd,"%s %s\r",dialstr,ms_argv[p+1]);
 MOD_Command(cmd);
 MOD_Response("CONNECT");
 if (strncmp(response+8,MAXSPEEDSTR,strlen(MAXSPEEDSTR)))
 MS_Error(CONNECTERROR,MAXSPEEDSTR);
 greedcom.consoleplayer=1;
 }


void MOD_Answer(void)
{
 MOD_Init();
 printf("\nWaiting for ring...\n");
 MOD_Response("RING");
 MOD_Command("ATA");
 MOD_Response("CONNECT");
 if (strncmp(response+8,MAXSPEEDSTR,strlen(MAXSPEEDSTR)))
 MS_Error(CONNECTERROR,MAXSPEEDSTR);
 greedcom.consoleplayer=0;
 }


