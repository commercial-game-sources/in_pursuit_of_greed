// Copyright 1995 by Robert Morgan of Channel 7
// CPR file format loader


#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <alloc.h>
#include <string.h>


/**** CONSTANTS ****/

#define VERSION "1.000"

enum ERRORS
 {
  ERR_OPEN,
  ERR_MOUSE,
  ERR_SOUND,
  ERR_FILE,
  ERR_PROG,
  ERR_BLO,
  ERR_MEM,
  ERR_UNKNOWN
  };


/**** TYPES ****/

typedef char byte;

typedef unsigned int word;

typedef byte screentype[200][320];

typedef screentype far *pscreentype;

typedef byte paltype[768];

typedef struct
 {
  int signature;
  byte version;
  int  width, height;
  byte flags, headersize;
  byte indicator;
  int  orgx, orgy;
  } CPR_HEADER;


/**** VARIABLES ****/

 char far *errormsgs[7]=
  {
   "File Error","Mouse Error","Sound Error",
   "Fatal File Error","Program Error","BLO File Error",
   "Memory Error"
   };

 static char far *detailmsgs[8]=
  {
   /* ERR_OPEN */
   "File does not exist or DOS was unable to open this file.\n"
   " Please test your hard drive for errors or missing files.",
   /* ERR_MOUSE */
   "Mouse is not found or the driver is not installed.  Please\n"
   " install a MS compatible mouse and driver.",
   /* ERR_SOUND */
   "Sound card not found, module corrupted, or other sound card\n"
   " failure.  Make sure that the sound card is installed\n"
   " correctly and is supported by this software.",
   /* ERR_FILE */
   "File read or write error.  Your hard drive may be full or\n"
   " errors may exist on your drive.  Please test and retry.",
   /* ERR_PROG */
   "A logical error has occured in the code.  This trap is for\n"
   " debugging the code.",
   /* ERR_BLO */
   "A file that should exist in the BLO file is not found.\n"
   " The BLO file version might be incorrect.",
   /* ERR_MEM */
   "A memory error has been detected.  If insufficient memory\n"
   " is found, please increase convential memory by a few more\n"
   " kilobytes.",
   /* ERR_UNKNOWN */
   "Unknown Error!"
   };

 paltype colors;


/**** FUNCTIONS ****/

void errorhandler(char *s,int code,int line,char *sourcefile)
/* handle all errors and output error messages
    outputs programmer's error message (s)
    outputs file and line number of calling routine
    gets dos error message
    displays more info about error code
    tech support message
    debug info
   saves all msgs to error log file
   display error log */
{
 FILE    *f;
 char    str1[85], *errmsg;
 int     olderror=errno;

 _AX = 3;
 geninterrupt(0x10);
 f=fopen("ERROR.LOG","w+");
 if (f==NULL)
  {
   printf("Unable to Create Error Log!\n\n");
   exit(4);
   }
 fprintf(f,"%s: %s\n\n",errormsgs[code],s);
 fprintf(f,"DOS's Opinion: ");
 if (olderror==0) fprintf(f,"No Comment");
  else
   {
    errmsg=strerror(olderror);
    fputs(errmsg,f);
    }
 fprintf(f,"\n\n");
 fprintf(f,"  Line: %i   Module: %s   Version: %s, %s\n\n",line,sourcefile,VERSION,__DATE__);
 if (code<7) fputs(detailmsgs[code],f);
  else fputs(detailmsgs[6],f);
 fprintf(f,"\n\nIf the problem persists, please contact technical support.\n\n");
 fprintf(f,"Core Free     : %lu bytes\n",coreleft());
 rewind(f);
 do
  {
   errmsg=fgets(str1,80,f);
   if (errmsg!=NULL) printf("%s",str1);
   } while (errmsg!=NULL);
 fclose(f);
 exit(4);
 }


void set256colors(paltype pal)
/* set all 256 colors */
{
 asm {
   push ds
   xor di, di
   lds si, pal
   mov dx, 3DAh
   }
wait48:
 asm {
   in ax, dx
   test ax, 8
   jz wait48
   }
wait40:
 asm {
   in ax, dx
   test ax, 8
   jnz wait40                /* wait for video refresh */
   mov dx, 3C8h
   xor ax, ax
   out dx, al
   inc dx
   mov cx, 256
   }
loop:
 asm {
   outsb
   outsb
   outsb
   loop loop                 /* set colors */
   pop ds
   }
 }


void LoadCPR(char *s)
/* load image file from blo file
    gets the file offset in BLO file
    seeks to that offset
    loads CPR header
    tests CPR header for a real CPR file
    uncompresses CPR file */
{
 int         x, y, i, data, count, orgx, orgy, indicator;
 char        str1[64];
 FILE        *f;
 CPR_HEADER  h;
 pscreentype ts=(pscreentype)MK_FP(0xA000,0x0000);
 byte far    *t=(byte *)ts;

 sprintf(str1,"%s.CPR",s);
 f=fopen(str1,"rb");
 if (f==NULL) errorhandler(str1,ERR_OPEN,__LINE__,__FILE__);
 if (!fread(&h,sizeof(CPR_HEADER),1,f) ||
  h.signature!=19794 || fseek(f,h.headersize,0) ||
  ((h.flags & 1) && !fread(colors,768,1,f))) errorhandler(str1,ERR_FILE,__LINE__,__FILE__);
 set256colors(colors);
 if (h.version==4)
  {
   indicator=255;
   orgx=0;
   orgy=0;
   }
 else if (h.version==5)
  {
   indicator=h.indicator;
   orgx=h.orgx;
   orgy=h.orgy;
   memset(ts,0,sizeof(screentype));
   }
 else errorhandler("Invalid CPR version",ERR_PROG,__LINE__,__FILE__);
 y=orgy;                      /* jump to origin of image */
 x=orgx;
 t=&(*ts)[y][x];
 while (y<h.height)
  {
   data=fgetc(f);
   if (data==indicator)
    {
     count=fgetc(f);
     data=fgetc(f);
     for(i=0;i<count;i++)
      {
       *t++=data;
       ++x;
       if (x==h.width)
	{
	 x=orgx;
	 ++y;
	 t=&(*ts)[y][x];
	 }
       }
     }
   else if (data>=0)
    {
     *t++=data;
     ++x;
     if (x==h.width)
      {
       x=orgx;
       ++y;
       t=&(*ts)[y][x];
       }
     }
   else errorhandler(str1,ERR_FILE,__LINE__,__FILE__);
   }
 fclose(f);
 }