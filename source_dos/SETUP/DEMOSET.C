#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <i86.h>
#include <process.h>
#include <graph.h>
#include "audio.h"
#include "import.h"
#include "setup.h"

/**** CONSTANTS ****/

//#define DEMO

#define NUMDRIVERS          (sizeof(DriversTable)/sizeof(DRIVER))
#define bt_north            0
#define bt_east             1
#define bt_south            2
#define bt_west             3
#define bt_fire             4
#define bt_slide            5
#define bt_use              6
#define bt_run              7
#define bt_jump             8
#define bt_useitem          9
#define bt_asscam           10
#define bt_lookup           11
#define bt_lookdown         12
#define bt_centerview       13
#define bt_slideleft        14
#define bt_slideright       15
#define bt_invleft          16
#define bt_invright         17
#define MENU_SELECTCARD     0
#define MENU_TESTSOUND      1
#define MENU_SETKEYS        2
#define MENU_NETWORK        5
#define MENU_SERSET         6
#define MENU_JSTICK         3
#define MENU_MOUSE          4
#define MENU_SAVEEXIT       7
#define MENU_SAVEEXITRUN    8
#define TITLELEN            32
#define MAXITEMS            16
#define ITEMLEN             32
#define HELPLEN             60
#define KB_ESC              1
#define KB_ENTER            28
#define KB_UP               72
#define KB_DOWN             80
#define TITLECOLOR          0x1e // 0x3b
#define FRAMECOLOR          0x1e // 0x1b
#define COMMANDCOLOR        0x1e // 0x1a
#define MENUTITLECOLOR      0x1e // 0x71

/**** VARIABLES ****/

netargs *netstuff;
char    scode=0;
char    playernum;
short   socketnum;

extern void kisr(void);
#pragma aux kisr "_*" parm [];

typedef unsigned int  WORD;
typedef unsigned long DWORD;

union REGS   r;
struct SREGS sr;
void far     *fh;
WORD         orig_pm_sel;
DWORD        orig_pm_off;
WORD         orig_rm_seg;
WORD         orig_rm_off;
char         dialstr[13], netname[13];

char *keytable[]=
 {
  " ","ESC","1","2","3","4","5","6","7","8","9","0",
  "-","=","BKSPACE","TAB","Q","W","E","R","T","Y","U",
  "I","O","P","[","]","ENTER","CTRL","A","S","D","F",
  "G","H","J","K","L",";","'","`","LSHIFT","\\","Z","X",
  "C","V","B","N","M",",",".","/","RSHIFT","*","ALT",
  "SPACE","CAPSLK","F1","F2","F3","F4","F5","F6","F7",
  "F8","F9","F10","NUMLK","SCRLK","HOME","UP","PGUP",
  "-","LEFT","CENTER","RIGHT","+","END","DOWN","PGDN",
  "INS","DEL"
  };

char chartable[]=
 {
  ' ',' ','1','2','3','4','5','6','7','8','9','0',
  '-','=',' ',' ','Q','W','E','R','T','Y','U',
  'I','O','P','[',']',' ',' ','A','S','D','F',
  'G','H','J','K','L',';','\'','`',' ','\\','Z','X',
  'C','V','B','N','M',',','.','/',' ','*',' ',
  ' ',' '
  };


typedef struct
{
 char *FileName;
 char *DriverName;
 char *HelpText;
 int  DevId;
 int  Modes;
 int  Port;
 int  IrqLine;
 int  DmaChannel;
 int  SampleRate;
 int  PortTable[8],IrqTable[16],DmaTable[8];
 } DRIVER;

DRIVER DriversTable[] =
{
 {"NONE.DRV", "No Sound",
  "Do not use any sound",
  ID_NONE, AF_8BITS | AF_MONO, 0x000, 0, 0, 0,
  { 0, 0xFFFF },
  { 0, 0xFFFF },
  { 0, 0xFFFF } },

 {"SB.DRV", "Sound Blaster",
  "Plain Sound Blaster or compatible",
  ID_SB, AF_8BITS | AF_MONO, 0x220, 7, 1, 22050,
  { 0x210, 0x220, 0x230, 0x240, 0x250, 0x260, 0xFFFF },
  { 2, 3, 5, 7, 0xFFFF },
  { 1, 0xFFFF } },

 {"SB.DRV", "Sound Blaster 2.01",
  "Sound Blaster 2.01 or compatible",
  ID_SB201, AF_8BITS | AF_MONO, 0x220, 7, 1, 44100,
  { 0x210, 0x220, 0x230, 0x240, 0x250, 0x260, 0xFFFF },
  { 2, 3, 5, 7, 0xFFFF },
  { 1, 0xFFFF } },

 {"SB.DRV", "Sound Blaster Pro",
  "Sound Blaster Pro, Sound Galaxy Pro, or compatible",
  ID_SBPRO, AF_8BITS | AF_STEREO, 0x220, 5, 1, 22050,
  { 0x210, 0x220, 0x230, 0x240, 0x250, 0x260, 0xFFFF },
  { 2, 3, 5, 7, 0xFFFF },
  { 0, 1, 3, 0xFFFF } },

 {"SB.DRV", "Sound Blaster 16/16ASP",
  "Sound Blaster 16/16ASP, Wave Blaster, or Sound Blaster AWE32",
  ID_SB16, AF_16BITS | AF_STEREO, 0x220, 5, 5, 44100,
  { 0x220, 0x240, 0x260, 0x280, 0xFFFF },
  { 2, 5, 7, 10, 0xFFFF },
  { 0, 1, 3, 5, 6, 7, 0xFFFF } },

 {"PAS.DRV", "Pro Audio Spectrum",
  "Media Vision Pro Audio Spectrum or compatible",
  ID_PAS, AF_8BITS | AF_STEREO, 0x388, 7, 1, 44100,
  { 0x388, 0x384, 0x38C, 0x288, 0xFFFF },
  { 2, 3, 5, 7, 10, 11, 12, 13, 15, 0xFFFF },
  { 0, 1, 3, 5, 6, 7, 0xFFFF } },

 {"PAS.DRV", "Pro Audio Spectrum+",
  "Media Vision Pro Audio Spectrum Plus or compatible",
  ID_PASPLUS, AF_8BITS | AF_STEREO, 0x388, 7, 1, 44100,
  { 0x388, 0x384, 0x38C, 0x288, 0xFFFF },
  { 2, 3, 5, 7, 10, 11, 12, 13, 15, 0xFFFF },
  { 0, 1, 3, 5, 6, 7, 0xFFFF } },

 {"PAS.DRV", "Pro Audio Spectrum 16",
  "Pro Audio Spectrum 16, Logitech SoundMan 16, or compatible",
  ID_PAS16, AF_16BITS | AF_STEREO, 0x388, 7, 1, 44100,
  { 0x388, 0x384, 0x38C, 0x288, 0xFFFF },
  { 2, 3, 5, 7, 10, 11, 12, 13, 15, 0xFFFF },
  { 0, 1, 3, 5, 6, 7, 0xFFFF } },

 {"WSS.DRV", "Windows Sound System",
  "Windows Sound System, Audiotrix Pro, or compatible",
  ID_WSS, AF_16BITS | AF_STEREO, 0x530, 7, 3, 44100,
  { 0x530, 0x604, 0xE80, 0xF40, 0xFFFF },
  { 7, 9, 10, 11, 0xFFFF },
  { 0, 1, 3, 0xFFFF } },

 {"GUS.DRV", "Gravis UltraSound",
  "Advanced Gravis UltraSound, UltraSound Max, or compatible",
  ID_GUS, AF_16BITS | AF_STEREO, 0x220, 11, 1, 0,
  { 0x210, 0x220, 0x230, 0x240, 0x250, 0x260, 0xFFFF },
  { 2, 3, 5, 7, 11, 12, 15, 0xFFFF },
  { 1, 3, 5, 6, 7, 0xFFFF } }
 };


typedef struct
{
 int  Line;
 char Title[TITLELEN];
 char ItemsText[MAXITEMS][ITEMLEN];
 char HelpText[MAXITEMS][HELPLEN];
 } MENU;


MENU MainMenu =
{
 0, "Main Menu",
 {
  "Select Sound Card",
  "Test Sound",
  "Custom Control",
  "Joystick Setup",
  "Mouse Setup",
  "Network Setup",
  "Modem/Serial Setup",
  "Save and Exit",
//  "Save and Run Greed"
  },
 {
  "Select music and sound effects playback device",
  "Test sound playback",
  "Configure custom key combos",
  "Configure and activate joystick",
  "Configure and activate mouse",
  "Configure and begin network play",
  "Configure and begin serial network play",
  "Save SETUP file and exit to DOS",
//  "Save SETUP file and run In Pursuit of Greed"
  }
 };


MENU JoyMenu =
{
 0, "Configure Joystick",
 {
  "Joystick :    ",
  "Button 1 Select",
  "Button 2 Select",
  },
 {
  "Toggle joystick on/off",
  "Select button 1 function",
  "Select button 2 function",
  }
 };


MENU MouseMenu =
{
 0, "Configure Mouse",
 {
  "Mouse :    ",
  "Left Mouse Button",
  "Right Mouse Button",
  "Sensitivity",
  },
 {
  "Toggle mouse on/off",
  "Select left mouse button function",
  "Select right mouse button function",
  "Set mouse movement sensitivity",
  },
 };


MENU QuaMenu =
{
 2, "Select Sound Quality",
 {
  "Low Quality",
  "Medium Quality",
  "Normal Quality",
  "High Quality",
  "Very High Quality",
  },
 {
  "Select Low Quality (486/33)",
  "Select Medium Quality (486/50)",
  "Select Normal Quality (486/66)",
  "Select High Quality (586/66)",
  "Select Very High Quality (586/90)",
  }
 };


MENU TraxMenu =
{
 2,"Sound Effect Tracks",
 {
  "2",
  "3",
  "4",
  "5",
  "6",
  "7",
  "8",
  },
 };


MENU TestSoundMenu =
{
 0,"Test Sound Playback",
 {
  "Test Left Channel",
  "Test Right Channel",
  "Swap Channels",
  "Test Music",
  },
 {
  "Tests left channel playback",
  "Tests right channel playback",
  "Swaps left and right channels",
  "Plays music module",
  }
 };


MENU KbMenu =
{
 0,"Custom Control",
 {
  "Run        :         ",
  "Jump       :         ",
  "Slide      :         ",
  "Fire       :         ",
  "Use        :         ",
  "Use Item   :         ",
  "A.S.S. Cam :         ",
  "Look Up    :         ",
  "Look Down  :         ",
  "Center View:         ",
  "Slide Left :         ",
  "Slide Right:         ",
  "Inv. Left  :         ",
  "Inv. Right :         ",
  "Turn Acceleration    ",
  "Max Turn Speed       ",
  },
 {
  "Press enter + new key to change run key",
  "Press enter + new key to change jump key",
  "Press enter + new key to change slide key",
  "Press enter + new key to change fire key",
  "Press enter + new key to change use key",
  "Press enter + new key to change use item key",
  "Press enter + new key to change A.S.S. Cam key",
  "Press enter + new key to change look up key",
  "Press enter + new key to change look down key",
  "Press enter + new key to change center view key",
  "Press enter + new key to change slide left key",
  "Press enter + new key to change slide right key",
  "Press enter + new key to change inventory left key",
  "Press enter + new key to change inventory right key",
  "Select turning acceleration",
  "Select maximum turn speed",
  }
 };


MENU turnaccel=
{
 1,"Turn Acceleration",
 {
  "1",
  "2",
  "3",
  "4",
  "5",
  },
 };


MENU turnspeed=
{
 3,"Max Turn Speed",
 {
  "5",
  "6",
  "7",
  "8",
  "9",
  "10",
  "11",
  "12",
  },
 };


MENU setplayer=
{
 0,"Player Selection",
 {
  "Cyborg",
  "Lizard Man",
  "MooMan",
  "Specimen 7",
  "Dominatrix",
  "Big Head",
  }
 };


MENU setdifficulty=
{
 2,"Monster Difficulty",
 {
  "Brainless",
  "Average",
  "Aggressive",
  "Psychotic",
  "Suicidal",
  "Demon Spawn",
  }
 };


MENU selectmap1=
{
 0,"Prison Map",
 {



  },
 {

  },
 };


MENU selectmap3=
{
 0,"Multiplayer Map",
 {
#ifndef DEMO
  "Demo Level",
  "Reactor",
  "Lava Caves",
  "Courtyard",
  "Melee",
  "Pillars",
  "Castle",
  "Sewers",
  "Techno",
  "Huge",
#else
  "Demo Level",
  "Reactor",
  "Lava Caves",
  "Courtyard",
  "Melee",
#endif
  },
 {
#ifndef DEMO
  "2-4 Players",
  "2-3 Players",
  "2-4 Players",
  "2-6 Players",
  "3-5 Players",
  "3-8 Players",
  "3-7 Players",
  "4-8 Players",
  "5-8 Players",
  "6-8 Players",
#else
  "2-4 Players",
  "2-3 Players",
  "2-4 Players",
  "2-6 Players",
  "3-5 Players",
#endif
  }
 };


MENU netplay=
{
 5,"Network Setup",
 {
  "Number of Players :  ",
  "Network Socket :     ",
  "Select Player Character",
  "Name :            ",
  "Select Map",
  "PLAY Net Greed!!",
  },
 {
  "Press enter to set number of players",
  "Sets network socket number for a unique session",
  "Select player type",
  "Enter your character's name",
  "Select Network Level",
  "Press enter to start a network game",
  }
 };


MENU serplay=
{
 6,"Modem/Serial Setup",
 {
  "Player :  ",
  "Number :             ",
  "Com Port :  ",
  "Select Player Character",
  "Name :            ",
  "Select Map",
  "Dial",
  "Answer",
  "Null Modem",
  },
 {
  "Select player 1 or player 2",
  "Set number to dial with modem",
  "Set com port of modem",
  "Select player type",
  "Enter your character's name",
  "Select Network Level",
  "Dial modem and begin session",
  "Set modem to answer incoming call",
  "Begin Greed null modem session",
  }
 };


MENU players=
{
 4,"Number of Players",
  {
   "Practice",
   "2",
   "3",
   "4",
   "5",
   "6",
   "7",
   "8",
   }
 };


MENU serplayers=
{
 0,"Player Number",
  {
   "1",
   "2",
   }
 };


MENU comports=
{
 0,"Communication Ports",
 {
  "Com 1",
  "Com 2",
  "Com 3",
  "Com 4",
  }
 };


MENU mouseleft=
{
 0,"Left Mouse Button",
 {
  "Forward",
  "Backward",
  "Fire",
  "Slide",
  "Use",
  "Run"  ,
  "Jump",
  "Use Item",
  "Reverse Cam",
  "Slide Left",
  "Slide Right",
  }
 };


MENU mouseright=
{
 2,"Right Mouse Button",
 {
  "Forward",
  "Backward",
  "Fire",
  "Slide",
  "Use",
  "Run"  ,
  "Jump",
  "Use Item",
  "Reverse Cam",
  "Slide Left",
  "Slide Right",
  }
 };


MENU joybut1=
{
 2,"Joystick Button 1",
 {
  "Forward",
  "Backward",
  "Fire",
  "Slide",
  "Use",
  "Run"  ,
  "Jump",
  "Use Item",
  "Reverse Cam",
  "Slide Left",
  "Slide Right",
  }
 };


MENU joybut2=
{
 3,"Joystick Button 2",
 {
  "Forward",
  "Backward",
  "Fire",
  "Slide",
  "Use",
  "Run"  ,
  "Jump",
  "Use Item",
  "Reverse Cam",
  "Slide Left",
  "Slide Right",
  }
 };


MENU setsensitivity=
{
 2,"Mouse Sensitivity",
 {
  "Not Sensitive",
  "Non-Sensitive",
  "Default",
  "Sensitive",
  "Very Sensitive",
  }
 };


char *TextTable[] =
{
 "SETUP.CFG",
 "In Pursuit of Greed Setup (v1.00)",
 "Copyright (C) 1995 Channel 7",
 "Accept",
 "Exit",
 "Select Playback Device",
 "Select Base I/O Port",
 "Select Base I/O Port %03X hex",
 "Select IRQ Interrupt Line",
 "Select IRQ Interrupt Line %d",
 "Select DMA Channel",
 "Select DMA Channel %d",
 "SETUP file saved.",
 "Error saving SETUP file!",
 "Setup aborted."
 };


int RatesTable[] =
 { 16000, 19025, 22050, 33075, 44100 };


/**** FUNCTIONS ****/

int ReadKey(void)
{
 int c;

 if (!(c=getch())) c=getch()<<8;
 return c;
 }


void set_kb_int()
{
 r.x.eax=0x3509;   /* DOS get vector (INT 9) */
 sr.ds=sr.es=0;
 int386x(0x21, &r, &r, &sr);
 orig_pm_sel=(WORD)sr.es;
 orig_pm_off=r.x.ebx;
 r.x.eax=0x2509;   /* DOS set vector (INT 9) */
 fh=(void near *)kisr;
 r.x.edx=FP_OFF(fh);  /* DS:EDX == &handler */
 sr.ds=FP_SEG(fh);
 sr.es=0;
 int386x(0x21, &r, &r, &sr);
 }


void restore_kb_int(void)
{
 r.x.eax=0x2509;   /* DOS set vector (INT 9) */
 r.x.edx=orig_pm_off;
 sr.ds=orig_pm_sel;    /* DS:EDX == &handler */
 sr.es=0;
 int386x(0x21, &r, &r, &sr);
 }

/*---------------------- Setup I/O File Routines --------------------------*/

int dLoadSetup(SoundCard *SC, char *Filename)
{
 int Handle;

 if ((Handle = open(Filename,O_RDONLY|O_BINARY)) < 0) return 1;
 if (read(Handle,SC,sizeof(SoundCard)) != sizeof(SoundCard))
  {
   close(Handle);
   return 1;
   }
 close(Handle);
 return 0;
 }


int dSaveSetup(SoundCard *SC, char *Filename)
{
 int Handle;

 if ((Handle = open(Filename,O_CREAT|O_WRONLY|O_BINARY,S_IRWXU)) < 0) return 1;
 if (write(Handle,SC,sizeof(SoundCard)) != sizeof(SoundCard))
  {
   close(Handle);
   return 1;
   }
 close(Handle);
 return 0;
 }

/*-------------------- Text Screen Mode Routines --------------------------*/

void SetTextMode(void)
{
 union REGS r;

 r.h.ah=0x00;
 r.h.al=0x03;
 int386(0x10,&r,&r);
 }


void SetPalette(int index, int red, int green, int blue)
{
 outp(0x3c8,index);
 outp(0x3c9,red);
 outp(0x3c9,green);
 outp(0x3c9,blue);
 }


void WaitVertRetrace(void)
{
 while (inp(0x3da) & 0x08) ;
 while (!(inp(0x3da) & 0x08)) ;
 while (!(inp(0x3da) & 0x01)) ;
 }


void HideCursor(void)
{
 outpw(0x3d4,0x100a);
 outpw(0x3d4,0x100b);
 }


void DrawRect(int x, int y, int width, int height, int c, int color)
{
 char *ptr = (char*)(0xb8000+((x+80*y)<<1));
 int  i,j;

 for (j = 0; j < height; j++)
  {
   for (i = 0; i < width; i++)
    {
     *ptr++ = c;
     *ptr++ = color;
     }
   ptr += (80-width)<<1;
   }
 }


void DrawFrame(int x, int y, int width, int height, int color)
{
 static char frame[]={ 0xda,0xbf,0xc0,0xd9,0xc4,0xb3,0xc3,0xb4,0x20 };
 char        *ptr=(char*)(0xb8000+((x+80*y)<<1));
 int         i,j;

 *ptr++=frame[0];
 *ptr++=color;
 for (i=0;i<width;i++)
  {
   *ptr++=frame[4];
   *ptr++=color;
   }
 *ptr++=frame[1];
 *ptr++=color;
 ptr += (78-width)<<1;

 *ptr++=frame[5];
 *ptr++=color;
 for (i=0;i<width;i++)
  {
   *ptr++=frame[8];
   *ptr++=color;
   }
 *ptr++=frame[5];
 *ptr++=color;
 ptr += (78-width)<<1;

 *ptr++=frame[6];
 *ptr++=color;
 for (i=0;i<width;i++)
  {
   *ptr++=frame[4];
   *ptr++=color;
   }
 *ptr++=frame[7];
 *ptr++=color;
 ptr += (78-width)<<1;

 for (j=0;j<height;j++)
  {
   *ptr++=frame[5];
   *ptr++=color;
   for (i=0;i<width;i++)
    {
     *ptr++=frame[8];
     *ptr++=color;
     }
   *ptr++=frame[5];
   *ptr++=color;
   ptr += (78-width)<<1;
   }

 *ptr++=frame[6];
 *ptr++=color;
 for (i=0;i<width;i++)
  {
   *ptr++=frame[4];
   *ptr++=color;
   }
 *ptr++=frame[7];
 *ptr++=color;
 ptr += (78-width)<<1;

 *ptr++=frame[5];
 *ptr++=color;
 for (i=0;i<width;i++)
  {
   *ptr++=frame[8];
   *ptr++=color;
   }
 *ptr++=frame[5];
 *ptr++=color;
 ptr += (78-width)<<1;

 *ptr++=frame[2];
 *ptr++=color;
 for (i=0;i<width;i++)
  {
   *ptr++=frame[4];
   *ptr++=color;
   }
 *ptr++=frame[3];
 *ptr++=color;
 ptr += (78-width)<<1;
 }


void DrawText(int x, int y, char *text, int color)
{
 char *ptr=(char*)(0xb8000+((x+80*y)<<1));

 while (*text)
  {
   *ptr++=*text++;
   *ptr++=color;
   }
 }


void DrawCenterText(int y, char *text, int color)
{
 DrawText((80-strlen(text))>>1,y,text,color);
 }

/*--------------------------- Menu Routines -------------------------------*/

void DrawMenu(MENU *menu)
{
 int x,y,width,height;
 int i,j;

 WaitVertRetrace();
 width=strlen(menu->Title);
 if ((j=10+strlen(TextTable[3])+strlen(TextTable[4])) > width) width=j;
 for (i=height=0;i<MAXITEMS;i++,height++)
  {
   if (menu->ItemsText[i]==NULL || !(j=strlen(menu->ItemsText[i]))) break;
   if (j>width) width=j;
   }
 width += 4;
 x=(78-width)>>1;
 y=2+((14-height)>>1);
 DrawRect(x+1,y+1,width+3,height+6,0xb0,0x37);
 DrawFrame(x,y,width,height,FRAMECOLOR);
// DrawRect(x+1,y+1,width,1,0x20,MENUTITLECOLOR);
 DrawText(x+1+((width-strlen(menu->Title))>>1),y+1,menu->Title,MENUTITLECOLOR);
 DrawText(x+2,y+4+height,"ENTER",COMMANDCOLOR);
 DrawText(x+7,y+4+height,"=",FRAMECOLOR);
 DrawText(x+8,y+4+height,TextTable[3],0x1f);
 DrawText(x-4+width-strlen(TextTable[4]),y+4+height,"ESC",COMMANDCOLOR);
 DrawText(x-1+width-strlen(TextTable[4]),y+4+height,"=",FRAMECOLOR);
 DrawText(x+width-strlen(TextTable[4]),y+4+height,TextTable[4],0x1f);
 for (i=0;i<height;i++)
  {
   if (menu->Line==i) DrawRect(x+1,y+i+3,width,1,0x20,0x71);
   DrawText(x+3,y+i+3,menu->ItemsText[i],(menu->Line == i)?0x71:0x1f);
   }
 DrawRect(0,24,80,1,0x20,TITLECOLOR);
 DrawText(1,24,menu->HelpText[menu->Line],TITLECOLOR);
 }


int ExecMenu(MENU *menu)
{
 int n;

 for (n=0; n < MAXITEMS; n++)
  if (!strlen(menu->ItemsText[n])) break;
 if (n <= 1) return 0;

 WaitVertRetrace();
 DrawRect(0,1,80,23,0xB1,0x37);
 for (;;)
  {
   DrawMenu(menu);
   scode=0;
   while(!scode){};
   if (scode==KB_ESC) break;
   if (scode==42)
    {
     scode=0;
     while(!scode){};
     }

   switch (scode)
    {
     case KB_ENTER:
      return menu->Line;
      break;
     case KB_UP:
      (menu->Line)--;
      if (menu->Line < 0) (menu->Line)++;
      break;
     case KB_DOWN:
      (menu->Line)++;
      if (menu->Line >= n) (menu->Line)--;
      break;
      }
    }
 return -1;
 }

/*------------------- Select Soundcard Menu Routines ----------------------*/

#define MAXDRIVERS ((MAXITEMS>NUMDRIVERS)?NUMDRIVERS:MAXITEMS)

MENU *MakeDeviceMenu(MENU *menu, SoundCard *SC)
{
 DRIVER *drv;
 int    i;

 memset(menu,0,sizeof(MENU));
 strncpy(menu->Title,TextTable[5],TITLELEN);
 for (drv=DriversTable, i=0; i < MAXDRIVERS; drv++, i++)
  {
   strncpy(menu->ItemsText[i],drv->DriverName,ITEMLEN);
   strncpy(menu->HelpText[i],drv->HelpText,HELPLEN);
   if (SC->ID == drv->DevId) menu->Line=i;
   }
 return menu;
 }


MENU *MakePortMenu(MENU *menu, SoundCard *SC)
{
 DRIVER *drv;
 int    i,j;

 memset(menu,0,sizeof(MENU));
 strncpy(menu->Title,TextTable[6],TITLELEN);
 for (drv=DriversTable, i=0; i < MAXDRIVERS; drv++, i++)
  if (SC->ID == drv->DevId) break;
 for (i=0; (i < MAXITEMS) && ((j=drv->PortTable[i]) != 0xFFFF); i++)
  {
   _bprintf(menu->ItemsText[i],ITEMLEN,"%03X",j);
   _bprintf(menu->HelpText[i],HELPLEN,TextTable[7],j);
   if (SC->Port == j) menu->Line=i;
   }
 return menu;
 }


MENU *MakeIrqMenu(MENU *menu, SoundCard *SC)
{
 DRIVER *drv;
 int    i,j;
 memset(menu,0,sizeof(MENU));
 strncpy(menu->Title,TextTable[8],TITLELEN);
 for (drv=DriversTable, i=0; i < MAXDRIVERS; drv++, i++)
  if (SC->ID == drv->DevId) break;
 for (i=0; (i < MAXITEMS) && ((j=drv->IrqTable[i]) != 0xFFFF); i++)
  {
   _bprintf(menu->ItemsText[i],ITEMLEN,"IRQ %d",j);
   _bprintf(menu->HelpText[i],HELPLEN,TextTable[9],j);
   if (SC->IrqLine == j) menu->Line=i;
   }
 return menu;
 }


MENU *MakeDmaMenu(MENU *menu, SoundCard *SC)
{
 DRIVER *drv;
 int    i,j;

 memset(menu,0,sizeof(MENU));
 strncpy(menu->Title,TextTable[10],TITLELEN);
 for (drv=DriversTable, i=0; i < MAXDRIVERS; drv++, i++)
  if (SC->ID == drv->DevId) break;
 for (i=0; (i < MAXITEMS) && ((j=drv->DmaTable[i]) != 0xFFFF); i++)
  {
   _bprintf(menu->ItemsText[i],ITEMLEN,"DMA %d",j);
   _bprintf(menu->HelpText[i],HELPLEN,TextTable[11],j);
   if (SC->DmaChannel == j) menu->Line=i;
   }
 return menu;
 }


int SelectSoundCard(SoundCard *SC)
{
 MENU   menu;
 DRIVER *drv;

 if (ExecMenu(MakeDeviceMenu(&menu,SC)) < 0) return 1;
 drv=&DriversTable[menu.Line];
 if (SC->ID != drv->DevId)
  {
   SC->Port=drv->Port;
   SC->IrqLine=drv->IrqLine;
   SC->DmaChannel=drv->DmaChannel;
   }
 SC->ID=drv->DevId;
 SC->Modes=drv->Modes;
 if (ExecMenu(MakePortMenu(&menu,SC)) < 0) return 1;
 SC->Port=drv->Port=drv->PortTable[menu.Line];
 if (ExecMenu(MakeIrqMenu(&menu,SC)) < 0) return 1;
 SC->IrqLine=drv->IrqLine=drv->IrqTable[menu.Line];
 if (ExecMenu(MakeDmaMenu(&menu,SC)) < 0) return 1;
 SC->DmaChannel=drv->DmaChannel=drv->DmaTable[menu.Line];
 if (drv->SampleRate)
  {
   if (ExecMenu(&QuaMenu) < 0) return 1;
   if ((SC->SampleRate=RatesTable[QuaMenu.Line]) > drv->SampleRate)
    SC->SampleRate=drv->SampleRate;
   }
 else
  {
   SC->SampleRate=44100;
   }

 if (SC->ID)
  {
   if (ExecMenu(&TraxMenu) < 0)
    {
     SC->effecttracks=3;
     return 1;
     }
   SC->effecttracks=TraxMenu.Line + 2;
   }
 return 0;
 }

/*-------------------- Save and Exit Menu Routines ------------------------*/

int SaveAndExit(SoundCard *SC, char *Filename)
{
 int I;

 for (I=0; I < NUMDRIVERS; I++)
  if (SC->ID == DriversTable[I].DevId)
   strncpy(SC->DriverName,DriversTable[I].FileName,sizeof(SC->DriverName));
 return dSaveSetup(SC,Filename);
 }


void newkey(char *keyval,char *menustr)
{
 strcpy(menustr,"       ");
 DrawMenu(&KbMenu);
 scode=0;
 while(1)
  {
   while(!scode){}
   if (scode<0x54) break;
   scode=0;
   }
 *keyval=scode;
 scode=0;
 strcpy(menustr,keytable[scode]);
 }

//°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°
//° Set cutsom keys                                                         °
//°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°
void SetKeys(SoundCard *SC)
{
 int i;

 while(1)
  {
   for (i=0;i<14;i++)
    strcpy(&KbMenu.ItemsText[i][13],keytable[SC->Ckeys[i]]);
   if (ExecMenu(&KbMenu)<0) break;
   if (KbMenu.Line<14)
    newkey(&SC->Ckeys[KbMenu.Line],&KbMenu.ItemsText[KbMenu.Line][13]);
   else if (KbMenu.Line==14)
    {
     turnaccel.Line=SC->turnaccel-1;
     if (ExecMenu(&turnaccel)>=0)
      SC->turnaccel=1+turnaccel.Line;
     }
   else if (KbMenu.Line==15)
    {
     turnspeed.Line=SC->turnspeed-5;
     if (ExecMenu(&turnspeed)>=0)
      SC->turnspeed=5+turnspeed.Line;
     }
   }
 }

/*----------------------- Test Sound Here ---------------------------------*/

void TestSound(SoundCard *SC)
{
 DSM       *M;
 Sample    *samp;
 SoundCard temp;

 memcpy(&temp,SC,sizeof(SoundCard));
 dRegisterDrivers();
 if ((dInit(&temp)) || (temp.ID==0) )
  {
   DrawRect(0,24,80,1,0x20,TITLECOLOR);
   DrawText(1,24,"Couldn't init sound card",TITLECOLOR);
   scode=0;
   while (!scode){};
   dDone();
   return;
   }

 while(1)
  {
   if (ExecMenu(&TestSoundMenu) < 0) break;

   memcpy(&temp,SC,sizeof(SoundCard));
   dRegisterDrivers();
   dInit(&temp);
   if (TestSoundMenu.Line == 0 || TestSoundMenu.Line==1)
    {
     if (!(samp=dImportSample("sample.wav",FORM_WAV)))
      {
       DrawRect(0,24,80,1,0x20,TITLECOLOR);
       DrawText(1,24,"Couldn't load sample.wav ",TITLECOLOR);
       scode=0;
       while (!scode) {};
       }
     else
      {
       dSetupVoices(1,255);
       dPlayVoice(0,samp);
       if (TestSoundMenu.Line==0)
	{
	 if (SC->InversePan) dSetVoiceBalance(0,PAN_RIGHT);
	  else dSetVoiceBalance(0,PAN_LEFT);
	 }
       else
	{
	 if (SC->InversePan) dSetVoiceBalance(0,PAN_LEFT);
	  else dSetVoiceBalance(0,PAN_RIGHT);
	 }
       DrawRect(0,24,80,1,0x20,TITLECOLOR);
       DrawText(1,24,"Sample Playing...",TITLECOLOR);
       scode=0;
       while (!scode && dGetVoiceStatus(0) == PS_PLAYING) dPoll();
       dStopVoice(0);
       dFreeSample(samp);
       dDone();
       }
     }
   if (TestSoundMenu.Line == 2) SC->InversePan^=1;

   if (TestSoundMenu.Line == 3)
    {
     DrawRect(0,24,80,1,0x20,TITLECOLOR);
     DrawText(1,24,"Loading module",TITLECOLOR);
     M=dImportModule("Song0.S3M",FORM_S3M);
     if (M)
      {
       dSetupVoices(M->Header.NumTracks,M->Header.MasterVolume);
       dPlayMusic(M);
       dSetMusicVolume(255);
       DrawRect(0,24,80,1,0x20,TITLECOLOR);
       DrawText(1,24,"Music Playing...",TITLECOLOR);
       scode=0;
       while (!scode) dPoll();
       dStopMusic();
       dFreeModule(M);
       dDone();
       }
     else
      {
       DrawText(1,24,"Error loading SONG0.S3M ",TITLECOLOR);
       scode=0;while(!scode){};
       }
     }
   }
 dDone();
 }

//°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°
//° Set Network play                                                        °
//°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°
void NetPlay(SoundCard *SC)
{
 char socket[5];
 int  i;

 while(1)
  {
   sprintf(&netplay.ItemsText[0][20],"%d",playernum);
   sprintf(&netplay.ItemsText[1][17],"%d",socketnum);
   strncpy(&netplay.ItemsText[3][7],&SC->netname[0],12);
   sprintf(&socket[0],"%d",socketnum);
   if (ExecMenu(&netplay)<0) break;
   switch (netplay.Line)
    {
     case 0:
      players.Line=playernum-1;
      if (ExecMenu(&players)>=0)
       {
	playernum=players.Line+1;
	SC->numplayers=playernum;
	}
      break;
     case 1:
      strncpy(socket,"    ",4);
      for (i=0;i<4;i++)
       {
	socket[i]='_';
	strncpy(&netplay.ItemsText[1][17],socket,4);
	DrawMenu(&netplay);
	scode=0;
	while(!scode){};
	if ((scode==0x0E || scode==0x53 || scode==0x4B) && i!=0)
	 {
	  socket[i]=' ';
	  socket[i-1]=' ';
	  i-=2;
	  continue;
	  }
	if (scode==0x1C)
	 {
	  socket[i]=' ';
	  break;
	  }
	if (scode>0x0C || scode<0x02)
	 {
	  --i;
	  continue;
	  }
	socket[i]=chartable[scode];
	}
      socketnum=atoi(&socket[0]);
      SC->socket=socketnum;
      break;
     case 2:
      setplayer.Line=SC->chartype; //netstuff->character;
      if (ExecMenu(&setplayer)>=0)
       {
	netstuff->character=setplayer.Line;
	SC->chartype=setplayer.Line;
	}
      break;
     case 3:
      strncpy(netname,"            ",12);
      for (i=0;i<12;i++)
       {
	netname[i]='_';
	strcpy(&netplay.ItemsText[3][7],netname);
	DrawMenu(&netplay);
	scode=0;
	while (!scode) {};
	if ((scode==0x0E || scode==0x53 || scode==0x4B) && i!=0)
	 {
	  netname[i]=' ';
	  netname[i-1]=' ';
	  i-=2;
	  continue;
	  }
	if (scode==0x1C)
	 {
	  netname[i]=' ';
	  break;
	  }
	if (scode<=0x01 || scode==0x0C || scode==0x0D || (scode>0x19 && scode<0x1E)
	 || (scode>0x26 && scode<0x2C) || (scode>0x32 && scode<0x39) || scode>0x39)
	 {
	  --i;
	  continue;
	  }
	netname[i]=chartable[scode];
	}
      strncpy(SC->netname,&netname[0],12);
      break;
     case 4:
      selectmap.Line=SC->netmap-22;
      if (ExecMenu(&selectmap)>=0)
       SC->netmap=selectmap.Line+22;
      break;
     case 5:
      restore_kb_int();
      SetTextMode();
      netstuff->numplayers=playernum;
      strncpy(netstuff->netsocket,socket,12);
      SaveAndExit(SC,TextTable[0]); // save config file
      exit(5);
     }
   }
 }

//°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°
//° Set Serial Network play                                                 °
//°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°
void sernet(SoundCard *SC)
{
 int i;

 while(1)
  {
   sprintf(&serplay.ItemsText[0][9],"%d",netstuff->serplayers);
   strncpy(&serplay.ItemsText[1][9],&dialstr[0],12);
   sprintf(&serplay.ItemsText[2][11],"%d",netstuff->comport);
   strncpy(&serplay.ItemsText[4][7],&SC->netname[0],12);

   if (ExecMenu(&serplay)<0) break;

   switch (serplay.Line)
    {
     case 0:
      serplayers.Line=SC->serplayers-1;
      while (1)
       {
	if (ExecMenu(&serplayers)<0) break;
	netstuff->serplayers=serplayers.Line+1;
	SC->serplayers=serplayers.Line+1;
	break;
	}
      break;
     case 1:
      strncpy(dialstr,"            ",12);
      for (i=0;i<12;i++)
       {
	dialstr[i]='_';
	strncpy(&serplay.ItemsText[1][9],dialstr,12);
	DrawMenu(&serplay);
	scode=0;
	while (!scode) {};
	if ((scode==0x0E || scode==0x53 || scode==0x4B) && i!=0)
	 {
	  dialstr[i]=' ';
	  dialstr[i-1]=' ';
	  i-=2;
	  continue;
	  }
	if (scode==0x1C)
	 {
	  dialstr[i]=' ';
	  break;
	  }
	if (scode<=0x01 || (scode>0x0C && scode!=0x39))
	 {
	  --i;
	  continue;
	  }
	dialstr[i]=chartable[scode];
	}
      strncpy(SC->dialnum,&dialstr[0],12);
      break;
     case 2:
      comports.Line=SC->com-1;
      while (1)
       {
	if (ExecMenu(&comports)<0) break;
	netstuff->comport=comports.Line+1;
	SC->com=comports.Line+1;
	break;
	}
      break;
     case 3:
      setplayer.Line=SC->chartype;
      while (1)
       {
	if (ExecMenu(&setplayer)<0) break;
	netstuff->character=setplayer.Line;
	SC->chartype=setplayer.Line;
	break;
	}
      break;
     case 4:
      strncpy(netname,"            ",12);
      for (i=0;i<12;i++)
       {
	netname[i]='_';
	strncpy(&serplay.ItemsText[4][7],netname,12);
	DrawMenu(&serplay);
	scode=0;
	while (!scode) {};
	if ((scode==0x0E || scode==0x53 || scode==0x4B) && i!=0)
	 {
	  netname[i]=' ';
	  netname[i-1]=' ';
	  i-=2;
	  continue;
	  }
	if (scode==0x1C)
	 {
	  netname[i]=' ';
	  break;
	  }
	if (scode<=0x01 || scode==0x0C || scode==0x0D || (scode>0x19 && scode<0x1E)
	 || (scode>0x26 && scode<0x2C) || (scode>0x32 && scode<0x39) || scode>0x39)
	 {
	  --i;
	  continue;
	  }
	netname[i]=chartable[scode];
	}
      strncpy(SC->netname,&netname[0],12);
      break;
     case 5:
      selectmap.Line=SC->netmap-22;
      if (ExecMenu(&selectmap)>=0)
       SC->netmap=selectmap.Line+22;
      break;
     case 6:
      restore_kb_int();
      SetTextMode();
      SaveAndExit(SC,TextTable[0]); // save config file
      strncpy(&netstuff->dialnum[0],&dialstr[0],12);
      exit(6); // dial
     case 7:
      restore_kb_int();
      SetTextMode();
      SaveAndExit(SC,TextTable[0]); // save config file
      exit(7); // answer
     case 8:
      restore_kb_int();
      SetTextMode();
      SaveAndExit(SC,TextTable[0]); // save config file
      exit(8); // null modem
     }
   }
 }

//°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°
//° Set mouse control                                                       °
//°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°
void setmouse(SoundCard *SC)
{
 if (SC->mouse==0) strcpy(&MouseMenu.ItemsText[0][8],"OFF");
  else strcpy(&MouseMenu.ItemsText[0][8],"ON");
 while(1)
  {
   if (ExecMenu(&MouseMenu)<0) break;
   if (MouseMenu.Line==0)
    {
     SC->mouse^=1;
     if (SC->mouse==0) strcpy(&MouseMenu.ItemsText[0][8],"OFF");
      else strcpy(&MouseMenu.ItemsText[0][8],"ON");
     }
   else if (MouseMenu.Line==1)
    while (1)
     {
      if (ExecMenu(&mouseleft)<0) break;
      switch (mouseleft.Line)
       {
	case 0:
	 SC->leftbutton=bt_north;
	 break;
	case 1:
	 SC->leftbutton=bt_south;
	 break;
	case 2:
	 SC->leftbutton=bt_fire;
	 break;
	case 3:
	 SC->leftbutton=bt_slide;
	 break;
	case 4:
	 SC->leftbutton=bt_use;
	 break;
	case 5:
	 SC->leftbutton=bt_run;
	 break;
	case 6:
	 SC->leftbutton=bt_jump;
	 break;
	case 7:
	 SC->leftbutton=bt_useitem;
	 break;
	case 8:
	 SC->leftbutton=bt_asscam;
	 break;
	case 9:
	 SC->leftbutton=bt_slideleft;
	 break;
	case 10:
	 SC->leftbutton=bt_slideright;
	 break;
	}
      break;
      }
   else if (MouseMenu.Line==2)
    while (1)
     {
      if (ExecMenu(&mouseright)<0) break;
      switch (mouseright.Line)
       {
	case 0:
	 SC->rightbutton=bt_north;
	 break;
	case 1:
	 SC->rightbutton=bt_south;
	 break;
	case 2:
	 SC->rightbutton=bt_fire;
	 break;
	case 3:
	 SC->rightbutton=bt_slide;
	 break;
	case 4:
	 SC->rightbutton=bt_use;
	 break;
	case 5:
	 SC->rightbutton=bt_run;
	 break;
	case 6:
	 SC->rightbutton=bt_jump;
	 break;
	case 7:
	 SC->rightbutton=bt_useitem;
	 break;
	case 8:
	 SC->rightbutton=bt_asscam;
	 break;
	case 9:
	 SC->rightbutton=bt_slideleft;
	 break;
	case 10:
	 SC->rightbutton=bt_slideright;
	 break;
	}
      break;
      }
   else if (MouseMenu.Line==3)
    while (1)
     {
      if (ExecMenu(&setsensitivity)<0) break;
      switch (setsensitivity.Line)
       {
	case 0:
	 SC->mousesensitivity=48;
	 break;
	case 1:
	 SC->mousesensitivity=40;
	 break;
	case 2:
	 SC->mousesensitivity=32;
	 break;
	case 3:
	 SC->mousesensitivity=24;
	 break;
	case 4:
	 SC->mousesensitivity=16;
	 break;
	}
      break;
      }
   }
 }


//°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°
//° Set joystick control                                                    °
//°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°±²±°
void setjoy(SoundCard *SC)
{
 if (SC->jstick==0) strcpy(&JoyMenu.ItemsText[0][11],"OFF");
  else strcpy(&JoyMenu.ItemsText[0][11],"ON");
 while(1)
  {
   if (ExecMenu(&JoyMenu)<0) break;
   if (JoyMenu.Line==0)
    {
     SC->jstick^=1;
     if (SC->jstick==0) strcpy(&JoyMenu.ItemsText[0][11],"OFF");
      else strcpy(&JoyMenu.ItemsText[0][11],"ON");
     }
   else if (JoyMenu.Line==1)
    while (1)
     {
      if (ExecMenu(&joybut1)<0) break;
      switch (joybut1.Line)
       {
	case 0:
	 SC->joybut1=bt_north;
	 break;
	case 1:
	 SC->joybut1=bt_south;
	 break;
	case 2:
	 SC->joybut1=bt_fire;
	 break;
	case 3:
	 SC->joybut1=bt_slide;
	 break;
	case 4:
	 SC->joybut1=bt_use;
	 break;
	case 5:
	 SC->joybut1=bt_run;
	 break;
	case 6:
	 SC->joybut1=bt_jump;
	 break;
	case 7:
	 SC->joybut1=bt_useitem;
	 break;
	case 8:
	 SC->joybut1=bt_asscam;
	 break;
	case 9:
	 SC->joybut1=bt_slideleft;
	 break;
	case 10:
	 SC->joybut1=bt_slideright;
	 break;
	}
      break;
      }
   else if (JoyMenu.Line==2)
    while (1)
     {
      if (ExecMenu(&joybut2)<0) break;
      switch (joybut2.Line)
       {
	case 0:
	 SC->joybut2=bt_north;
	 break;
	case 1:
	 SC->joybut2=bt_south;
	 break;
	case 2:
	 SC->joybut2=bt_fire;
	 break;
	case 3:
	 SC->joybut2=bt_slide;
	 break;
	case 4:
	 SC->joybut2=bt_use;
	 break;
	case 5:
	 SC->joybut2=bt_run;
	 break;
	case 6:
	 SC->joybut2=bt_jump;
	 break;
	case 7:
	 SC->joybut2=bt_useitem;
	 break;
	case 8:
	 SC->joybut2=bt_asscam;
	 break;
	case 9:
	 SC->joybut2=bt_slideleft;
	 break;
	case 10:
	 SC->joybut2=bt_slideright;
	 break;
	}
      break;
      }
   }
 }

/*----------------------- Setup Main Program ------------------------------*/

void SetupProgram(SoundCard *SC)
{
 SetTextMode();
 HideCursor();
// SetPalette(1,1,4,16);
 SetPalette(1,0,0,24);
 SetPalette(3,4,6,26);
 DrawRect(0,1,80,22,0xb1,0x37);
 DrawRect(0,0,80,1,0x20,TITLECOLOR);
 DrawCenterText(0,TextTable[1],TITLECOLOR);
// DrawCenterText(1,TextTable[2],TITLECOLOR);
 if (dLoadSetup(SC,TextTable[0]))
  { /* default settings */
   memset(SC,0,sizeof(SoundCard));
   dAutoDetect(SC);
   SC->Ckeys[0]=0x2a;
   SC->Ckeys[1]=44;
   SC->Ckeys[2]=56;
   SC->Ckeys[3]=29;
   SC->Ckeys[4]=57;
   SC->Ckeys[5]=45;
   SC->Ckeys[6]=0x1E;  // bt_asscam
   SC->Ckeys[7]=0x49;  // bt_lookup
   SC->Ckeys[8]=0x51;  // bt_lookdown
   SC->Ckeys[9]=0x47;  // bt_centerview
   SC->Ckeys[10]=0x33; // bt_slideleft
   SC->Ckeys[11]=0x34; // bt_slideright
   SC->Ckeys[12]=0x52; // bt_invleft
   SC->Ckeys[13]=0x53; // bt_invright
   SC->ambientlight=2048;
   SC->violence=1;
   SC->animation=1;
   SC->musicvol=100;
   SC->sfxvol=128;
   SC->InversePan=0;
   SC->screensize=0;
   SC->camdelay=35;
   SC->effecttracks=4;
   SC->mouse=1;
   SC->jstick=0;
   SC->chartype=0;
   SC->socket=1234;
   SC->numplayers=2;
   SC->serplayers=1;
   SC->com=1;
   SC->rightbutton=bt_north;
   SC->leftbutton=bt_fire;
   SC->joybut1=bt_fire;
   SC->joybut2=bt_slide;
   SC->netmap=1;
   SC->netdifficulty=2;
   strncpy(SC->dialnum,"           ",12);
   strncpy(SC->netname,"           ",12);
   SC->netmap=22;
   SC->netdifficulty=2;
   SC->mousesensitivity=32;
   SC->turnaccel=2;
   SC->turnspeed=8;
   }
 playernum=SC->numplayers;
 socketnum=SC->socket;

 netstuff->serplayers=SC->serplayers;
 netstuff->comport=SC->com;
 netstuff->character=SC->chartype;
 strcpy(dialstr,SC->dialnum);
 strcpy(netname,SC->netname);

 for (;;)
  {
   if (ExecMenu(&MainMenu) < 0)
    {
     MainMenu.Line=MENU_SELECTCARD;
     break;
     }
   if (MainMenu.Line == MENU_SELECTCARD)
    {
     if (SelectSoundCard(SC)) continue;
     }
   else if (MainMenu.Line == MENU_TESTSOUND) TestSound(SC);
   else if (MainMenu.Line == MENU_SETKEYS) SetKeys(SC);
   else if (MainMenu.Line == MENU_NETWORK) NetPlay(SC);
   else if (MainMenu.Line == MENU_SERSET) sernet(SC);
   else if (MainMenu.Line == MENU_JSTICK) setjoy(SC);
   else if (MainMenu.Line == MENU_MOUSE) setmouse(SC);
   else if (MainMenu.Line == MENU_SAVEEXITRUN)
    {
     SaveAndExit(SC,TextTable[0]);
     SetTextMode();
     exit(4);
     }
   else if (MainMenu.Line == MENU_SAVEEXIT) break;
   }
 SetTextMode();
 if (MainMenu.Line == MENU_SAVEEXIT)
  {
   printf("%s\n", SaveAndExit(SC,TextTable[0]) ? TextTable[13] : TextTable[12]);
   }
 else
  {
   printf("%s\n",TextTable[14]);
   }
 }


void lock_region(void *address, unsigned length)
{
 union REGS regs;
 unsigned linear;

 linear=(unsigned) address;
 regs.w.ax=0x600;                 /* DPMI Lock Linear Region */
 regs.w.bx=(linear>>16);          /* Linear address in BX:CX */
 regs.w.cx=(linear&0xFFFF);
 regs.w.si=(length>>16);          /* Length in SI:DI */
 regs.w.di=(length&0xFFFF);
 int386(0x31,&regs,&regs);
 }


int main(int argc,char *argv[])
{
 SoundCard SC;

 lock_region(&scode,128);
 lock_region(kisr,128);

 netstuff=(netargs *) atoi(argv[1]);
 netstuff->serplayers=1;
 netstuff->comport=1;
 netstuff->character=0;

 set_kb_int();
 SetupProgram(&SC);
 restore_kb_int();

 printf("\nIn Pursuit of Greed\n"
	"Developed by Mind Shear Software\n"
	"Copyright (c) 1996 by Softdisk Publishing\n"
	"All Rights Reserved.\n\n");
 return EXIT_SUCCESS;
 }
