/****************************************************************************
*
*                   Digital Sound Interface Kit (DSIK)
*                            Version 2.00
*
*                           by Carlos Hasan
*
* Filename:     detect.c
* Version:      Revision 1.1
*
* Language:     WATCOM C
* Environment:  IBM PC (DOS/4GW)
*
* Description:  Soundcards autodetection routines.
*
* Revision History:
* ----------------
*
* Revision 1.1  94/12/28  13:02:27  chv
* New Windows Sound System detection routine.
*
* Revision 1.0  94/11/20  18:02:36  chv
* Initial revision
*
****************************************************************************/

#include <i86.h>
#include <dos.h>
#include <env.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include "audio.h"

#define TRUE    1
#define FALSE   0

struct DPMIREGS {
    long    edi,esi,ebp,reserved,ebx,edx,ecx,eax;
    short   flags,es,ds,fs,gs,ip,cs,sp,ss;
};


/****************************************************************************
*
* Function:     SBDetect
* Parameters:   SC  - soundcard structure
*
* Returns:      Returns true if a Sound Blaster card is present.
*
* Description:  Autodetect soundcard device.
*
****************************************************************************/

static int SBDetect(SoundCard *SC)
{
    char *Blaster,*Ptr,Buffer[32];
    int Port,Irq,LowDma,HighDma,MidiPort,Type;

    /* The BLASTER environment variable parameter meaning:
	  A specifies the base address (0x220)
	  I specifies the IRQ (7)
	  D specifies the low DMA channel (1)
	  H specifies the high DMA channel (5)
	  P specifies the MIDI base address (0x330)
	  T specifies the version of the soundcard:
	      1 : Sound Blaster 1.0/1.5
	      2 : Sound Blaster Pro
	      3 : Sound Blaster 2.0/2.5
	      4 : Sound Blaster Pro 3/Pro 4.0
	      5 : Sound Blaster Pro (Microchannel)
	      6 : Sound Blaster 16
    */

    if (!(Blaster = getenv("BLASTER")))
	return FALSE;

    Port = 0x220;
    Irq = 7;
    LowDma = 1;
    HighDma = 5;
    MidiPort = 0x330;
    Type = 1;
    strupr(strncpy(Buffer,Blaster,sizeof(Buffer)));
    for (Ptr = strtok(Buffer," "); Ptr != NULL; Ptr = strtok(NULL," ")) {
	switch (*Ptr) {
	    case 'A':
		if (!sscanf(Ptr,"A%03X",&Port)) return FALSE;
		break;
	    case 'I':
		if (!sscanf(Ptr,"I%d",&Irq)) return FALSE;
		break;
	    case 'D':
		if (!sscanf(Ptr,"D%d",&LowDma)) return FALSE;
		break;
	    case 'H':
		if (!sscanf(Ptr,"H%d",&HighDma)) return FALSE;
		break;
	    case 'P':
		if (!sscanf(Ptr,"P%03X",&MidiPort)) return FALSE;
		break;
	    case 'T':
		if (!sscanf(Ptr,"T%d",&Type)) return FALSE;
		break;
	    /*
	    default:
		return FALSE;
	    */
	}
    }

    if (Type >= 6) {
	SC->ID = ID_SB16;
	SC->Modes = AF_16BITS | AF_STEREO;
    }
    else if (Type == 3) {
	SC->ID = ID_SB201;
	SC->Modes = AF_8BITS | AF_MONO;
    }
    else if ((Type == 2) || (Type == 4) || (Type == 5)) {
	SC->ID = ID_SBPRO;
	SC->Modes = AF_8BITS | AF_STEREO;
    }
    else {
	SC->ID = ID_SB;
	SC->Modes = AF_8BITS | AF_MONO;
    }

    SC->Port = Port;
    SC->IrqLine = Irq;
    SC->DmaChannel = (SC->Modes & AF_16BITS) ? HighDma : LowDma;
    SC->SampleRate = 22050;
    memcpy(SC->DriverName,"SB.DRV",6);
    return TRUE;
}

/****************************************************************************
*
* Function:     GUSDetect
* Parameters:   SC  - soundcard structure
*
* Returns:      Returns true if a UltraSound card is present.
*
* Description:  Autodetect soundcard device.
*
****************************************************************************/

static int GUSDetect(SoundCard *SC)
{
    char *Ultrasnd;
    int Port,PlayDma,RecDma,Gf1Irq,MidiIrq;

    if (!(Ultrasnd = getenv("ULTRASND")))
	return FALSE;

    if (sscanf(Ultrasnd,"%03X,%d,%d,%d,%d",
	&Port,&PlayDma,&RecDma,&Gf1Irq,&MidiIrq) != 5)
	return FALSE;

    SC->ID = ID_GUS;
    SC->Modes = AF_16BITS | AF_STEREO;
    SC->Port = Port;
    SC->IrqLine = Gf1Irq;
    SC->DmaChannel = PlayDma;
    SC->SampleRate = 44100;
    memcpy(SC->DriverName,"GUS.DRV",7);
    return TRUE;
}

/****************************************************************************
*
* Function:     PASProbe
* Parameters:   Port    - base I/O port address
*
* Returns:      Returns true if a Pro Audio Spectrum is there.
*
* Description:  Autodetect soundcard device.
*
****************************************************************************/

static int PASProbe(int Port)
{
    int Data;
    if ((Data = inp(Port ^ 0x388 ^ 0xB8B)) == 0xFF) return FALSE;
    outp(Port ^ 0x388 ^ 0xB8B, Data ^ 0xE0);
    return (inp(Port ^ 0x388 ^ 0xB8B) == Data);
}

/****************************************************************************
*
* Function:     PASDetect
* Parameters:   SC  - soundcard structure
*
* Returns:      Returns true if a Pro Audio Spectrum is present.
*
* Description:  Autodetect soundcard device.
*
****************************************************************************/

static int PASDetect(SoundCard *SC)
{
    union REGS regs;
    struct SREGS sregs;
    struct DPMIREGS dregs;
    int Port,Irq,Dma;

    memset((void*)&regs,0,sizeof(regs));
    memset((void*)&sregs,0,sizeof(sregs));
    memset((void*)&dregs,0,sizeof(dregs));

    /* detect if MVSOUND.SYS is present */
    regs.w.ax = 0x0300;
    regs.h.bl = 0x2F;
    regs.h.bh = 0;
    regs.w.cx = 0;
    regs.x.edi = FP_OFF(&dregs);
    sregs.es = FP_SEG(&dregs);
    dregs.eax = 0xBC00;
    dregs.ebx = 0x3F3F;
    dregs.ecx = 0x0000;
    dregs.edx = 0x0000;
    int386x(0x31,&regs,&regs,&sregs);
    if (((dregs.ebx ^ dregs.ecx ^ dregs.edx) & 0xFFFF) != 0x4D56)
	return FALSE;

    /* call MVSOUND.SYS to ask for the IRQ and DMA parameters */
    regs.w.ax = 0x0300;
    regs.h.bl = 0x2F;
    regs.h.bh = 0;
    regs.w.cx = 0;
    regs.x.edi = FP_OFF(&dregs);
    sregs.es = FP_SEG(&dregs);
    dregs.eax = 0xBC04;
    int386x(0x31,&regs,&regs,&sregs);
    Irq = dregs.ecx & 0xFF;
    Dma = dregs.ebx & 0xFF;

    /* detect the I/O Port address */
    if (!PASProbe(Port = 0x388) && !PASProbe(Port = 0x384) &&
	!PASProbe(Port = 0x38C) && !PASProbe(Port = 0x288))
	return FALSE;

    SC->ID = ID_PAS;
    SC->Modes = AF_8BITS | AF_STEREO;
    SC->Port = Port;
    SC->IrqLine = Irq;
    SC->DmaChannel = Dma;
    SC->SampleRate = 22050;
    memcpy(SC->DriverName,"PAS.DRV",7);
    return TRUE;
}

/****************************************************************************
*
* Function:     WSSProbe
* Parameters:   Port    - base I/O port address
*
* Returns:      Returns true if a Windows Sound System is there.
*
* Description:  Autodetect soundcard device.
*
****************************************************************************/

static int WSSProbe(int Port)
{
    int data;
    if (!(inp(Port+0x04) & 0x80)) {
	outp(Port+0x04,0x0c);
	data = inp(Port+0x05);
	outp(Port+0x05,0x00);
	if ((inp(Port+0x04) == 0x0c) && (inp(Port+0x05) == data))
	    return TRUE;
    }
    return FALSE;
}

/****************************************************************************
*
* Function:     WSSDetect
* Parameters:   SC  - soundcard structure
*
* Returns:      Returns true if a Windows Sound System card is present.
*
* Description:  Autodetect soundcard device.
*
****************************************************************************/

static int WSSDetect(SoundCard *SC)
{
    int Port;
    if (!WSSProbe(Port = 0x530) && !WSSProbe(Port = 0x604) &&
	!WSSProbe(Port = 0xE80) && !WSSProbe(Port = 0xF40))
	return FALSE;
    SC->ID = ID_WSS;
    SC->Modes = AF_16BITS | AF_STEREO;
    SC->Port = Port;
    SC->IrqLine = 7;
    SC->DmaChannel = 1;
    SC->SampleRate = 22050;
    memcpy(SC->DriverName,"WSS.DRV",7);
    return TRUE;
}


/****************************************************************************
*
* Function:     dAutoDetect
* Parameters:   SC  - Soundcard structure
*
* Description:  Autodetect soundcard device.
*
****************************************************************************/

int dAutoDetect(SoundCard *SC)
{
    if (!GUSDetect(SC) && !PASDetect(SC) &&
	    !SBDetect(SC) && !WSSDetect(SC)) {
	SC->ID = ID_NONE;
	SC->Modes = AF_8BITS | AF_MONO;
	SC->Port = SC->SampleRate = SC->IrqLine = SC->DmaChannel = 0;
    }
    return (SC->ID == ID_NONE);
}
