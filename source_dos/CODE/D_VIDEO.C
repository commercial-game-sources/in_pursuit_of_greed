/***************************************************************************/
/*                                                                         */
/*                                                                         */
/* Raven 3D Engine                                                         */
/* Copyright (C) 1996 by Softdisk Publishing                               */
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

#include <BIOS.H>
#include <I86.H>
#include <STRING.H>
#include <MALLOC.H>
#include <STDLIB.H>
#include <CONIO.H>
#include "d_global.h"
#include "d_ints.h"
#include "d_video.h"
#include "d_misc.h"
#include "d_disk.h"
#include "r_public.h"
#include "r_refdef.h"
#include "audio.h"


/**** CONSTANTS ****/

#define CRTCOFF (inbyte(STATUS_REGISTER_1)&1)


/**** VARIABLES ****/

byte *screen=(byte *)SCREEN;
byte *ylookup[SCREENHEIGHT];
byte *transparency;
byte *translookup[255];

extern SoundCard SC;


/**** FUNCTIONS ****/

void VI_SetTextMode(void)
{
 union REGS r;

 r.w.ax = 3;
 int386(0x10,(const union REGS *)&r,&r);
 }


void VI_SetVGAMode(void)
{
 union REGS r;

 r.w.ax = 0x13;
 int386(0x10,(const union REGS *)&r,&r);
 }


void VI_WaitVBL(int vbls)
{
 while (vbls--)
  {
   while (inp(0x3da) & 0x08) ;
   while (!(inp(0x3da) & 0x08)) ;
   while (!(inp(0x3da) & 0x01)) ;
   }
 }


void VI_FillPalette(int red, int green, int blue)
{
 int i;

 outbyte(PEL_WRITE_ADR,0);
 for (i=0;i<256;i++)
  {
   outbyte(PEL_DATA,red);
   outbyte(PEL_DATA,green);
   outbyte(PEL_DATA,blue);
   }
 }


void VI_SetColor(int color,int red,int green,int blue)
{
 outbyte(PEL_WRITE_ADR,color);
 outbyte(PEL_DATA,red);
 outbyte(PEL_DATA,green);
 outbyte(PEL_DATA,blue);
 }


void VI_GetColor(int color,int *red,int *green,int *blue)
{
 outbyte(PEL_READ_ADR,color);
 *red=inbyte(PEL_DATA);
 *green=inbyte(PEL_DATA);
 *blue=inbyte(PEL_DATA);
 }


void VI_SetPalette(byte *palette)
{
 int i;

 while (inp(0x3da) & 0x08) ;
 while (!(inp(0x3da) & 0x08)) ;
 while (!(inp(0x3da) & 0x01)) ;
 outbyte(PEL_WRITE_ADR,0);
 for (i=0;i<768;i++)
  outbyte(PEL_DATA,*palette++);
 }


void VI_GetPalette(byte *palette)
{
 int i;

 outbyte (PEL_READ_ADR,0);
 for (i=0;i<768;i++)
  *palette++=inbyte(PEL_DATA);
 }


void VI_FadeOut(int start,int end,int red,int green,int blue,int steps)
{
 byte        basep[768];
 signed char px[768], pdx[768], dx[768];
 int         i, j;

 VI_GetPalette(basep);
 memset(dx,0,768);
 for(j=start;j<end;j++)
  {
   pdx[j*3]=(basep[j*3]-red)%steps;
   px[j*3]=(basep[j*3]-red)/steps;
   pdx[j*3+1]=(basep[j*3+1]-green)%steps;
   px[j*3+1]=(basep[j*3+1]-green)/steps;
   pdx[j*3+2]=(basep[j*3+2]-blue)%steps;
   px[j*3+2]=(basep[j*3+2]-blue)/steps;
   }
 start*=3;
 end*=3;
 for (i=0;i<steps;i++)
  {
   for (j=start;j<end;j++)
    {
     basep[j]-=px[j];
     dx[j]+=pdx[j];
     if (dx[j]>=steps)
      {
       dx[j]-=steps;
       --basep[j];
       }
     else if (dx[j]<=-steps)
      {
       dx[j]+=steps;
       ++basep[j];
       }
     }
   Wait(1);
   VI_SetPalette(basep);
   }
 VI_FillPalette(red,green,blue);
 }


void VI_FadeIn(int start,int end,byte *palette,int steps)
{
 byte        basep[768], work[768];
 signed char px[768], pdx[768], dx[768];
 int         i, j;

 VI_GetPalette(basep);
 memset(dx,0,768);
 memset(work,0,768);
 start*=3;
 end*=3;
 for(j=start;j<end;j++)
  {
   pdx[j]=(palette[j]-basep[j])%steps;
   px[j]=(palette[j]-basep[j])/steps;
   }
 for (i=0;i<steps;i++)
  {
   for (j=start;j<end;j++)
    {
     work[j]+=px[j];
     dx[j]+=pdx[j];
     if (dx[j]>=steps)
      {
       dx[j]-=steps;
       ++work[j];
       }
     else if (dx[j]<=-steps)
      {
       dx[j]+=steps;
       --work[j];
       }
     }
   Wait(1);
   VI_SetPalette(work);
   }
 VI_SetPalette(palette);
 }


void VI_ColorBorder(int color)
{
 if (SC.vrhelmet==1)
  return;
 inbyte(STATUS_REGISTER_1);
 outbyte(ATR_INDEX,ATR_OVERSCAN);
 outbyte(ATR_INDEX,color);
 outbyte(ATR_INDEX,0x20);
 }


void VI_Plot(int x,int y,int color)
{
 *(ylookup[y]+x)=color;
 }


void VI_Hlin(int x,int y,int width,int color)
{
 memset(ylookup[y]+x,color,(size_t)width);
 }


void VI_Vlin(int x,int y,int height,int color)
{
 byte *dest;

 dest=ylookup[y]+x;
 while (height--)
  {
   *dest=color;
   dest+=SCREENWIDTH;
   }
 }


void VI_Bar(int x,int y,int width,int height,int color)
{
 byte *dest;

 dest=ylookup[y]+x;
 while (height--)
  {
   memset(dest,color,(size_t)width);
   dest+=SCREENWIDTH;
   }
 }


void VI_DrawPic(int x,int y,pic_t *pic)
{
 byte *dest, *source;
 int  width, height;

 width=pic->width;
 height=pic->height;
 source=&pic->data;
 dest=ylookup[y]+x;
 while (height--)
  {
   memcpy(dest,source,width);
   dest+=SCREENWIDTH;
   source+=width;
   }
 }


void VI_DrawMaskedPic(int x, int y, pic_t  *pic)
/* Draws a formatted image to the screen, masked with zero */
{
 byte *dest, *source;
 int  width, height, xcor;

 x -= pic->orgx;
 y -= pic->orgy;
 height=pic->height;
 source=&pic->data;
 while (y<0)
  {
   source+=pic->width;
   height--;
   y++;
   }
 while (height--)
  {
   if (y<200)
    {
     dest=ylookup[y]+x;
     xcor=x;
     width=pic->width;
     while (width--)
      {
       if (xcor>=0 && xcor<=319 && *source) *dest=*source;
       xcor++;
       source++;
       dest++;
       }
     }
   y++;
   }
 }


void VI_DrawTransPicToBuffer(int x,int y,pic_t *pic)
/* Draws a transpartent, masked pic to the view buffer */
{
 byte *dest,*source;
 int  width,height;

 x -= pic->orgx;
 y -= pic->orgy;
 height=pic->height;
 source=&pic->data;
 while (y<0)
  {
   source+=pic->width;
   height--;
   y++;
   }
 while (height-->0)
  {
   if (y<200)
    {
     dest=viewylookup[y]+x;
     width=pic->width;
     while (width--)
      {
       if (*source) *dest=*(translookup[*source-1]+*dest);
       source++;
       dest++;
       }
     }
   y++;
   }
 }


void VI_DrawMaskedPicToBuffer2(int x,int y,pic_t *pic)
/* Draws a masked pic to the view buffer */
{
 byte *dest, *source, *colormap;
 int  width, height, maplight;

// x-=pic->orgx;
// y-=pic->orgy;
 height=pic->height;
 source=&pic->data;

 wallshadow=mapeffects[player.mapspot];
 if (wallshadow==0)
  {
   maplight=((int)maplights[player.mapspot]<<3)+reallight[player.mapspot];
   if (maplight<0) colormap=zcolormap[0];
    else if (maplight>MAXZLIGHT) colormap=zcolormap[MAXZLIGHT];
    else colormap=zcolormap[maplight];
   }
 else if (wallshadow==1) colormap=colormaps+(wallglow<<8);
 else if (wallshadow==2) colormap=colormaps+(wallflicker1<<8);
 else if (wallshadow==3) colormap=colormaps+(wallflicker2<<8);
 else if (wallshadow==4) colormap=colormaps+(wallflicker3<<8);
 else if (wallshadow>=5 && wallshadow<=8)
  {
   if (wallcycle==wallshadow-5) colormap=colormaps;
   else
    {
     maplight=((int)maplights[player.mapspot]<<3)+reallight[player.mapspot];
     if (maplight<0) colormap=zcolormap[0];
      else if (maplight>MAXZLIGHT) colormap=zcolormap[MAXZLIGHT];
      else colormap=zcolormap[maplight];
     }
   }
 else if (wallshadow==9)
  {
   maplight=((int)maplights[player.mapspot]<<3)+reallight[player.mapspot]+wallflicker4;
   if (maplight<0) colormap=zcolormap[0];
    else if (maplight>MAXZLIGHT) colormap=zcolormap[MAXZLIGHT];
    else colormap=zcolormap[maplight];
   }
 if (height+y>windowHeight)
  height=windowHeight-y;
 while (height-->0)
  {
   dest=viewylookup[y]+x;
   width=pic->width;
   while (width--)
    {
     if (*source) *dest=*(colormap + *(source));
     source++;
     dest++;
     }
   y++;
   }
 }


void VI_Init(int specialbuffer)
{
 int y;

 if (specialbuffer)
  {
   screen=(byte *)malloc(64000);
   if (screen==NULL)
    MS_Error("VI_Init: Out of memory for vrscreen");
   }
 else
  screen=(byte *)SCREEN;
 for (y=0;y<SCREENHEIGHT;y++)
  ylookup[y]=screen + y*SCREENWIDTH;
 transparency=CA_CacheLump(CA_GetNamedNum("TRANSPARENCY"));
 for(y=0;y<255;y++)
  translookup[y]=transparency+256*y;
 VI_SetVGAMode();
 }

