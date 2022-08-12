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
#include <I86.H>
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
#include "audio.h"
#include "protos.h"
#include "r_refdef.h"
#include "d_disk.h"
#include "svrdos4g.h"


/**** CONSTANTS ****/

#define TIMERINT       8
#define KEYBOARDINT    9
#define VBLCOUNTER     16000
#define MOUSEINT       0x33
#define MOUSESENSE     SC.mousesensitivity
#define JOYPORT        0x201
#define MOUSESIZE      16


/**** VARIABLES ****/

void    (interrupt *oldkeyboardisr)();
void    (*timerhook)();                 // called every other frame (player)
boolean timeractive;
longint timecount;                     // current time index
boolean keyboard[NUMCODES];             // keyboard flags
boolean pause, capslock, newascii;
boolean mouseinstalled, joyinstalled;
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
extern SoundCard SC;


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


void interrupt INT_KeyboardISR()
/* keyboard interrupt
    processes make/break codes
    sets key flags accordingly */
{
 static boolean special;
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
 outbyte(0x20,0x20);
 }


void INT_ReadControls(void)
/* read in input controls */
{
 int i, c, k, jx, jy;

 memset(in_button,0,sizeof(in_button));
 for(i=0;i<NUMBUTTONS;i++)
  if (keyboard[scanbuttons[i]])
   in_button[i]=1;

 if (mouseinstalled)
  {
 if (mdx>=MOUSESENSE)
    {
     in_button[bt_east]=2;
     mdx-=MOUSESENSE;
     }
   else if (mdx<=-MOUSESENSE)
    {
     in_button[bt_west]=2;
     mdx+=MOUSESENSE;
     }
   if (mdy>=MOUSESENSE)
    {
     in_button[bt_south]=2;
     mdy-=MOUSESENSE;
     }
   else if (mdy<=-MOUSESENSE)
    {
     in_button[bt_north]=2;
     mdy+=MOUSESENSE;
     }
/*   if (mdx>0)
    in_button[bt_east]=2;
   else if (mdx<0)
    in_button[bt_west]=2;
   if (mdy>0)
    in_button[bt_south]=2;
   else if (mdx<0)
    in_button[bt_north]=2;
   mdx=0;
   mdy=0;  */

   if (b1) in_button[SC.leftbutton]=2;
   if (b2) in_button[SC.rightbutton]=2;
   }

 if (joyinstalled)
  {
   _disable();
   outp(JOYPORT,0xff);
   c=inp(JOYPORT);
   j1=((c&16)>>4)^1;
   j2=((c&32)>>5)^1;
   for (k=0;c&3;k++)
    {
     if (c&1) jx=k;
     if (c&2) jy=k;
     c=inp(JOYPORT);
     }
   _enable();
   jx-=jcenx;
   jy-=jceny;
   if (jx>xsense)
    jx-=xsense;  // remove trash results
   else if (jx<-xsense)
    jx+=xsense;
   else
    jx=0;
   if (jy>ysense)
    jy-=ysense;
   else if (jy<-ysense)
    jy+=ysense;
   else
    jy=0;
   if (jx>0)
    in_button[bt_east]=3;
   else if (jx<0)
    in_button[bt_west]=3;
   if (jy>0)
    in_button[bt_south]=3;
   else if (jy<0)
    in_button[bt_north]=3;
   if (j1)
    in_button[SC.joybut1]=3;
   if (j2)
    in_button[SC.joybut2]=3;
   }
 }

/*============================================================================*/

void INT_TimerISR(void)
/* process each timer tick */
{
 timecount++;  // increment timing variables
 if (MusicPresent) dPoll();    // have to poll a lot!
 if ((timecount&1) && timerhook) timerhook();
 if (MusicPresent) dPoll();    // have to poll a lot!
 }


void INT_TimerHook(void(* hook)(void))
{
 timerhook=hook;
 }


void KBstub()
{
 }

/*============================================================================*/

void J_GetJoyInfo(word *x,word *y)
{
 byte c;
 int  k;

 _disable();
 outp(JOYPORT,0xff);
 c=inp(JOYPORT);
 j1=((c&16)>>4)^1;
 j2=((c&32)>>5)^1;
 for (k=0;c&3;k++)
  {
   if (c&1) *x=k;
   if (c&2) *y=k;
   c=inp(JOYPORT);
   }
 _enable();
 }


void J_GetButtons(void)
{
 int b;

 outp(JOYPORT,0);
 b=-inp(JOYPORT);
 j1=b&0x10;
 j2=b&0x20;
 }


boolean J_Detect(void)
{
 union REGS r;

 r.h.ah=0x84;
 r.w.dx=0x01;
 int386(0x15,&r,&r);
 if (r.w.cflag || r.w.ax==0 || r.w.bx==0) return false;
 return true;
 }


void J_Calibrate(void)
{
 word  x1, y1, x2, y2, a, b;

 xsense=SC.xsense;
 ysense=SC.ysense;
 jcenx=SC.jcenx;
 jceny=SC.jceny;

 printf("\tMove joystick to upper-left and press a button (ESC=continue)");
 newascii=false;
 do
  {
   J_GetJoyInfo(&x1,&y1);
   if (newascii && lastascii==27)
    {
     newascii=false;
     printf("\n");
     return;
     }
   } while (!j1 && !j2);
 printf("\n");
 do
  {
   J_GetJoyInfo(&a,&b);  // wait for button release
   if (newascii && lastascii==27)
    {
     newascii=false;
     printf("\n");
     return;
     }
   } while (j1 || j2);
 printf("\tMove joystick to lower-right and press a button");
 do
  {
   J_GetJoyInfo(&x2,&y2);
   if (newascii && lastascii==27)
    {
     newascii=false;
     printf("\n");
     return;
     }
   } while (!j1 && !j2);
 printf("\n");
 do
  {
   J_GetJoyInfo(&a,&b);  // wait for button release
   if (newascii && lastascii==27)
    {
     newascii=false;
     printf("\n");
     return;
     }
   } while (j1 || j2);
 printf("\tCenter the joystick and press a button");
 do
  {
   J_GetJoyInfo(&jcenx,&jceny);
   if (newascii && lastascii==27)
    {
     newascii=false;
     printf("\n");
     return;
     }
   } while (!j1 && !j2);
 printf("\n");
 xsense=(y2-y1)/7;
 ysense=(x2-x1)/7;
 SC.xsense=xsense;
 SC.ysense=ysense;
 SC.jcenx=jcenx;
 SC.jceny=jceny;
 }


void J_Init(void)
{
 if (!SC.joystick) return;
 printf("Joystick: ");
 if (!J_Detect())
  {
   printf("Joystick Not Found\n");
   return;
   }
 joyinstalled=true;
 printf("Found\n");
 J_Calibrate();
 }


/*============================================================================*/

void UpdateMouse(void)
{
 union REGS r;
 static int count=0;

 if (mouseinstalled)
  {
   r.w.ax=0x0B;
   int386(MOUSEINT,&r,&r);

   if (r.w.cx==0 && r.w.dx==0)
    {
     ++count;
     if (count==45)
      {
       mdx=0;
       mdy=0;
       }
     }
   else
    count=0;
   mdx+=r.w.cx;
   mdy+=r.w.dx;
   r.w.ax=0x03;
   r.w.bx=7;
   int386(MOUSEINT,&r,&r);
   b1=r.w.bx&1;
   b2=r.w.bx&2;
   }
 }


int MouseGetClick(short *x,short *y)
{
 union REGS r;

 if (!mouseinstalled) return false;
 r.w.ax=0x06;
 r.w.bx=0;
 int386(MOUSEINT,&r,&r);
 *x=r.w.cx>>1;
 *y=r.w.dx;
 return r.w.bx&1;
 }


void _loadds far MouseMove(int x,int y)
#pragma aux MouseMove parm [ecx] [edx]
{
 int  i, j, w, h, x1, y1;
 byte *bp, *tg;

 CLI;
 x1=x>>1;
 y1=y;
 if (busy)
  {
   STI;
   return;
   }
 if (hiding!=0)
  {
   mousex=x1;
   mousey=y1;
   STI;
   return;
   }
 busy=1;
 STI;
 w=MOUSESIZE;
 h=MOUSESIZE;
 if (mousex>319-MOUSESIZE) w=320-mousex;
 if (mousey>199-MOUSESIZE) h=200-mousey;
 for(i=0;i<h;i++)
  {
   bp=&back[i*16];
   tg=ylookup[mousey+i]+mousex;
   for(j=0;j<w;j++,bp++,tg++)
    *tg=*bp;
   }
 mousex=x1;
 mousey=y1;
 w=MOUSESIZE;
 h=MOUSESIZE;
 if (mousex>319-MOUSESIZE) w=320-mousex;
 if (mousey>199-MOUSESIZE) h=200-mousey;
 for(i=0;i<h;i++)
  {
   bp=&back[i*16];
   tg=ylookup[mousey+i]+mousex;
   for(j=0;j<w;j++,bp++,tg++)
    *bp=*tg;
   }
 for(i=0;i<h;i++)
  {
   bp=&fore[i*16];
   tg=ylookup[mousey+i]+mousex;
   for(j=0;j<w;j++,bp++,tg++)
    if (*bp!=255)
     *tg=*bp;
   }
 busy=0;
 }


void ResetMouse(void)
{
 union REGS r;

 if (!mouseinstalled) return;
 mdx=0;
 mdy=0;
 b1=0;
 b2=0;
 r.w.ax=0x0B;
 int386(MOUSEINT,&r,&r);
 }


void M_Init(void)
{
 union REGS   r;
 struct SREGS s;
 int lump;

 if (!SC.mouse) return;
 printf("Mouse: ");
 segread(&s);
 r.w.ax=0x00;
 int386(MOUSEINT,&r,&r);
 if (r.w.ax==0xFFFF)
  {
   mouseinstalled=true;
   printf("Found\n");
   }
 else
  {
   mouseinstalled=false;
   printf("Mouse Not Found\n");
   return;
   }
 busy=0;
 r.w.ax=0x600;               /* DPMI Lock Linear Region */
 r.w.bx=FP_SEG(MouseMove);   /* Linear address in BX:CX */
 r.w.cx=FP_OFF(MouseMove);
 r.w.si=0;                   /* Length in SI:DI */
 r.w.di=512;
 int386(0x31,&r,&r);
 r.x.ecx=1;
 s.es=FP_SEG(MouseMove);
 s.ds=0;
 r.x.edx=FP_OFF(MouseMove);
 r.x.eax=0x0C;
 int386x(MOUSEINT,&r,&r,&s);
 lock_region(fore,256);
 lock_region(back,256);
 lock_region(ylookup,sizeof(ylookup));
 lump=CA_GetNamedNum("MCURSOR");
 lseek(cachehandle,infotable[lump].filepos+8,SEEK_SET);
 read(cachehandle,fore,MOUSESIZE*MOUSESIZE);
 }


void M_Shutdown(void)
{
 union REGS r;

 r.w.ax=0x00;
 int386(MOUSEINT,&r,&r);
 }


/***************************************************************************/

void lock_region(void *address, unsigned length)
{
 union REGS regs;
 unsigned linear;

 linear=(unsigned) address;
 regs.w.ax=0x600;                   /* DPMI Lock Linear Region */
 regs.w.bx=(linear >> 16);          /* Linear address in BX:CX */
 regs.w.cx=(linear & 0xFFFF);
 regs.w.si=(length >> 16);          /* Length in SI:DI */
 regs.w.di=(length & 0xFFFF);
 int386(0x31,&regs,&regs);
 }


void unlock_region(void *address, unsigned length)
{
 union REGS regs;
 unsigned   linear;

 linear=(unsigned) address;
 regs.w.ax=0x601;                   /* DPMI UnLock Linear Region */
 regs.w.bx=(linear >> 16);          /* Linear address in BX:CX */
 regs.w.cx=(linear & 0xFFFF);
 regs.w.si=(length >> 16);          /* Length in SI:DI */
 regs.w.di=(length & 0xFFFF);
 int386(0x31,&regs,&regs);
 }


void timerstub1(void);
void timerstub2(void);

void INT_Setup(void)
{
 oldkeyboardisr = _dos_getvect(KEYBOARDINT);
 lock_region(keyboard,sizeof(keyboard));
 lock_region(in_button,sizeof(in_button));
 lock_region(scanbuttons,sizeof(scanbuttons));
 lock_region(&j1,512);        // check greed.map (should be first var)
 lock_region(ASCIINames,sizeof(ASCIINames));
 lock_region(ShiftNames,sizeof(ShiftNames));
 lock_region(SpecialNames,sizeof(SpecialNames));
 lock_region((void near *)INT_KeyboardISR,(char *)KBstub - (char near *)INT_KeyboardISR);
 lock_region((void near *)timerstub1,(char *)timerstub2 - (char near *)timerstub1);
 _dos_setvect(KEYBOARDINT,INT_KeyboardISR);
 memset(keyboard,0,sizeof(keyboard));
 M_Init();
 dInitTimer();
 dStartTimer(INT_TimerISR,TICKS(70));
 timeractive=true;
 J_Init();
 }


void INT_ShutdownKeyboard(void)
{
 if (oldkeyboardisr) _dos_setvect(KEYBOARDINT,oldkeyboardisr);
 INT_TimerHook(NULL);
 }


void INT_Shutdown(void)
{
 if (oldkeyboardisr) _dos_setvect(KEYBOARDINT,oldkeyboardisr);
 if (timeractive) dDoneTimer();
 if (mouseinstalled) M_Shutdown();
 }
