/* Copyright 1996 by Robert Morgan of Channel 7
   Sound Interface */

#include <STDIO.H>
#include <STDLIB.H>
#include <STRING.H>
#include <IO.H>
#include <fcntl.h>
#include <sys/stat.h>
#include "d_global.h"
#include "d_misc.h"
#include "d_disk.h"
#include "d_video.h"
#include "protos.h"
#include "r_refdef.h"
#include "d_ints.h"


/**** CONSTANTS ****/

#define MAXEFFECTS    8
#define NUMTRACKS     effecttracks
#define MAXCACHESIZE  (MAXEFFECTS*2+4)
#define MAXSOUNDS     90
#define MAXSOUNDDIST  384
#define MSD           (MAXSOUNDDIST<<FRACBITS)
#define DEFAULTVRDIST  157286
#define DEFAULTVRANGLE 4

/**** VARIABLES ****/

bool   MusicPresent, MusicPlaying, MusicSwapChannels;
SoundCard SC;
int       MusicError, EffectChan, CurrentChan, FXLump;
int       MusicVol, effecttracks;


/**** FUNCTIONS ****/

int LoadSetup(SoundCard *SC, char *Filename)
{
    int Handle;
    if ((Handle = _open(Filename,O_RDONLY|O_BINARY)) < 0)
        return 1;
    if (read(Handle,SC,sizeof(SoundCard)) != sizeof(SoundCard)) {
        close(Handle);
        return 1;
    }
    close(Handle);
    return 0;
}


int SaveSetup(SoundCard *SC, char *Filename)
{
    int Handle;
    if ((Handle = _open(Filename,O_CREAT|O_WRONLY|O_BINARY,_S_IREAD | _S_IWRITE )) < 0)
        return 1;
    if (write(Handle,SC,sizeof(SoundCard)) != sizeof(SoundCard)) {
        close(Handle);
        return 1;
    }
    close(Handle);
    return 0;
}


void StopMusic(void)
{
 int i;

 if (MusicError)
  return;
 if (MusicPlaying)
  {
   if (!netmode)
    for(i=MusicVol;i>0;i-=3)              /* fade out */
     {
      if (i<0) i=0;
      //dSetMusicVolume(i);
      Wait(1);
      }
   //dStopMusic();
   MusicPlaying=false;
   //dFreeModule(M);
   }
 if (netmode)
  NetGetData();
 }


void InitSound(void)
{
 bool autodetect, noconfig;

 MusicPresent=false;
 autodetect=false;
 noconfig=false;
 if (LoadSetup(&SC,"SETUP.CFG"))     /* load config file */
  {
   noconfig=true;
   printf("Sound:\tSETUP.CFG not found\n");
   printf("\tAuto-Detection=");
   autodetect=false;

   SC.ambientlight=2048;      // load all defaults
   SC.violence=true;
   SC.animation=true;
   SC.musicvol=100;
   SC.sfxvol=128;
   SC.ckeys[0]=scanbuttons[bt_run];
   SC.ckeys[1]=scanbuttons[bt_jump];
   SC.ckeys[2]=scanbuttons[bt_straf];
   SC.ckeys[3]=scanbuttons[bt_fire];
   SC.ckeys[4]=scanbuttons[bt_use];
   SC.ckeys[5]=scanbuttons[bt_useitem];
   SC.ckeys[6]=scanbuttons[bt_asscam];
   SC.ckeys[7]=scanbuttons[bt_lookup];
   SC.ckeys[8]=scanbuttons[bt_lookdown];
   SC.ckeys[9]=scanbuttons[bt_centerview];
   SC.ckeys[10]=scanbuttons[bt_slideleft];
   SC.ckeys[11]=scanbuttons[bt_slideright];
   SC.ckeys[12]=scanbuttons[bt_invleft];
   SC.ckeys[13]=scanbuttons[bt_invright];
   SC.inversepan=false;
   SC.screensize=0;
   SC.camdelay=35;
   SC.effecttracks=4;
   SC.mouse=1;
   SC.joystick=0;

   SC.chartype=0;
   SC.socket=1234;
   SC.numplayers=2;
   SC.serplayers=1;
   SC.com=1;
   SC.rightbutton=bt_north;
   SC.leftbutton=bt_fire;
   SC.joybut1=bt_fire;
   SC.joybut2=bt_straf;
   strncpy(SC.dialnum,"           ",12);
   strncpy(SC.netname,"           ",12);
   SC.netmap=22;
   SC.netdifficulty=2;
   SC.mousesensitivity=32;
   SC.turnspeed=8;
   SC.turnaccel=2;

   SC.vrhelmet=0;
   SC.vrangle=DEFAULTVRANGLE;
   SC.vrdist=DEFAULTVRDIST;

   lighting=1;
   changelight=SC.ambientlight;

   if (autodetect)
    {
     printf("Failed\n");
     MusicError=1;
     return;
     }
   else printf("Success\n");
   }

 MusicSwapChannels=SC.inversepan;

 scanbuttons[bt_run]=SC.ckeys[0];
 scanbuttons[bt_jump]=SC.ckeys[1];
 scanbuttons[bt_straf]=SC.ckeys[2];
 scanbuttons[bt_fire]=SC.ckeys[3];
 scanbuttons[bt_use]=SC.ckeys[4];
 scanbuttons[bt_useitem]=SC.ckeys[5];
 scanbuttons[bt_asscam]=SC.ckeys[6];
 scanbuttons[bt_lookup]=SC.ckeys[7];
 scanbuttons[bt_lookdown]=SC.ckeys[8];
 scanbuttons[bt_centerview]=SC.ckeys[9];
 scanbuttons[bt_slideleft]=SC.ckeys[10];
 scanbuttons[bt_slideright]=SC.ckeys[11];
 scanbuttons[bt_invleft]=SC.ckeys[12];
 scanbuttons[bt_invright]=SC.ckeys[13];

 lighting=1;
 changelight=SC.ambientlight;
 playerturnspeed=SC.turnspeed;
 turnunit=SC.turnaccel;

 effecttracks=SC.effecttracks;

 MusicError=2;
 return;
 }

void PlaySong(char *sname,int pattern)
{
}


void SoundEffect(int n,int variation,fixed_t x,fixed_t y)
{
 }


void StaticSoundEffect(int n,fixed_t x,fixed_t y)
{
 }


void UpdateSound(void)
{
 }


void SetVolumes(int music,int fx)
{
 if (MusicError)
  return;
 if (music>255) music=255;
 //dSetMusicVolume(music);
 MusicVol=music;
 if (fx>255) fx=255;
 //dSetSoundVolume(fx);
 SC.musicvol=music;
 SC.sfxvol=fx;
 }
