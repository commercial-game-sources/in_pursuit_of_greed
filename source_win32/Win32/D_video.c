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

#include <STRING.H>
#include <STDLIB.H>
#include <CONIO.H>
#include "d_global.h"
#include "d_ints.h"
#include "d_video.h"
#include "d_misc.h"
#include "d_disk.h"
#include "r_public.h"
#include "r_refdef.h"

/**** CONSTANTS ****/

#define CRTCOFF (inbyte(STATUS_REGISTER_1)&1)


/**** VARIABLES ****/

byte *		screen;
byte *		ylookup[SCREENHEIGHT];
byte *		transparency;
byte *		translookup[255];
HBITMAP		Bitmap;
HDC			Memory_DC;
HPALETTE	Palette;

extern SoundCard SC;


/**** FUNCTIONS ****/

void VI_FillPalette(int red, int green, int blue)
{
 }


void VI_SetPalette(byte *palette)
{
	HDC				dc;
	int				i;
	int				j = 0;
	PALETTEENTRY	entries[256];

	for ( i = 0 ; i < 256 ; i++ )
	{
		entries[i].peRed = palette[j++] << 2;
		entries[i].peGreen = palette[j++] << 2;
		entries[i].peBlue = palette[j++] << 2;
		entries[i].peFlags = PC_NOCOLLAPSE;
	}	
	
	dc = GetDC(Window_Handle);

	SetPaletteEntries(Palette,0,256,entries);

	VI_BlitView();

	SelectPalette(Memory_DC,Palette,TRUE);
	SelectPalette(dc,Palette,TRUE);

	RealizePalette(Memory_DC);
	RealizePalette(dc);

	ReleaseDC(Window_Handle,dc);
}


void VI_ResetPalette()
{
	HDC	dc;

	dc = GetDC(Window_Handle);

	RealizePalette(dc);

	ReleaseDC(Window_Handle,dc);
}


void VI_GetPalette(byte *palette)
{
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


void VI_DrawPic(int x,int y,pic_t * pic)
{
	byte *	dest;
	byte *	source;
	int		width;
	int		height;

	width = pic->width;
	height = pic->height;
	source = &pic->data;
	dest = ylookup[y] + x;

	while ( height-- )
	{
		memcpy(dest,source,width);
		dest += SCREENWIDTH;
		source += width;
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
	HDC				dc;
	int				y;
	int				i;
	int				j;
	BITMAPINFO *	bmi;
	LOGPALETTE *	pal;
	byte *			pal_data;

	dc = GetDC(Window_Handle);
	Memory_DC = CreateCompatibleDC(dc);

	bmi = malloc(sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 256);
	memset(bmi,0,sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 256);

	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = SCREENWIDTH;   
	bmi->bmiHeader.biHeight = SCREENHEIGHT;    
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biBitCount = 8;
	bmi->bmiHeader.biCompression = BI_RGB;  

	for ( i = 0 ; i < 256 ; i++ )
	{
		(WORD)*((WORD*)(bmi->bmiColors) + i) = i;
	}

	pal = malloc(sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY));
	memset(pal,0,sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY));

	pal->palVersion = 0x300;
	pal->palNumEntries = 256;

	pal_data = CA_CacheLump(CA_GetNamedNum("palette"));

	for ( i = 0,j = 0 ; i < 256 ; i++ )
	{
		pal->palPalEntry[i].peRed = pal_data[j++] << 2;
		pal->palPalEntry[i].peGreen = pal_data[j++] << 2;
		pal->palPalEntry[i].peBlue = pal_data[j++] << 2;
		pal->palPalEntry[i].peFlags = PC_NOCOLLAPSE;
	}

	Palette = CreatePalette(pal);
	SelectPalette(dc,Palette,TRUE);
	SelectPalette(Memory_DC,Palette,TRUE);
	RealizePalette(dc);
	RealizePalette(Memory_DC);
	
	free(pal);

	Bitmap = CreateDIBSection(
		Memory_DC,
		bmi,
		DIB_PAL_COLORS, 
		&screen,
		NULL,
		0 );

	free(bmi);

	SelectObject(Memory_DC,Bitmap);

	ReleaseDC(Window_Handle,dc);

	if ( screen == NULL )
		MS_Error("VI_Init: Out of memory for screen");

	for ( y = 0 ; y < SCREENHEIGHT ; y++ )
		ylookup[y] = screen + y * SCREENWIDTH;

	transparency=CA_CacheLump(CA_GetNamedNum("TRANSPARENCY"));

	for( y = 0 ; y < 255 ; y++ )
		translookup[y] = transparency + 256 * y;
}


void RF_BlitView(void)
{
	int i = 0;
	int j = SCREENHEIGHT - 1;

	for ( ; i < SCREENHEIGHT ; i++,j-- )
		memcpy(ylookup[i],viewylookup[j],SCREENWIDTH);
}


void VI_BlitView()
{	
	HDC dc;

	dc = GetDC(Window_Handle);
	BitBlt(dc,0,0,SCREENWIDTH,SCREENHEIGHT,Memory_DC,0,0,SRCCOPY);
	ReleaseDC(Window_Handle,dc);
}


void VI_DrawMaskedPicToBuffer(int x,int y,pic_t *pic)
/* Draws a masked pic to the view buffer */
{
	BYTE *	dest;
	BYTE *	source;
	int		width,height,xcor;

	x -= pic->orgx;
	y -= pic->orgy;
	height = pic->height;
	source = &pic->data;
	while (y<0) 
	{
		source += pic->width;
		height--;
	}
	while (height--) 
	{
		if (y<200) 
		{
			dest = viewbuffer + (y * MAX_VIEW_WIDTH + x);     
			xcor = x;
			width = pic->width;
			while (width--) 
			{
				if ( ( xcor >= 0 ) && ( xcor <= 319 ) ) 
				{
					if (*source) 
						*dest = *source;
				}
				xcor++;
				source++;
				dest++;
			}
		}
		y++;
	}
 }
