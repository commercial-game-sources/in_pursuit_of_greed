/****************************************************************************
*
*                   Digital Sound Interface Kit (DSIK)
*                            Version 2.00
*
*                           by Carlos Hasan
*
* Filename:     timer.c
* Version:      Revision 1.1
*
* Language:     WATCOM C
* Environment:  IBM PC (DOS/4GW)
*
* Description:  Timer interrupt services.
*
* Revision History:
* ----------------
*
* Revision 1.1  94/11/16  10:48:42  chv
* Added VGA vertical retrace synchronization code
*
* Revision 1.0  94/10/28  22:45:47  chv
* Initial revision
*
****************************************************************************/

#include <STDLIB.H>
#include <CONIO.H>
#include <DOS.H>
#include "timer.h"


static void (interrupt far *BiosTimer)(void) = NULL;
static void (*UserTimer)(void);
static long TimerCount, TimerSpeed, TimerSync;


void timerstub1(void)
{
 }
static void WaitVR(void)
{
 while (inp(0x3da) & 8) ;
 while (!(inp(0x3da) & 8)) ;
 }


static void StartTimer2(void)
{
 outp(0x61,inp(0x61) | 0x01);
 outp(0x43,0xb4);
 outp(0x42,0x00);
 outp(0x42,0x00);
 }


static int ReadTimer2(void)
{
 int n;

 outp(0x43,0xb0);
 n = inp(0x42);
 n += inp(0x42) << 8;
 return 0x10000L - n;
 }


static int GetSyncTicks(void)
{
 int n,Ticks,Delta;

 for (n = 0; n < 16; n++)
  {
   _disable();
   WaitVR();
   StartTimer2();
   WaitVR();
   Ticks = Delta = ReadTimer2();
   WaitVR();
   StartTimer2();
   WaitVR();
   Delta -= ReadTimer2();
   _enable();
   if (Delta >= -64 && Delta <= +64) return Ticks;
   }
   return TICKS(70);
 }


static void interrupt TimerHandler(void)
{
 static int TimerSem = 0;

 if (TimerSem) outp(0x20,0x20);
 else
  {
   TimerSem++;
   if (TimerSync)
    {
     WaitVR();
     outp(0x43,0x30);
     outp(0x40,TimerSpeed & 0xff);
     outp(0x40,TimerSpeed >> 8);
     }
   (*UserTimer)();
   if ((TimerCount += TimerSpeed) >= 0x10000)
    {
     TimerCount -= 0x10000;
     (*BiosTimer)();
     }
   else outp(0x20,0x20);
   TimerSem--;
   }
 }


static void IdleTimer(void)
{
 }


void dInitTimer(void)
{
 if (!BiosTimer)
  {
   UserTimer = IdleTimer;
   BiosTimer = _dos_getvect(0x08);
   _dos_setvect(0x08,TimerHandler);
   dSetTimerSpeed(0x10000);
   }
 }


void dDoneTimer(void)
{
 if (BiosTimer)
  {
   dSetTimerSpeed(0x10000);
   _dos_setvect(0x08,BiosTimer);
   BiosTimer = NULL;
   }
 }


void dSetTimerSpeed(int Speed)
{
 if (BiosTimer)
  {
   TimerSync = !(TimerSpeed = Speed);
   if (TimerSync)
    {
     TimerSpeed = GetSyncTicks() - 1024;
     outp(0x43,0x30);
     }
   else outp(0x43,0x36);
   outp(0x40,TimerSpeed & 0xff);
   outp(0x40,TimerSpeed >> 8);
   }
 }


void dStartTimer(TimerProc Timer, int Speed)
{
 if (BiosTimer)
  {
   UserTimer = Timer;
   dSetTimerSpeed(Speed);
   }
 }


void dStopTimer(void)
{
 if (BiosTimer)
  {
   UserTimer = IdleTimer;
   dSetTimerSpeed(0x10000);
   }
 }

void timerstub2(void)
{
 }