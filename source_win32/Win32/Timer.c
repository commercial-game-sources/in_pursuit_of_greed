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

#include <windows.h>
#include "timer.h"

void	(*User_Timer)(void);
UINT	Timer_Event;


void CALLBACK TimerHandler(
	UINT uTimerID, 
	UINT uMsg, 
	DWORD dwUser, 
	DWORD dw1, 
	DWORD dw2 )
{
	(*User_Timer)();
}


void dStopTimer(void)
{
	timeKillEvent(Timer_Event);
}


void dStartTimer(TimerProc timer,int rate)
{
	User_Timer = timer;
	Timer_Event = timeSetEvent(1000 / rate,10,TimerHandler,0,TIME_PERIODIC);
}

