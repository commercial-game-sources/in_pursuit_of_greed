#include <string.h>
#include <stdio.h>
#include <dir.h>
#include <dos.h>
#include <mem.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <io.h>
#include <bios.h>
#include <stdlib.h>


#define RATIO1 1  //7
#define RATIO2 1  //4
#define TOTALR (RATIO1+RATIO2)

typedef unsigned char byte;
unsigned NUMLIGHTS;

byte    palette[768];
byte    far lightpalette[255][256];


void    RF_BuildLights (void)
{
 int      i,l,c,test,table;
 int      red,green,blue;
 int      dist,bestdist,rdist,gdist,bdist,bestcolor;
 byte     *palptr, *palsrc;
 byte far *screen;

 screen = MK_FP(0xa000,0);

 for(l=0;l<256;l++)
  for(c=0;c<200;c++) *(screen+c*320+l)=l;

 for (l=1;l<256;l++)
  {
   palsrc = palette;
   for (c=0;c<256;c++)
    {
     red = (palsrc[c*3]*RATIO1+palsrc[l*3]*RATIO2)/TOTALR;
     green = (palsrc[c*3+1]*RATIO1+palsrc[l*3+1]*RATIO2)/TOTALR;
     blue = (palsrc[c*3+2]*RATIO1+palsrc[l*3+2]*RATIO2)/TOTALR;
     palptr = palette;
     bestdist = 32000;
     for (test=0;test<256;test++)
      {
       rdist = (int)*palptr++ - red;
       gdist = (int)*palptr++ - green;
       bdist = (int)*palptr++ - blue;
       dist = rdist*rdist + gdist*gdist + bdist*bdist;
       if (dist<bestdist)
	{
	 bestdist=dist;
	 bestcolor=test;
	 }
       }
     lightpalette[l-1][c] = bestcolor;
     }
  _fmemcpy (screen,lightpalette[l-1],256);
  screen+=320;
  }
 }


void extern LoadCPR(char *filename);


int cdecl main(int argc,char **argv)
{
 int      i,j,loop,handle;
 unsigned writen;

 if (argc != 3)
  {
   printf ("lights2 picture.cpr lighttables\n");
   return 1;
   }

 _AX=0x13;
 geninterrupt(0x10);

 LoadCPR(argv[1]);

 _ES = FP_SEG(palette);
 _DX = FP_OFF(palette);
 _BX = 0;
 _CX = 0x100;
 _AX = 0x1017;
 geninterrupt(0x10);                     // get default palette

// write palette
 if ( (handle = open("palette.dat", O_BINARY | O_WRONLY | O_CREAT |
  O_TRUNC,S_IREAD | S_IWRITE) ) == -1)
  {
   perror ("Error opening output palette.dat");
   return 1;
   }
 write (handle,palette,sizeof(palette));
 close(handle);

// write light sources
 NUMLIGHTS = atoi (argv[2]);
 RF_BuildLights ();

 if ( (handle = open("lights.dat", O_BINARY | O_WRONLY | O_CREAT |
  O_TRUNC,S_IREAD | S_IWRITE) ) == -1)
  {
   perror ("Error opening output lights.dat");
   return 1;
   }

 _dos_write (handle,&lightpalette[0][0],255*256,&writen);
 close(handle);
 bioskey (0);
 _AX = 3;
 geninterrupt(0x10);
 return 0;
 }


