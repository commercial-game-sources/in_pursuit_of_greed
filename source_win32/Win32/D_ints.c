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

#include <DOS.H>
#include <STRING.H>
#include <CONIO.H>
#include <STDIO.H>
#include <IO.H>
#include <STDLIB.H>
#include "d_global.h"
#include "d_video.h"
#include "d_ints.h"
#include "d_misc.h"
#include "timer.h"
#include "protos.h"
#include "r_refdef.h"
#include "d_disk.h"

/**** CONSTANTS ****/

#define TIMERINT       8
#define KEYBOARDINT    9
#define VBLCOUNTER     16000
#define MOUSEINT       0x33
#define MOUSESENSE     SC.mousesensitivity
#define JOYPORT        0x201
#define MOUSESIZE      16


/**** VARIABLES ****/

void    (*oldkeyboardisr)();
void    (*timerhook)();                 // called every other frame (player)
bool timeractive;
longint timecount;                     // current time index
bool keyboard[NUMCODES];             // keyboard flags
bool pause, capslock, newascii;
bool mouseinstalled, joyinstalled;
int     in_button[NUMBUTTONS];          // frames the button has been down
byte    lastscan;
char    lastascii;

/* mouse data */
short mdx, mdy, b1, b2;
int   hiding=1, busy=1;           /* internal flags */
int   mousex=160, mousey=100;
byte  back[MOUSESIZE*MOUSESIZE];  /* background for mouse */
byte  fore[MOUSESIZE*MOUSESIZE];  /* mouse foreground */


/* joystick data */
int   jx, jy, jdx, jdy, j1, j2;
word  jcenx, jceny, xsense, ysense;

/* config data */
//extern SoundCard SC;


byte ASCIINames[] = // Unshifted ASCII for scan codes
 {
  0  ,27 ,'1','2','3','4','5','6','7','8','9','0','-','=',8  ,9  ,
  'q','w','e','r','t','y','u','i','o','p','[',']',13 ,0  ,'a','s',
  'd','f','g','h','j','k','l',';',39 ,'`',0  ,92 ,'z','x','c','v',
  'b','n','m',',','.','/',0  ,'*',0  ,' ',0  ,0  ,0  ,0  ,0  ,0  ,
  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,'-',0  ,0  ,0  ,'+',0  ,
  0  ,0  ,0  ,127,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,
  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,
  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0
  };

byte ShiftNames[] = // Shifted ASCII for scan codes
 {
  0  ,27 ,'!','@','#','$','%','^','&','*','(',')','_','+',8  ,9  ,
  'Q','W','E','R','T','Y','U','I','O','P','{','}',13 ,0  ,'A','S',
  'D','F','G','H','J','K','L',':',34 ,'~',0  ,'|','Z','X','C','V',
  'B','N','M','<','>','?',0  ,'*',0  ,' ',0  ,0  ,0  ,0  ,0  ,0  ,
  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,'-',0  ,0  ,0  ,'+',0  ,
  0  ,0  ,0  ,127,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,
  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,
  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0
  };

byte SpecialNames[] = // ASCII for 0xe0 prefixed codes
 {
  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,
  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,13 ,0  ,0  ,0  ,
  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,
  0  ,0  ,0  ,0  ,0  ,'/',0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,
  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,
  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,
  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,
  0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0
  };

int scanbuttons[NUMBUTTONS] =
{
 SC_UPARROW,       // bt_north
 SC_RIGHTARROW,    // bt_east
 SC_DOWNARROW,     // bt_south
 SC_LEFTARROW,     // bt_west
 SC_CONTROL,       // bt_fire
 SC_ALT,           // bt_straf
 SC_SPACE,         // bt_use
 SC_LSHIFT,        // bt_run
 SC_Z,             // bt_jump
 SC_X,             // bt_useitem
 SC_A,             // bt_asscam
 SC_PGUP,          // bt_lookup
 SC_PGDN,          // bt_lookdown
 SC_HOME,          // bt_centerview
 SC_COMMA,         // bt_slideleft
 SC_PERIOD,        // bt_slideright
 SC_INSERT,        // bt_invleft
 SC_DELETE,        // bt_invright
 };


/**** FUNCTIONS ****/


void INT_KeyboardISR()
/* keyboard interrupt
    processes make/break codes
    sets key flags accordingly */
{
 /*static bool special;
 byte           k, c, al;

// Get the scan code
 k=inbyte(0x60);

 if (k==0xE0) special=true;
 else if (k==0xE1) pause^=true;
 else
  {
   if (special && (k==0x2A || k==0xAA || k==0xAA || k==0x36))
    {
     special=false;
     goto end;
     }
   if (k & 0x80) // Break code
    {
     k&=0x7F;
     keyboard[k]=false;
     }
   else // Make code
    {
     lastscan=k;
     keyboard[k]=true;
     if (special) c=SpecialNames[k];
     else
      {
       if (k==SC_CAPSLOCK) capslock^=true;
       if (keyboard[SC_LSHIFT] || keyboard[SC_RSHIFT])
	{
	 c=ShiftNames[k];
	 if (capslock && c>='A' && c<='Z') c+='a'-'A';
	 }
       else
	{
	 c=ASCIINames[k];
	 if (capslock && c>='a' && c<='z') c-='a'-'A';
	 }
       }
     if (c)         // return a new ascii character
      {
       lastascii=c;
       newascii=true;
       }
     }
   special=false;
   }
end:
// acknowledge the interrupt
 al=inbyte(0x61);
 al|=0x80;
 outbyte(0x61,al);
 al&=0x7F;
 outbyte(0x61,al);
 outbyte(0x20,0x20);*/
 }


void INT_ReadControls(void)
/* read in input controls */
{
 int i;

 i = MapVirtualKey(SC_A,1);

 for ( i = 0 ; i < 128 ; i++ )
	 keyboard[i] = GetAsyncKeyState(MapVirtualKey(i,1));

 memset(in_button,0,sizeof(in_button));
 for(i=0;i<NUMBUTTONS;i++)
  if (keyboard[scanbuttons[i]])
   in_button[i]=1;

 if (mouseinstalled)
  {
   }
 }

/*============================================================================*/

void INT_TimerISR(void)
/* process each timer tick */
{
	timecount+=2;
	if ( timerhook )
		timerhook();
}


void INT_TimerHook(void(* hook)(void))
{
 timerhook=hook;
 }


/*============================================================================*/

void UpdateMouse(void)
{
 }


int MouseGetClick(short *x,short *y)
{
 return 0;
 }

void ResetMouse(void)
{
 }


void M_Init(void)
{
   mouseinstalled=false;
   printf("Mouse Not Found\n");
   return;
 }


void M_Shutdown(void)
{
 }


/***************************************************************************/

void INT_Setup(void)
{
	memset(keyboard,0,sizeof(keyboard));
	M_Init();
	dStartTimer(INT_TimerISR,1000/35);
	timeractive = true;
}


void INT_ShutdownKeyboard(void)
{
	INT_TimerHook(NULL);
}


void INT_Shutdown(void)
{
 if ( timeractive ) 
	 dStopTimer();
 if ( mouseinstalled ) 
	 M_Shutdown();
}


void MouseHide()
{
}

void MouseShow()
{
}