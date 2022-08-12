/****************************************************************************
*
*                   Digital Sound Interface Kit (DSIK)
*                            Version 2.00
*
*                           by Carlos Hasan
*
* Filename:     play.c
* Version:      Revision 1.0
*
* Language:     WATCOM C
* Environment:  IBM PC (DOS/4GW)
*
* Description:  DSIK Module Player.
*
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <direct.h>
#include <ctype.h>
#include <conio.h>
#include <i86.h>
#include "audio.h"
#include "import.h"
#include "timer.h"

#define KB_FF       0x4D00
#define KB_RW       0x4B00
#define KB_ESC      0x001B
#define KB_PAUSE    0x0020
#define KB_SHELL    0x4200
#define KB_NEXT     0x000D
#define KB_PLUS     0x002B
#define KB_MINUS    0x002D
#define KB_INCSPD   0x007D
#define KB_DECSPD   0x007B
#define KB_TRACE    0x0054
#define KB_NOTRACE  0x0050

char *keytable = "123456789ABCDEFG";

volatile int ticks;


/* TheDraw C Crunched Screen Image */
#define WIDTH   80
#define HEIGHT  50
#define IMGSIZ  962

//#define PROFILE

unsigned char image[] = {
   "\x0F\x11 DSIK Module Player Version 2.00\x19\x0E""Copyright (C) 1"
   "995 Carlos Hasan  \x18\x09\x1AO±\x18±ÉÍSong Information:\x1A\x15Í"
   "»±±ÉÍPlayback Device:\x1A\x0DÍ»±±\x18±º Songname:\x19\x1Dº°±º\x19"
   "\x1Eº°±\x18±º Filename:\x19\x0E""CPU used:\x19\x05º°±Ç\x1A\x1EÄ¶°"
   "±\x18±Ç\x1A\x27Ä¶°±º\x19\x1Eº°±\x18±º Patt:Beat:Tick:\x19\x0ATemp"
   "o:\x19\x06º°±Ç\x1A\x1EÄ¶°±\x18±º Position:\x19\x08Pattern:\x19\x04"
   "Line:\x19\x02º°±º I/O Port:\x19\x05IRQ:\x19\x03""DMA:\x19\x02º°±\x18"
   "±È\x1A\x27Í¼°±È\x1A\x1EÍ¼°±\x18±±\x1A)°±±\x1A °±\x18\x1AO±\x18±ÉÍ"
   "Song Tracks:\x1A\x1AÍ»±±ÉÍVU Meters:\x1A\x13Í»±±\x18±º 01\x19$º°±"
   "º\x19\x1Eº°±\x18±º 02\x19$º°±º\x19\x1Eº°±\x18±º 03\x19$º°±º\x19\x1E"
   "º°±\x18±º 04\x19$º°±º\x19\x1Eº°±\x18±º 05\x19$º°±º\x19\x1Eº°±\x18"
   "±º 06\x19$º°±º\x19\x1Eº°±\x18±º 07\x19$º°±º\x19\x1Eº°±\x18±º 08\x19"
   "$º°±º\x19\x1Eº°±\x18±º 09\x19$º°±º\x19\x1Eº°±\x18±º 10\x19$º°±º\x19"
   "\x1Eº°±\x18±º 11\x19$º°±º\x19\x1Eº°±\x18±º 12\x19$º°±º\x19\x1Eº°±"
   "\x18±º 13\x19$º°±º\x19\x1Eº°±\x18±º 14\x19$º°±º\x19\x1Eº°±\x18±º "
   "15\x19$º°±º\x19\x1Eº°±\x18±º 16\x19$º°±º\x19\x1Eº°±\x18±È\x1A\x27"
   "Í¼°±È\x1A\x1EÍ¼°±\x18±±\x1A)°±±\x1A °±\x18\x1AO±\x18±ÉÍInstrument"
   " Names:\x1A""8Í»±±\x18±º 01\x19\x27""17\x19\x1Dº°±\x18±º 02\x19\x27"
   "18\x19\x1Dº°±\x18±º 03\x19\x27""19\x19\x1Dº°±\x18±º 04\x19\x27""2"
   "0\x19\x1Dº°±\x18±º 05\x19\x27""21\x19\x1Dº°±\x18±º 06\x19\x27""22"
   "\x19\x1Dº°±\x18±º 07\x19\x27""23\x19\x1Dº°±\x18±º 08\x19\x27""24\x19"
   "\x1Dº°±\x18±º 09\x19\x27""25\x19\x1Dº°±\x18±º 10\x19\x27""26\x19\x1D"
   "º°±\x18±º 11\x19\x27""27\x19\x1Dº°±\x18±º 12\x19\x27""28\x19\x1Dº"
   "°±\x18±º 13\x19\x27""29\x19\x1Dº°±\x18±º 14\x19\x27""30\x19\x1Dº°"
   "±\x18±º 15\x19\x27""31\x19\x1Dº°±\x18±º 16\x19\x27""32\x19\x1Dº°±"
   "\x18±È\x1AJÍ¼°±\x18±±\x1AL°±\x18"};


#define VPTR(x,y) (void*)(0xb8000+2*(x)+160*(y))

void setmode80x25(void)
{
    union REGS r;
    r.w.ax = 0x0003;
    int386(0x10,&r,&r);
}

void setmode80x50(void)
{
    union REGS r;
    r.w.ax = 0x0003;
    int386(0x10,&r,&r);
    r.w.ax = 0x1112;
    r.h.bl = 0x00;
    int386(0x10,&r,&r);
}

void waitvr(void)
{
    while (!(inp(0x3da) & 8)) ;
    while (inp(0x3da) & 8) ;
}

void hidecursor(void)
{
    outpw(0x3d4,0x100a);
    outpw(0x3d4,0x100b);
}

void setborder(int color)
{
    inp(0x3da);
    outp(0x3c0,0x31);
    outp(0x3c0,color);
}

void drawtext(int x, int y, char *text, int color)
{
    short *ptr = (short*)VPTR(x,y);
    color <<= 8;
    while (*text) {
        *ptr++ = *text++ | color;
    }
}

void drawchars(int x, int y, int c, int count, int color)
{
    short *ptr = (short*)VPTR(x,y);
    c |= color << 8;
    while (count--) *ptr++ = c;
}

void drawimage(unsigned char *image, int len)
{
    unsigned char *endimage,*p,*line;
    unsigned int c,a,x,y;

    a = 0;
    p = line = (unsigned char *)VPTR(0,0);
    endimage = image + len;
    while (image < endimage) {
        if ((c = *image++) >= 32) {
            *p++ = c;
            *p++ = a;
        }
        else {
            if (c <= 15) {
                a = (a & 0xf0) | (c & 0x0f);
            }
            else if (c >= 16 && c <= 23) {
                a = (a & 0x8f) | ((c & 0x07) << 4);
            }
            else if (c == 24) {
                line += (WIDTH<<1);
                p = line;
            }
            else if (c == 25) {
                x = *image++;
                do {
                    *p++ = 0x20;
                    *p++ = a;
                } while (x--);
            }
            else if (c == 26) {
                x = *image++;
                y = *image++;
                do {
                    *p++ = y;
                    *p++ = a;
                } while (x--);
            }
            else if (c == 27) {
                a ^= 0x80;
            }
        }
    }
}

void starttimer(void)
{
    outp(0x61,inp(0x61) | 0x01);
    outp(0x43,0xb4);
    outp(0x42,0xff);
    outp(0x42,0xff);
}

int readtimer(void)
{
    int n;
    outp(0x43,0xb0);
    n = inp(0x42);
    n += inp(0x42) << 8;
    return 0x10000L - n;
}

void timer(void)
{
    #ifdef PROFILE
    setborder(15);
    #endif
    starttimer();
    dPoll();
    ticks = (ticks+readtimer())>>1;
    #ifdef PROFILE
    setborder(0);
    #endif
}

void initscreen(DSM *M, SoundCard *SC, char *path)
{
    char buf[40];
    int i;

    setmode80x50();
    hidecursor();
    drawimage(image,IMGSIZ);
    drawtext(13,3,M->Header.ModuleName,0x13);
    drawtext(13,4,path,0x13);
    drawtext(47,3,dGetDriverStruc(SC->ID)->Name,0x13);
    sprintf(buf,"%d-bit %s at %d hertz",
        SC->Modes & AF_16BITS ? 16 : 8,
        SC->Modes & AF_STEREO ? "stereo" : "mono",
        SC->SampleRate);
    drawtext(47,5,buf,0x13);
    sprintf(buf,"%03Xh",SC->Port);
    drawtext(56,7,buf,0x13);
    sprintf(buf,"%d",SC->IrqLine);
    drawtext(66,7,buf,0x13);
    sprintf(buf,"%d",SC->DmaChannel);
    drawtext(74,7,buf,0x13);
    for (i = 0; i < 16; i++) {
        if (i < M->Header.NumSamples)
            drawtext(6,32+i,M->Samples[i]->SampleName,0x13);
        if (i+16 < M->Header.NumSamples)
            drawtext(48,32+i,M->Samples[i+16]->SampleName,0x13);
    }
}

void donescreen(void)
{
    setmode80x25();
}

void updatescreen(DSM *M, MHdr *P)
{
    static char buf[80];
    static char notes[] =
        "C-n\0C#n\0D-n\0D#n\0E-n\0F-n\0F#n\0G-n\0G#n\0A-n\0A#n\0B-n\0";
    MTrk *Trk;
    char *p;
    int i,n;

    sprintf(buf,"%2d.%1d%%",(100*ticks)/TICKS(70),
        ((1000*ticks)/TICKS(70))%10);
    drawtext(36,4,buf,0x13);
    if (P->BreakFlag != PB_JUMP) {
        sprintf(buf,"%03d:%02d:%02d",P->OrderPos,P->PattRow>>4,P->PattRow&15);
        drawtext(18,6,buf,0x13);
        sprintf(buf,"%02d/%03d",P->Tempo,P->BPM);
        drawtext(35,6,buf,0x13);
        sprintf(buf,"%03d/%03d",P->OrderPos,P->OrderLen);
        drawtext(12,7,buf,0x13);
        sprintf(buf,"%03d",P->PattNum);
        drawtext(29,7,buf,0x13);
        sprintf(buf,"%02d",P->PattRow);
        drawtext(39,7,buf,0x13);
    }
    for (i = 0, Trk = P->Tracks; i < MAXTRACKS; i++, Trk++) {
        if (!(n = Trk->Note))
            drawtext(6,12+i,"---",0x19);
        else {
            n--;
            notes[4*(n % 12)+2] = '0' + (n / 12);
            drawtext(6,12+i,&notes[4*(n % 12)],0x19);
        }
        sprintf(buf,"%02d",Trk->Volume);
        drawtext(10,12+i,buf,0x19);
        if (((n = Trk->Sample) != 0) && (n <= M->Header.NumSamples)) {
            n--;
            p = M->Samples[n]->SampleName;
            n = strlen(p);
            drawtext(13,12+i,p,0x19);
            drawchars(13+n,12+i,0x20,28-n,0x19);
        }
        if ((n = (29*Trk->VUMeter)>>6) > 29) n = 29;
        drawchars(47,12+i,0xfe,n,0x1b);
        drawchars(47+n,12+i,0xfe,29-n,0x19);
        if (Trk->Flags & AM_PAUSE) drawchars(5,12+i,0x4d,1,0x1b);
        else drawchars(5,12+i, dGetVoiceStatus(i) == PS_PLAYING ? 0x07:0x20,1,0x15);
    }
}


int main(int argc, char *argv[])
{
    SoundCard SC;
    DSM *M;
    MHdr *P;
    DIR *d;
    char path[_MAX_PATH];
    char drive[_MAX_DRIVE],dir[_MAX_DIR],name[_MAX_NAME],ext[_MAX_EXT];
    int i,j,c,form;

    printf("DSIK Module Player Version 2.00 (C) 1995 Carlos Hasan\n");
    if (argc < 2) {
        printf("Use: PLAY modfile[.mod|.nst|.s3m|.mtm|.669|.stm|.dsm]\n");
        exit(EXIT_FAILURE);
    }
    _splitpath(argv[0],drive,dir,name,ext);
    _makepath(path,drive,dir,"SETUP",".CFG");

    if (dLoadSetup(&SC,path)) {
        printf("Please run SETUP.EXE to configure.\n");
        exit(EXIT_FAILURE);
    }
    dRegisterDrivers();
    printf("%s at Port %03Xh using IRQ %d on DMA channel %d\n",
        dGetDriverStruc(SC.ID)->Name, SC.Port, SC.IrqLine, SC.DmaChannel);
    if (dInit(&SC)) {
        printf("Error initializing the sound system.\n");
        exit(EXIT_FAILURE);
    }
    atexit((void(*)(void))dDone);

    if (dMemAvail())
       printf("There are %lu free bytes on the soundcard.\n", dMemAvail());

    dInitTimer();
    atexit((void(*)(void))dDoneTimer);
    dStartTimer(timer,TICKS(70));

    for (i = 1; i < argc; i++) {
        strupr(argv[i]);
        _splitpath(argv[i],drive,dir,name,ext);
        if (*ext == 0) strcpy(ext,".*");
        _makepath(path,drive,dir,name,ext);
        if (!(d = opendir(path)))
            printf("File not found: %s\n",path);
        else {
            printf("Searching module file.\n");
            while ((d = readdir(d)) != NULL) {
                if (!(d->d_attr & _A_ARCH)) continue;
                _makepath(path,drive,dir,d->d_name,"");
                _splitpath(path,drive,dir,name,ext);
                if (!strcmp(ext,".DSM")) form = FORM_DSM;
                else if (!strcmp(ext,".MOD")) form = FORM_MOD;
                else if (!strcmp(ext,".NST")) form = FORM_MOD;
                else if (!strcmp(ext,".S3M")) form = FORM_S3M;
                else if (!strcmp(ext,".MTM")) form = FORM_MTM;
                else if (!strcmp(ext,".669")) form = FORM_669;
                else if (!strcmp(ext,".STM")) form = FORM_STM;
                else {
                    printf("Unknown file format: %s\n", path);
                    exit(EXIT_FAILURE);
                }
                printf("Loading module: %s\n",path);
                if (!(M = dImportModule(path,form))) {
                    printf("Error (%03d) loading %s module file: %s.\n",
                        dError, path, dErrorMsg[dError]);
                    exit(EXIT_FAILURE);
                }

                initscreen(M,&SC,d->d_name);
                dSetupVoices(M->Header.NumTracks,M->Header.MasterVolume);
                dPlayMusic(M);
                P = dGetMusicStruc();
                while (dGetMusicStatus() != PS_STOPPED) {
                    updatescreen(M,P);
                    if (kbhit()) {
                        if (!(c = toupper(getch()))) c = getch()<<8;
                        if (c == KB_ESC) {
                            break;
                        }
                        else if (c == KB_PAUSE) {
                            if (dGetMusicStatus() == PS_PAUSED) dResumeMusic();
                            else dPauseMusic();
                        }
                        else if (c == KB_FF) {
                            if (P->OrderPos < P->OrderLen) (P->OrderPos)++;
                            P->PattRow = 0;
                            P->BreakFlag = PB_JUMP;
                        }
                        else if (c == KB_RW) {
                            if (P->OrderPos > 0) (P->OrderPos)--;
                            P->PattRow = 0;
                            P->BreakFlag = PB_JUMP;
                        }
                        else if (c == KB_SHELL) {
                            donescreen();
                            spawnl(P_WAIT,getenv("COMSPEC"),getenv("COMSPEC"),NULL);
                            initscreen(M,&SC,d->d_name);
                        }
                        else if (c == KB_NEXT) {
                            break;
                        }
                        else if (c == KB_PLUS) {
                            if (P->MusicVolume <= 251) P->MusicVolume += 4;
                            else P->MusicVolume = 255;
                            dSetMusicVolume(P->MusicVolume);
                        }
                        else if (c == KB_MINUS) {
                            if (P->MusicVolume >= 4) P->MusicVolume -= 4;
                            else P->MusicVolume = 0;
                            dSetMusicVolume(P->MusicVolume);
                        }
                        else if (c == KB_INCSPD) {
                            if (P->Tempo < 31) (P->Tempo)++;
                        }
                        else if (c == KB_DECSPD) {
                            if (P->Tempo > 1) (P->Tempo)--;
                        }
                        else if (c == KB_TRACE) {
                            P->BreakFlag = PB_TRACE;
                        }
                        else if (c == KB_NOTRACE) {
                            P->BreakFlag = PB_NONE;
                        }
                        else {
                            for (j = 0; j < MAXTRACKS; j++) {
                                if (c == keytable[j]) {
                                    P->Tracks[j].Flags ^= AM_PAUSE;
                                    dStopVoice(j);
                                }
                            }
                        }
                    }
                }
                dStopMusic();
                donescreen();
                dFreeModule(M);
                if (c == KB_ESC) exit(EXIT_FAILURE);
                if (dMemAvail())
                   printf("There are %lu free bytes on the soundcard.\n", dMemAvail());
            }
            closedir(d);
        }
    }
    return 0;
}
