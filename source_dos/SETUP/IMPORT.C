/****************************************************************************
*
*                   Digital Sound Interface Kit (DSIK)
*                            Version 2.00
*
*                           by Carlos Hasan
*
* Filename:     import.c
* Version:      Revision 1.3
*
* Language:     WATCOM C
* Environment:  IBM PC (DOS/4GW)
*
* Description:  Import routines for module and sample files.
*
* Revision History:
* ----------------
*
* Revision 1.3  94/11/17  16:58:20  chv
* Added Composer 669 and Scream Tracker 2.0 import routines
*
* Revision 1.2  94/11/08  15:10:42  chv
* Added Amiga IFF/8SVX sample file import routines
*
* Revision 1.1  94/10/26  13:12:43  chv
* Added Multitracker MTM modules import routines
*
* Revision 1.0  94/10/03  12:50:22  chv
* Initial revision
*
****************************************************************************/

#include <io.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <sys\stat.h>
#include "audio.h"
#include "import.h"


/*--------------- Protracker/Fastracker Import Routines -------------------*/

static word PeriodTable[96] = {
    6848,6464,6096,5760,5424,5120,4832,4560,4304,4064,3840,3624,
    3424,3232,3048,2880,2712,2560,2416,2280,2152,2032,1920,1812,
    1712,1616,1524,1440,1356,1280,1208,1140,1076,1016,960,906,
    856,808,762,720,678,640,604,570,538,508,480,453,
    428,404,381,360,339,320,302,285,269,254,240,226,
    214,202,190,180,170,160,151,143,135,127,120,113,
    107,101,95,90,85,80,75,71,67,63,60,56,
    53,50,47,45,42,40,37,35,33,31,30,28 };

static word RateTable[16] = {
    8363,8424,8485,8546,8608,8670,8733,8797,
    7894,7951,8008,8066,8125,8184,8243,8303 };

/****************************************************************************
*
* Function:     MODLoadPattern
* Parameters:   Handle      - file handle
*               NumTracks   - number of tracks
*
* Returns:      Pattern structure or NULL if error.
*
* Description:  Used to load MOD patterns.
*
****************************************************************************/

static Pattern *MODLoadPattern(int Handle, int NumTracks)
{
    Pattern *DPatt;
    byte *SPatt;
    byte *S,*D;
    byte Row,Track,Flags,Note,Inst,Effect,Param;
    word Period;

    if (!(DPatt = (Pattern*)malloc(2+(1+5*NumTracks)*64))) {
        dError = ERR_NOMEM;
        return NULL;
    }
    if (!(SPatt = (byte*)malloc(NumTracks<<8))) {
        dError = ERR_NOMEM;
        free(DPatt);
        return NULL;
    }
    if (read(Handle,SPatt,NumTracks<<8) != (NumTracks<<8)) {
        dError = ERR_FILEIO;
        free(SPatt);
        free(DPatt);
        return NULL;
    }
    S = SPatt;
    D = DPatt->Data;
    for (Row = 0; Row < 64; Row++) {
        for (Track = 0; Track < NumTracks; Track++) {
            Period = (((word)S[0] & 0x0F) << 8) | ((word)S[1]);
            Inst = (S[0] & 0xF0) | ((S[2] & 0xF0)>>4);
            Effect = S[2] & 0x0F;
            Param = S[3];
            S += 4;
            for (Note = 0; Note < 96; Note++)
                if (Period >= PeriodTable[Note]) break;
            if (Note >= 96) Note = 0; else Note++;
            Flags = 0;
            if (Note) Flags |= 0x80;
            if (Inst) Flags |= 0x40;
            if (Effect | Param) Flags |= 0x10;
            if (Flags) {
                *D++ = Flags | (Track & 0x0F);
                if (Note) *D++ = Note;
                if (Inst) *D++ = Inst;
                if (Effect | Param) {
                    *D++ = Effect;
                    *D++ = Param;
                }
            }
        }
        *D++ = 0;
    }
    free(SPatt);
    DPatt->Length = D-(byte*)DPatt;
    return (Pattern*)realloc(DPatt,DPatt->Length);
}

/****************************************************************************
*
* Function:     MODLoadSample
* Parameters:   Handle  - file handle
*               Instr   - MOD sample structure
*
* Returns:      Sample structure or NULL if error.
*
* Description:  Used to load MOD samples.
*
****************************************************************************/

static Sample *MODLoadSample(int Handle, MODSample *Instr)
{
    Sample *SampPtr;

    Instr->Length = WSWAP(Instr->Length) << 1;
    Instr->LoopStart = WSWAP(Instr->LoopStart) << 1;
    Instr->LoopLength = WSWAP(Instr->LoopLength) << 1;
    if (!(SampPtr = dLoadSampleFile(Handle,Instr->Length,SF_8BITS|SF_SIGNED)))
        return NULL;
    strncpy(SampPtr->SampleName,Instr->SampleName,sizeof(Instr->SampleName));
    SampPtr->Volume = Instr->Volume;
    SampPtr->Rate = RateTable[Instr->Finetune & 0x0F];
    if (Instr->LoopLength > 2) {
        SampPtr->Flags |= SF_LOOPED;
        SampPtr->LoopStart = SampPtr->LoopEnd = Instr->LoopStart;
        SampPtr->LoopEnd += Instr->LoopLength;
        if (SampPtr->LoopEnd >= SampPtr->Length)
            SampPtr->LoopEnd = SampPtr->Length;
        if (SampPtr->LoopStart >= SampPtr->LoopEnd)
            SampPtr->LoopStart = SampPtr->LoopEnd;
    }
    return SampPtr;
}

/****************************************************************************
*
* Function:     MODLoadModule
* Parameters:   Handle      - DOS file handle
*               Length      - file length
*
* Returns:      Music module address or NULL when an error has occurred
*               while loading the file.
*
* Description:  Load Protracker/Fastracker (MOD) music modules from disk.
*
****************************************************************************/

static DSM *MODLoadModule(int Handle, long Length)
{
    DSM *Module;
    MODSong Header;
    int Index;

    if (!(Module = (DSM*)calloc(1,sizeof(DSM)))) {
        dError = ERR_NOMEM;
        return NULL;
    }
    if (read(Handle,&Header,sizeof(Header)) != sizeof(Header)) {
        dError = ERR_FILEIO;
        dFreeModule(Module);
        return NULL;
    }
    if (Header.Magic == MOD_MK || Header.Magic == MOD_FLT4) {
        Module->Header.NumTracks = 4;
    }
    else if (Header.Magic == MOD_6CHN) {
        Module->Header.NumTracks = 6;
    }
    else if (Header.Magic == MOD_8CHN || Header.Magic == MOD_FLT8) {
        Module->Header.NumTracks = 8;
    }
    else {
        dError = ERR_FORMAT;
        dFreeModule(Module);
        return NULL;
    }

    /* fill the SONG structure */
    strncpy(Module->Header.ModuleName,Header.SongName,sizeof(Header.SongName));
    Module->Header.OrderPos = 0;
    Module->Header.ReStart = Header.ReStart;
    Module->Header.NumOrders = Header.NumOrders;
    Module->Header.NumSamples = 31;
    Module->Header.NumPatterns = 0;
    Module->Header.GlobalVolume = 64;
    Module->Header.MasterVolume = 768/Module->Header.NumTracks;
    Module->Header.InitTempo = 6;
    Module->Header.InitBPM = 125;
    for (Index = 0; Index < 128; Index++) {
        Module->Header.Orders[Index] = Header.Orders[Index];
        if (Module->Header.NumPatterns < Header.Orders[Index])
            Module->Header.NumPatterns = Header.Orders[Index];
    }
    (Module->Header.NumPatterns)++;
    for (Index = 0; Index < Module->Header.NumTracks; Index++) {
        Module->Header.ChanMap[Index] = ((Index & 3) == 0 ||
                (Index & 3) == 3) ? PAN_LEFT : PAN_RIGHT;
    }

    /* allocate sample and pattern pointer tables */
    if (!(Module->Samples = (Sample**)calloc(Module->Header.NumSamples,sizeof(Sample *)))) {
        dError = ERR_NOMEM;
        dFreeModule(Module);
        return NULL;
    }
    if (!(Module->Patterns = (Pattern**)calloc(Module->Header.NumPatterns,sizeof(Pattern *)))) {
        dError = ERR_NOMEM;
        dFreeModule(Module);
        return NULL;
    }

    /* fill the PATT structures */
    for (Index = 0; Index < Module->Header.NumPatterns; Index++) {
        if (!(Module->Patterns[Index] =
                MODLoadPattern(Handle,Module->Header.NumTracks))) {
            dFreeModule(Module);
            return NULL;
        }
    }

    /* fill the INST structures */
    for (Index = 0; Index < Module->Header.NumSamples; Index++) {
        if (!(Module->Samples[Index] =
                MODLoadSample(Handle,&Header.Samples[Index]))) {
            dFreeModule(Module);
            return NULL;
        }
    }

    return Module;
}

/*----------------- Scream Tracker 3.0 Import Routines --------------------*/

/****************************************************************************
*
* Function:     S3MLoadPattern
* Parameters:   Handle  - file handle
*               SeekPos - pattern relative file position
*               ChanMap - 32 channels remap table
*
* Returns:      Pattern structure or NULL if error.
*
* Description:  Used to load S3M patterns.
*
****************************************************************************/

static Pattern *S3MLoadPattern(int Handle, long SeekPos, byte *ChanMap)
{
    Pattern *DPatt;
    byte *SPatt;
    byte TrackParam[MAXTRACKS];
    byte *S,*D,*MaxD;
    word Length;
    byte Row,Flags,Track,Note,Inst,Volume,Effect,Param,PattLoop;

    if (!(DPatt = (Pattern*)malloc(70+2+(1+6*MAXTRACKS)*64))) {
        dError = ERR_NOMEM;
        return NULL;
    }
    if (lseek(Handle,SeekPos,SEEK_SET) < 0) {
        dError = ERR_FILEIO;
        free(DPatt);
        return NULL;
    }
    if (read(Handle,&Length,sizeof(Length)) != sizeof(Length)) {
        dError = ERR_FILEIO;
        free(DPatt);
        return NULL;
    }
    if (!(SPatt = (byte*)malloc(Length -= 2))) {
        dError = ERR_NOMEM;
        free(DPatt);
        return NULL;
    }
    if (read(Handle,SPatt,Length) != Length) {
        dError = ERR_FILEIO;
        free(SPatt);
        free(DPatt);
        return NULL;
    }
    for (Track = PattLoop = 0; Track < MAXTRACKS; Track++) {
        TrackParam[Track] = 0;
    }
    S = SPatt;
    D = DPatt->Data;
    MaxD = D+2+(1+6*MAXTRACKS)*64;
    for (Row = 0; Row < 64; Row++) {
        while (D < MaxD) {
            if (!(Flags = *S++)) break;
            Note = 255;
            Inst = 0;
            Volume = 255;
            Effect = 0;
            Param = 0;
            Track = ChanMap[Flags & 0x1F];
            if (Flags & 0x20) {
                Note = *S++;
                Inst = *S++;
            }
            if (Flags & 0x40) {
                Volume = *S++;
            }
            if (Flags & 0x80) {
                Effect = *S++;
                Param = *S++;
            }
            if (Track >= MAXTRACKS) continue;
            if (Note == 0xFE) {
                Note = Volume = 0;
            }
            else if (Note == 0xFF) {
                Note = 0;
            }
            else {
                Note = (Note & 0x0F) + 12*(Note >> 4);
                if (Note >= 96) Note = 95; else Note++;
            }
            if (Param) TrackParam[Track] = Param;
            Effect += 64;
            if ((Param == 0) && (((Effect >= 'D') && (Effect <= 'G')) ||
                (Effect == 'K') || (Effect == 'L') || (Effect == 'Q'))) {
                Param = TrackParam[Track];
            }
            switch (Effect) {
            case 'A':
                if (Param <= 0x1F) Effect = 0x0F;
                else Effect = Param = 0;
                break;
            case 'B':
                Effect = 0x0B;
                break;
            case 'C':
                Effect = 0x0D;
                break;
            case 'D':
                if ((Param & 0xF0) == 0x00) {
                    Effect = 0x0A;
                    Param &= 0x0F;
                }
                else if ((Param & 0x0F) == 0x00) {
                    Effect = 0x0A;
                    Param &= 0xF0;
                }
                else if ((Param & 0xF0) == 0xF0) {
                    Effect = 0x0E;
                    Param = 0xB0 | (Param & 0x0F);
                }
                else if ((Param & 0x0F) == 0x0F) {
                    Effect = 0x0E;
                    Param = 0xA0 | (Param >> 4);
                }
                else Effect = Param = 0;
                break;
            case 'E':
                if ((Param & 0xF0) == 0xF0) {
                    Effect = 0x0E;
                    Param = 0x20 | (Param & 0x0F);
                }
                else if ((Param & 0xF0) == 0xE0) {
                    Effect = 0x0E;
                    Param = 0x20 | ((Param & 0x0F) >> 2);
                }
                else {
                    Effect = 0x02;
                }
                break;
            case 'F':
                if ((Param & 0xF0) == 0xF0) {
                    Effect = 0x0E;
                    Param = 0x10 | (Param & 0x0F);
                }
                else if ((Param & 0xF0) == 0xE0) {
                    Effect = 0x0E;
                    Param = 0x10 | ((Param & 0x0F) >> 2);
                }
                else {
                    Effect = 0x01;
                }
                break;
            case 'G':
                Effect = 0x03;
                break;
            case 'H':
                Effect = 0x04;
                break;
            case 'I':
                Effect = 0x0E;
                Param = 0x90 | (Param & 0x0F); /* emu tremor with retrig */
                break;
            case 'J':
                Effect = 0x00;
                break;
            case 'K':
                Effect = 0x06;
                break;
            case 'L':
                Effect = 0x05;
                break;
            case 'O':
                Effect = 0x09;
                break;
            case 'Q':
                Effect = 0x0A;  /* emu retrigvol with volslide */
                if ((Param >> 4) >= 8)
                    Param = (((Param >> 4)/(Param & 0x0F))+1) << 4;
                else
                    Param = (((Param >> 4)/(Param & 0x0F))+1);
                break;
            case 'R':
                Effect = 0x07;
                break;
            case 'S':
                switch (Param & 0xF0) {
                case 0x00:
                    Effect = 0x0E;
                    Param = 0x00 | (Param & 0x0F);
                    break;
                case 0x10:
                    Effect = 0x0E;
                    Param = 0x30 | (Param & 0x0F);
                    break;
                case 0x20:
                    Effect = 0x0E;
                    Param = 0x50 | (Param & 0x0F);
                    break;
                case 0x30:
                    Effect = 0x0E;
                    Param = 0x70 | (Param & 0x0F);
                    break;
                case 0x40:
                    Effect = 0x0E;
                    Param = 0x40 | (Param & 0x0F);
                    break;
                case 0x80:
                    Effect = 0x0E;
                    Param = 0x80 | (Param & 0x0F);
                    break;
                case 0xA0:
                    Effect = 0x08;
                    switch (Param & 0x0F) {
                    case 0x00:
                    case 0x02:
                        Param = PAN_LEFT;
                        break;
                    case 0x01:
                    case 0x03:
                        Param = PAN_RIGHT;
                        break;
                    case 0x04:
                        Param = (PAN_LEFT + PAN_MIDDLE)/2;
                        break;
                    case 0x05:
                        Param = (PAN_MIDDLE + PAN_RIGHT)/2;
                        break;
                    case 0x06:
                    case 0x07:
                        Param = PAN_MIDDLE;
                        break;
                    default:
                        Effect = Param = 0;
                        break;
                    }
                    break;
                case 0xB0:
                    if ((Param & 0x0F) == 0x00)
                        Param = PattLoop;
                    else {
                        PattLoop = Param & 0x0F;
                        Param = 0xB0;
                    }
                    Effect = 0x0E;
                    Param = 0x60 | (Param & 0x0F);
                    break;
                case 0xC0:
                    Effect = 0x0E;
                    Param = 0xC0 | (Param & 0x0F);
                    break;
                case 0xD0:
                    Effect = 0x0E;
                    Param = 0xD0 | (Param & 0x0F);
                    break;
                case 0xE0:
                    Effect = 0x0E;
                    Param = 0xE0 | (Param & 0x0F);
                    break;
                default:
                    Effect = Param = 0;
                    break;
                }
                break;
            case 'T':
                if (Param >= 0x20) Effect = 0x0F;
                else Effect = Param = 0;
                break;
            case 'X':
                Effect = 0x08;
                break;
            case 'Z':
                Effect = 0x0B;
                Param = 0x80 | Param;
                break;
            default:
                Effect = Param = 0;
                break;
            }
            Flags = 0;
            if (Note) Flags |= 0x80;
            if (Inst) Flags |= 0x40;
            if (Volume <= 64) Flags |= 0x20;
            if (Effect | Param) Flags |= 0x10;
            if (Flags) {
                *D++ = (Flags | (Track & 0x0F));
                if (Note) *D++ = Note;
                if (Inst) *D++ = Inst;
                if (Volume <= 64) *D++ = Volume;
                if (Effect | Param) {
                    *D++ = Effect;
                    *D++ = Param;
                }
            }
        }
        *D++ = 0x00;
    }
    free(SPatt);
    DPatt->Length = D-(byte*)DPatt;
    return (Pattern*)realloc(DPatt,DPatt->Length);
}

/****************************************************************************
*
* Function:     S3MLoadSample
* Parameters:   Handle  - file handle
*               SeekPos - sample relative file position
*
* Returns:      Sample structure or NULL if error.
*
* Description:  Used to load S3M samples.
*
****************************************************************************/

static Sample *S3MLoadSample(int Handle, long SeekPos)
{
    S3MSample Instr;
    Sample *SampPtr;

    if (lseek(Handle,SeekPos,SEEK_SET) < 0) {
        dError = ERR_FILEIO;
        return NULL;
    }
    if (read(Handle,&Instr,sizeof(Instr)) != sizeof(Instr)) {
        dError = ERR_FILEIO;
        return NULL;
    }
    if (Instr.Type == 1 && Instr.Magic != S3M_SCRS) {
        dError = ERR_FORMAT;
        return NULL;
    }
    if (Instr.Type != 1) {
        Instr.Type = Instr.Volume = Instr.Flags = Instr.Rate = 0;
        Instr.DataPtr = Instr.Length = Instr.LoopStart = Instr.LoopEnd = 0;
    }
    else if (lseek(Handle,(dword)Instr.DataPtr<<4,SEEK_SET) < 0) {
        dError = ERR_FILEIO;
        return NULL;
    }
    if (!(SampPtr = dLoadSampleFile(Handle,Instr.Length,SF_8BITS|SF_UNSIGNED)))
        return NULL;
    strcpy(SampPtr->SampleName,Instr.SampleName);
    strcpy(SampPtr->FileName,Instr.FileName);
    SampPtr->Volume = Instr.Volume;
    SampPtr->Rate = Instr.Rate;
    if (Instr.Flags) {
        SampPtr->Flags |= SF_LOOPED;
        SampPtr->LoopStart = Instr.LoopStart;
        SampPtr->LoopEnd = Instr.LoopEnd;
    }
    return SampPtr;
}

/****************************************************************************
*
* Function:     S3MLoadModule
* Parameters:   Handle      - DOS file handle
*               Length      - file length
*
* Returns:      Music module address or NULL when an error has occurred
*               while loading the file.
*
* Description:  Load Scream Tracker 3.0 (S3M) music modules from disk.
*
****************************************************************************/

static DSM *S3MLoadModule(int Handle, long Length)
{
    DSM *Module;
    S3MSong Header;
    word SampTab[MAXSAMPLES],PattTab[MAXORDERS];
    byte ChanMap[32],PanMap[32],PanPos;
    int Index;

    if (!(Module = (DSM*)calloc(1,sizeof(DSM)))) {
        dError = ERR_NOMEM;
        return NULL;
    }
    if (read(Handle,&Header,sizeof(Header)) != sizeof(Header)) {
        dError = ERR_FILEIO;
        dFreeModule(Module);
        return NULL;
    }
    if (Header.Magic != S3M_SCRM || Header.NumOrders > MAXORDERS ||
        Header.NumPatterns > MAXORDERS || Header.NumSamples > MAXSAMPLES) {
        dError = ERR_FORMAT;
        dFreeModule(Module);
        return NULL;
    }
    if (read(Handle,Module->Header.Orders,Header.NumOrders) != Header.NumOrders) {
        dError = ERR_FILEIO;
        dFreeModule(Module);
        return NULL;
    }
    if (read(Handle,SampTab,2*Header.NumSamples) != 2*Header.NumSamples) {
        dError = ERR_FILEIO;
        dFreeModule(Module);
        return NULL;
    }
    if (read(Handle,PattTab,2*Header.NumPatterns) != 2*Header.NumPatterns) {
        dError = ERR_FILEIO;
        dFreeModule(Module);
        return NULL;
    }
    if (Header.DefPan == 0xFC) {
        if (read(Handle,PanMap,sizeof(PanMap)) != sizeof(PanMap)) {
            dError = ERR_FILEIO;
            dFreeModule(Module);
            return NULL;
        }
    }

    /* fill the SONG structure */
    strcpy(Module->Header.ModuleName,Header.SongName);
    Module->Header.OrderPos = 0;
    Module->Header.ReStart = MAXORDERS-1;
    Module->Header.NumOrders = Header.NumOrders;
    Module->Header.NumSamples = Header.NumSamples;
    Module->Header.NumPatterns = Header.NumPatterns;
    Module->Header.GlobalVolume = 64;
    Module->Header.MasterVolume = 2*(Header.MasterVolume & 0x7F);
    Module->Header.InitTempo = Header.Tempo;
    Module->Header.InitBPM = Header.BPM;
    Module->Header.NumTracks = 0;
    for (Index = 0; Index < 32; Index++) {
        Header.ChanMap[Index] &= 0x7F;
        ChanMap[Index] = 0xFF;
        if (Module->Header.NumTracks >= MAXTRACKS) continue;
        else if (Header.ChanMap[Index] <= 15) {
            PanPos = (Header.ChanMap[Index] <= 7) ? PAN_LEFT : PAN_RIGHT;
            if ((Header.DefPan == 0xFC) && (PanMap[Index] & 0x20))
                PanPos = (PanMap[Index] & 0x0F) << 3;
            Module->Header.ChanMap[Module->Header.NumTracks] = PanPos;
            ChanMap[Index] = (Module->Header.NumTracks)++;
        }
    }

    /* allocate sample and pattern pointer tables */
    if (!(Module->Samples = (Sample**)calloc(Module->Header.NumSamples,sizeof(Sample *)))) {
        dError = ERR_NOMEM;
        dFreeModule(Module);
        return NULL;
    }
    if (!(Module->Patterns = (Pattern**)calloc(Module->Header.NumPatterns,sizeof(Pattern *)))) {
        dError = ERR_NOMEM;
        dFreeModule(Module);
        return NULL;
    }

    /* fill the PATT structures */
    for (Index = 0; Index < Header.NumPatterns; Index++) {
        if (!(Module->Patterns[Index] = S3MLoadPattern(Handle,
                (dword)PattTab[Index]<<4,ChanMap))) {
            dFreeModule(Module);
            return NULL;
        }
    }

    /* fill the INST structures */
    for (Index = 0; Index < Header.NumSamples; Index++) {
        if (!(Module->Samples[Index] = S3MLoadSample(Handle,
                (dword)SampTab[Index]<<4))) {
            dFreeModule(Module);
            return NULL;
        }
    }
    return Module;
}

/*----------------- Multitracker 1.0 Import Routines ----------------------*/

/****************************************************************************
*
* Function:     MTMLoadPattern
* Parameters:   Seq     - track sequence
*               Tracks  - track structures
*
* Returns:      Pattern structure or NULL if error.
*
* Description:  Used to build patterns from MTM track sequences.
*
****************************************************************************/

static Pattern *MTMLoadPattern(word *Seq, MTMTrack *Tracks)
{
    Pattern *DPatt;
    byte *S,*D;
    byte Row,Track,Flags,Note,Inst,Effect,Param;

    if (!(DPatt = (Pattern*)malloc(2+(1+5*MAXTRACKS)*64))) {
        dError = ERR_NOMEM;
        return NULL;
    }
    D = DPatt->Data;
    for (Row = 0; Row < 64; Row++) {
        for (Track = 0; Track < MAXTRACKS; Track++) {
            if (Seq[Track]) {
                S = &Tracks[Seq[Track]-1].Data[3*Row];
                Note = (S[0] >> 2) & 0x3F;
                Inst = ((S[0] & 0x03)<<4) + ((S[1]>>4) & 0x0F);
                Effect = (S[1] & 0x0F);
                Param = S[2];
                Flags = 0;
                if (Note) {
                    Flags |= 0x80;
                    Note += 12*2;
                }
                if (Inst) Flags |= 0x40;
                if (Effect | Param) Flags |= 0x10;
                if (Flags) {
                    *D++ = Flags | (Track & 0x0F);
                    if (Note) *D++ = Note;
                    if (Inst) *D++ = Inst;
                    if (Effect | Param) {
                        *D++ = Effect;
                        *D++ = Param;
                    }
                }
            }
        }
        *D++ = 0;
    }
    DPatt->Length = D-(byte*)DPatt;
    return (Pattern*)realloc(DPatt,DPatt->Length);
}

/****************************************************************************
*
* Function:     MTMLoadSample
* Parameters:   Handle  - file handle
*               Instr   - MTM sample structure
*
* Returns:      Sample structure or NULL if error.
*
* Description:  Used to load MTM samples.
*
****************************************************************************/

static Sample *MTMLoadSample(int Handle, MTMSample *Instr)
{
    Sample *SampPtr;

    if (!(SampPtr = dLoadSampleFile(Handle,Instr->Length,SF_8BITS|SF_UNSIGNED)))
        return NULL;
    strncpy(SampPtr->SampleName,Instr->SampleName,sizeof(Instr->SampleName));
    SampPtr->Volume = Instr->Volume;
    SampPtr->Rate = RateTable[Instr->Finetune & 0x0F];
    if (Instr->LoopEnd - Instr->LoopStart > 2) {
        SampPtr->Flags |= SF_LOOPED;
        SampPtr->LoopStart = Instr->LoopStart;
        SampPtr->LoopEnd = Instr->LoopEnd;
    }
    return SampPtr;
}

/****************************************************************************
*
* Function:     MTMLoadModule
* Parameters:   Handle      - DOS file handle
*               Length      - file length
*
* Returns:      Music module address or NULL when an error has occurred
*               while loading the file.
*
* Description:  Load Multitracker 1.0 (MTM) music modules from disk.
*
****************************************************************************/

static DSM *MTMLoadModule(int Handle, long Length)
{
    DSM *Module;
    MTMSong Header;
    MTMSample *Samples;
    MTMTrack *Tracks;
    word TrackSeq[32];
    int Index;

    /* open MTM module file */
    if (!(Module = (DSM*)calloc(1,sizeof(DSM)))) {
        dError = ERR_NOMEM;
        return NULL;
    }

    /* load MTM header */
    if (read(Handle,&Header,sizeof(Header)) != sizeof(Header)) {
        dError = ERR_FILEIO;
        dFreeModule(Module);
        return NULL;
    }
    if (Header.Magic != MTM_MAGIC || Header.NumChans > MAXTRACKS ||
        Header.NumSamples > MAXSAMPLES || Header.BeatsPerTrack != 64) {
        dError = ERR_FORMAT;
        dFreeModule(Module);
        return NULL;
    }

    /* fill the SONG structure */
    strncpy(Module->Header.ModuleName,Header.SongName,sizeof(Header.SongName));
    Module->Header.OrderPos = 0;
    Module->Header.ReStart = MAXORDERS-1;
    Module->Header.NumOrders = Header.LastOrder+1;
    Module->Header.NumSamples = Header.NumSamples;
    Module->Header.NumPatterns = Header.LastPattern+1;
    Module->Header.NumTracks = Header.NumChans;
    Module->Header.GlobalVolume = 64;
    Module->Header.MasterVolume = 768/Module->Header.NumTracks;
    Module->Header.InitTempo = 6;
    Module->Header.InitBPM = 125;
    for (Index = 0; Index < MAXTRACKS; Index++)
        Module->Header.ChanMap[Index] = Header.PanPos[Index] << 3;

    /* allocate sample and pattern pointer tables */
    if (!(Module->Samples = (Sample**)calloc(Module->Header.NumSamples,sizeof(Sample *)))) {
        dError = ERR_NOMEM;
        dFreeModule(Module);
        return NULL;
    }
    if (!(Module->Patterns = (Pattern**)calloc(Module->Header.NumPatterns,sizeof(Pattern *)))) {
        dError = ERR_NOMEM;
        dFreeModule(Module);
        return NULL;
    }

    /* allocate MTM sample and track structures */
    if (!(Samples = (MTMSample*)calloc(Module->Header.NumSamples,sizeof(MTMSample)))) {
        dError = ERR_NOMEM;
        dFreeModule(Module);
        return NULL;
    }
    if (!(Tracks = (MTMTrack*)calloc(Header.NumTracks,sizeof(MTMTrack)))) {
        dError = ERR_NOMEM;
        free(Samples);
        dFreeModule(Module);
        return NULL;
    }

    /* load MTM sample structures */
    if (read(Handle,Samples,Module->Header.NumSamples*sizeof(MTMSample)) != Module->Header.NumSamples*sizeof(MTMSample)) {
        dError = ERR_FILEIO;
        free(Tracks);
        free(Samples);
        dFreeModule(Module);
        return NULL;
    }

    /* load the SONG order list */
    if (read(Handle,Module->Header.Orders,128) != 128) {
        dError = ERR_FILEIO;
        free(Tracks);
        free(Samples);
        dFreeModule(Module);
        return NULL;
    }

    /* load MTM tracks */
    if (read(Handle,Tracks,Header.NumTracks*sizeof(MTMTrack)) != Header.NumTracks*sizeof(MTMTrack)) {
        dError = ERR_FILEIO;
        free(Tracks);
        free(Samples);
        dFreeModule(Module);
        return NULL;
    }

    /* fill PATT structures */
    for (Index = 0; Index < Module->Header.NumPatterns; Index++) {
        if (read(Handle,TrackSeq,sizeof(TrackSeq)) != sizeof(TrackSeq)) {
            dError = ERR_FILEIO;
            free(Tracks);
            free(Samples);
            dFreeModule(Module);
            return NULL;
        }
        if (!(Module->Patterns[Index] = MTMLoadPattern(TrackSeq,Tracks))) {
            free(Tracks);
            free(Samples);
            dFreeModule(Module);
            return NULL;
        }
    }
    free(Tracks);

    /* skip MTM comments data */
    if (lseek(Handle,Header.CommentLength,SEEK_CUR) < 0) {
        dError = ERR_FILEIO;
        free(Samples);
        dFreeModule(Module);
        return NULL;
    }

    /* fill INST structures */
    for (Index = 0; Index < Module->Header.NumSamples; Index++) {
        if (!(Module->Samples[Index] = MTMLoadSample(Handle,&Samples[Index]))) {
            free(Samples);
            dFreeModule(Module);
            return NULL;
        }
    }
    free(Samples);

    return Module;
}

/*------------------- Composer 669 Import Routines ------------------------*/

/****************************************************************************
*
* Function:     GG9LoadPattern
* Parameters:   Handle      - file handle
*               Tempo       - initial tempo value
*               BreakRow    - number of rows
*
* Returns:      Pattern structure or NULL if error.
*
* Description:  Used to load 669 patterns.
*
****************************************************************************/

static Pattern *GG9LoadPattern(int Handle, int Tempo, int BreakRow)
{
    static byte VolTable[16] = {
        0x00,0x06,0x0B,0x10,0x14,0x18,0x1C,0x20,
        0x24,0x28,0x2C,0x30,0x34,0x38,0x3C,0x40 };
    Pattern *DPatt;
    byte *SPatt,*S,*D;
    byte Row,Track,Flags,Note,Inst,Volume,Effect,Param;
    int InsertTempo, InsertBreak;

    if (!(DPatt = (Pattern*)malloc(2+(1+6*8)*64))) {
        dError = ERR_NOMEM;
        return NULL;
    }
    if (!(SPatt = (byte*)malloc(3*8*64))) {
        dError = ERR_NOMEM;
        free(DPatt);
        return NULL;
    }
    if (read(Handle,SPatt,3*8*64) != 3*8*64) {
        dError = ERR_FILEIO;
        free(SPatt);
        free(DPatt);
        return NULL;
    }
    S = SPatt;
    D = DPatt->Data;
    for (Row = 0; Row < 64; Row++) {
        InsertTempo = (Row == 0);
        InsertBreak = ((Row == BreakRow) && (Row != 63));
        for (Track = 0; Track < 8; Track++) {
            Note = (S[0] >> 2);
            Inst = ((S[0] & 0x03) << 4) | (S[1] >> 4);
            Volume = VolTable[S[1] & 0x0F];
            Effect = (S[2] >> 4);
            Param = (S[2] & 0x0F);
            if (S[0] == 0xFE) {
                Note = Inst = 0;
            }
            else if (S[0] == 0xFF) {
                Note = Inst = 0;
                Volume = 255;
            }
            else {
                Note++;
                Inst++;
            }
            S += 3;
            switch (Effect) {
                case 0x0:
                    Effect = 0x01;
                    break;
                case 0x1:
                    Effect = 0x02;
                    break;
                case 0x2:
                    Effect = 0x03;
                    break;
                case 0x3:
                    Effect = 0x0E;
                    Param = 0x21;
                    break;
                case 0x4:
                    Effect = 0x04;
                    Param = (Param << 4) + 1;
                    break;
                case 0x5:
                    Effect = 0x0F;
                    break;
                default:
                    Effect = Param = 0;
                    break;
            }
            if (InsertTempo && (!(Effect | Param) || (Track == 7))) {
                Effect = 0x0F;
                Param = Tempo;
                InsertTempo = 0;
            }
            if (InsertBreak && (!(Effect | Param) || (Track == 7))) {
                Effect = 0x0D;
                Param = 0x00;
                InsertBreak = 0;
            }
            Flags = 0;
            if (Note) {
                Note += 12*2;
                Flags |= 0x80;
            }
            if (Inst) Flags |= 0x40;
            if (Volume <= 64) Flags |= 0x20;
            if (Effect | Param) Flags |= 0x10;
            if (Flags) {
                *D++ = Flags | (Track & 0x0F);
                if (Note) *D++ = Note;
                if (Inst) *D++ = Inst;
                if (Volume <= 64) *D++ = Volume;
                if (Effect | Param) {
                    *D++ = Effect;
                    *D++ = Param;
                }
            }
        }
        *D++ = 0x00;
    }
    free(SPatt);
    DPatt->Length = D-(byte*)DPatt;
    return (Pattern*)realloc(DPatt,DPatt->Length);
}

/****************************************************************************
*
* Function:     GG9LoadSample
* Parameters:   Handle  - file handle
*               Instr   - 669 sample structure
*
* Returns:      Sample structure or NULL if error.
*
* Description:  Used to load 669 samples.
*
****************************************************************************/

static Sample *GG9LoadSample(int Handle, GG9Sample *Instr)
{
    Sample *SampPtr;

    if (!(SampPtr = dLoadSampleFile(Handle,Instr->Length,SF_8BITS|SF_UNSIGNED)))
        return NULL;
    strncpy(SampPtr->SampleName,Instr->FileName,sizeof(Instr->FileName));
    SampPtr->Volume = 64;
    SampPtr->Rate = MIDCFREQ;
    if (Instr->LoopEnd <= Instr->Length) {
        SampPtr->Flags |= SF_LOOPED;
        SampPtr->LoopStart = Instr->LoopStart;
        SampPtr->LoopEnd = Instr->LoopEnd;
    }
    return SampPtr;
}

/****************************************************************************
*
* Function:     GG9LoadModule
* Parameters:   Handle      - DOS file handle
*               Length      - file length
*
* Returns:      Music module address or NULL when an error has occurred
*               while loading the file.
*
* Description:  Load Composer 669 music modules from disk.
*
****************************************************************************/

static DSM *GG9LoadModule(int Handle, long Length)
{
    DSM *Module;
    GG9Song Header;
    GG9Sample Samples[64];
    int Index;

    if (!(Module = (DSM*)calloc(1,sizeof(DSM)))) {
        dError = ERR_NOMEM;
        return NULL;
    }
    if (read(Handle,&Header,sizeof(Header)) != sizeof(Header)) {
        dError = ERR_FILEIO;
        dFreeModule(Module);
        return NULL;
    }
    if (Header.Magic != GG9_MAGIC || Header.NumSamples > 64) {
        dError = ERR_FORMAT;
        dFreeModule(Module);
        return NULL;
    }
    if (read(Handle,Samples,sizeof(GG9Sample)*Header.NumSamples) != sizeof(GG9Sample)*Header.NumSamples) {
        dError = ERR_FILEIO;
        dFreeModule(Module);
        return NULL;
    }

    /* fill SONG structure */
    strncpy(Module->Header.ModuleName,Header.SongName,sizeof(Module->Header.ModuleName)-1);
    Module->Header.OrderPos = 0;
    Module->Header.ReStart = Header.ReStart;
    Module->Header.NumSamples = Header.NumSamples;
    Module->Header.NumPatterns = Header.NumPatterns;
    Module->Header.NumTracks = 8;
    Module->Header.GlobalVolume = 64;
    Module->Header.MasterVolume = 768/8;
    Module->Header.InitTempo = 4;
    Module->Header.InitBPM = 80;
    for (Index = 0; Index < 8; Index++) {
        Module->Header.ChanMap[Index] = (Index & 1) ? PAN_RIGHT : PAN_LEFT;
    }
    for (Index = 0; Index < 128; Index++) {
        if (Header.Orders[Index] >= 128) break;
        Module->Header.Orders[Index] = Header.Orders[Index];
        (Module->Header.NumOrders)++;
    }

    /* allocate sample and pattern pointer tables */
    if (!(Module->Samples = (Sample**)calloc(Module->Header.NumSamples,sizeof(Sample *)))) {
        dError = ERR_NOMEM;
        dFreeModule(Module);
        return NULL;
    }
    if (!(Module->Patterns = (Pattern**)calloc(Module->Header.NumPatterns,sizeof(Pattern *)))) {
        dError = ERR_NOMEM;
        dFreeModule(Module);
        return NULL;
    }

    /* load PATT structures */
    for (Index = 0; Index < Module->Header.NumPatterns; Index++) {
        if (!(Module->Patterns[Index] = GG9LoadPattern(Handle,
                Header.Tempos[Index],Header.Breaks[Index]))) {
            dFreeModule(Module);
            return NULL;
        }
    }

    /* load INST structures */
    for (Index = 0; Index < Module->Header.NumSamples; Index++) {
        if (!(Module->Samples[Index] = GG9LoadSample(Handle,&Samples[Index]))) {
            dFreeModule(Module);
            return NULL;
        }
    }
    return Module;
}

/*-------------------- Scream Tracker 2.0 import routines -----------------*/

/****************************************************************************
*
* Function:     STMLoadPattern
* Parameters:   Handle  - file handle
*
* Returns:      Pattern structure or NULL if error.
*
* Description:  Used to load STM patterns.
*
****************************************************************************/

static Pattern *STMLoadPattern(int Handle)
{
    Pattern *DPatt;
    byte *SPatt,*S,*D;
    byte Row,Track,Flags,Note,Inst,Volume,Effect,Param;

    if (!(DPatt = (Pattern*)malloc(2+(1+6*4)*64))) {
        dError = ERR_NOMEM;
        return NULL;
    }
    if (!(SPatt = (byte*)malloc(4*4*64))) {
        dError = ERR_NOMEM;
        free(DPatt);
        return NULL;
    }
    if (read(Handle,SPatt,4*4*64) != 4*4*64) {
        dError = ERR_FILEIO;
        free(SPatt);
        free(DPatt);
        return NULL;
    }
    S = SPatt;
    D = DPatt->Data;
    for (Row = 0; Row < 64; Row++) {
        for (Track = 0; Track < 4; Track++) {
            Note = S[0];
            Inst = S[1] >> 3;
            Volume = (S[1] & 0x07) | ((S[2] & 0xF0) >> 1);
            Effect = 64 + (S[2] & 0x0F);
            Param = S[3];
            S += 4;
            if (Note >= 251) continue;
            Note = (Note & 0x0F) + 12*(Note >> 4);
            switch (Effect) {
                case 'A':
                    Effect = 0x0F;
                    Param >>= 4;
                    break;
                case 'B':
                    Effect = 0x0B;
                    break;
                case 'C':
                    Effect = 0x0D;
                    break;
                case 'D':
                    if ((Param & 0xF0) == 0x00) {
                        Effect = 0x0A;
                        Param &= 0x0F;
                    }
                    else if ((Param & 0x0F) == 0x00) {
                        Effect = 0x0A;
                        Param &= 0xF0;
                    }
                    else {
                        Effect = Param = 0;
                    }
                    break;
                case 'E':
                    Effect = 0x02;
                    break;
                case 'F':
                    Effect = 0x01;
                    break;
                case 'G':
                    Effect = 0x03;
                    break;
                case 'H':
                    Effect = 0x04;
                    break;
                case 'I':
                    Effect = 0x0E;      /* emu tremor with retrig */
                    Param = 0x90 | (Param & 0x0F);
                    break;
                case 'J':
                    Effect = 0x00;
                    break;
                default:
                    Effect = Param = 0;
                    break;
            }
            Flags = 0;
            if (Note) {
                Note += 2*12;
                Flags |= 0x80;
            }
            if (Inst) Flags |= 0x40;
            if (Volume <= 64) Flags |= 0x20;
            if (Effect | Param) Flags |= 0x10;
            if (Flags) {
                *D++ = Flags | (Track & 0x0F);
                if (Note) *D++ = Note;
                if (Inst) *D++ = Inst;
                if (Volume <= 64) *D++ = Volume;
                if (Effect | Param) {
                    *D++ = Effect;
                    *D++ = Param;
                }
            }
        }
        *D++ = 0x00;
    }
    free(SPatt);
    DPatt->Length = D-(byte*)DPatt;
    return (Pattern*)realloc(DPatt,DPatt->Length);
}

/****************************************************************************
*
* Function:     STMLoadSample
* Parameters:   Handle  - file handle
*               Instr   - STM sample structure
*
* Returns:      Sample structure or NULL if error.
*
* Description:  Used to load STM samples.
*
****************************************************************************/

static Sample *STMLoadSample(int Handle, STMSample *Instr)
{
    Sample *SampPtr;

    if (!(SampPtr = dLoadSampleFile(Handle,Instr->Length,SF_8BITS|SF_SIGNED)))
        return NULL;
    strncpy(SampPtr->SampleName,Instr->FileName,sizeof(Instr->FileName));
    SampPtr->Volume = Instr->Volume;
    SampPtr->Rate = Instr->Rate;
    if (Instr->LoopEnd <= Instr->Length) {
        SampPtr->Flags |= SF_LOOPED;
        SampPtr->LoopStart = Instr->LoopStart;
        SampPtr->LoopEnd = Instr->LoopEnd;
    }
    return SampPtr;
}

/****************************************************************************
*
* Function:     STMLoadModule
* Parameters:   Handle      - DOS file handle
*               Length      - file length
*
* Returns:      Music module address or NULL when an error has occurred
*               while loading the file.
*
* Description:  Load Scream Tracker 2.0 (STM) music modules from disk.
*
****************************************************************************/

static DSM *STMLoadModule(int Handle, long Length)
{
    DSM *Module;
    STMSong Header;
    int Index;

    if (!(Module = (DSM*)calloc(1,sizeof(DSM)))) {
        dError = ERR_NOMEM;
        return NULL;
    }
    if (read(Handle,&Header,sizeof(Header)) != sizeof(Header)) {
        dError = ERR_FILEIO;
        dFreeModule(Module);
        return NULL;
    }
    if (memcmp(Header.Tracker,STM_TRACKER,sizeof(Header.Tracker))) {
        dError = ERR_FORMAT;
        dFreeModule(Module);
        return NULL;
    }

    /* fill SONG structure */
    strncpy(Module->Header.ModuleName,Header.SongName,sizeof(Header.SongName));
    Module->Header.OrderPos = 0;
    Module->Header.ReStart = MAXORDERS-1;
    Module->Header.NumSamples = 31;
    Module->Header.NumPatterns = Header.NumPatterns;
    Module->Header.NumTracks = 4;
    Module->Header.GlobalVolume = 64;
    Module->Header.MasterVolume = 768/4;
    Module->Header.InitTempo = Header.InitTempo >> 4;
    Module->Header.InitBPM = 125;
    for (Index = 0; Index < 128; Index++) {
        if (Header.Orders[Index] >= 99) break;
        Module->Header.Orders[Index] = Header.Orders[Index];
        (Module->Header.NumOrders)++;
    }
    for (Index = 0; Index < 4; Index++) {
        Module->Header.ChanMap[Index] = (Index & 1) ? PAN_RIGHT : PAN_LEFT;
    }

    /* allocate sample and pattern pointer tables */
    if (!(Module->Samples = (Sample**)calloc(Module->Header.NumSamples,sizeof(Sample *)))) {
        dError = ERR_NOMEM;
        dFreeModule(Module);
        return NULL;
    }
    if (!(Module->Patterns = (Pattern**)calloc(Module->Header.NumPatterns,sizeof(Pattern *)))) {
        dError = ERR_NOMEM;
        dFreeModule(Module);
        return NULL;
    }

    /* load PATT structures */
    for (Index = 0; Index < Module->Header.NumPatterns; Index++) {
        if (!(Module->Patterns[Index] = STMLoadPattern(Handle))) {
            dFreeModule(Module);
            return NULL;
        }
    }

    /* load INST structures */
    for (Index = 0; Index < Module->Header.NumSamples; Index++) {
        if (!(Module->Samples[Index] = STMLoadSample(Handle,&Header.Samples[Index]))) {
            dFreeModule(Module);
            return NULL;
        }
    }
    return Module;
}


/*-------------------- RIFF/WAVE Import Routines --------------------------*/

/****************************************************************************
*
* Function:     WAVLoadSample
* Parameters:   Handle      - DOS file handle
*               Length      - file length
*
* Parameters:   Filename    - WAVE filename
*               SeekPos     - starting position
*
* Returns:      Sample address or NULL when an error has occurred
*               while loading the file.
*
* Description:  Load RIFF/WAVE 8-bit mono PCM samples from disk.
*
****************************************************************************/

static Sample *WAVLoadSample(int Handle, long Length)
{
    Sample     *SampPtr;
    RiffHeader Header;
    RiffBlock  Block;
    WaveFmt    Fmt;

    /* read RIFF header block */
    if (read(Handle,&Header,sizeof(Header)) != sizeof(Header)) {
        dError = ERR_FILEIO;
        return NULL;
    }
    if (Header.ID != ID_RIFF || Header.Type != ID_WAVE) {
        dError = ERR_FORMAT;
        return NULL;
    }

    /* read RIFF/WAVE fmt block */
    if (read(Handle,&Block,sizeof(Block)) != sizeof(Block)) {
        dError = ERR_FILEIO;
        return NULL;
    }
    if (Block.ID != ID_FMT || Block.Length != sizeof(Fmt)) {
        dError = ERR_FORMAT;
        return NULL;
    }
    if (read(Handle,&Fmt,sizeof(Fmt)) != sizeof(Fmt)) {
        dError = ERR_FILEIO;
        return NULL;
    }
    if (Fmt.Format != WAVE_FMT_PCM || Fmt.Channels != 1 ||
            Fmt.BitsPerSample != 8) {
        dError = ERR_FORMAT;
        return NULL;
    }

    /* read RIFF/WAVE data block */
    if (read(Handle,&Block,sizeof(Block)) != sizeof(Block)) {
        dError = ERR_FILEIO;
        return NULL;
    }
    if (Block.ID != ID_DATA) {
        dError = ERR_FORMAT;
        return NULL;
    }

    /* load RIFF/WAVE PCM samples */
    if (!(SampPtr = dLoadSampleFile(Handle,Block.Length,SF_8BITS|SF_UNSIGNED)))
        return NULL;
    SampPtr->Volume = 64;
    SampPtr->Rate = Fmt.SampleRate;

    return SampPtr;
}


/*---------------- Creative Labs' Voice Import Routines -------------------*/

/****************************************************************************
*
* Function:     VOCLoadSample
* Parameters:   Handle      - DOS file handle
*               Length      - file length
*
* Returns:      Sample address or NULL when an error has occurred
*               while loading the file.
*
* Description:  Load Creative Labs' Voice 8-bit mono samples from disk.
*
****************************************************************************/

static Sample *VOCLoadSample(int Handle, long Length)
{
    Sample *SampPtr;
    VocHeader VocHdr;
    VocBlock VocBlk;
    VocData VocDta;
    VocExtInfo VocExt;
    long Size;

    /* read VOC header */
    if (read(Handle,&VocHdr,sizeof(VocHdr)) != sizeof(VocHdr)) {
        dError = ERR_FILEIO;
        return NULL;
    }
    if (memcmp(VocHdr.Magic,VOC_HDR,sizeof(VocHdr.Magic))) {
        dError = ERR_FORMAT;
        return NULL;
    }

    /* seek VOC data block */
    if (lseek(Handle,VocHdr.BlockPos-sizeof(VocHdr),SEEK_CUR) < 0) {
        dError = ERR_FILEIO;
        return NULL;
    }
    do {
        if (read(Handle,&VocBlk,sizeof(VocBlk)) != sizeof(VocBlk)) {
            dError = ERR_FILEIO;
            return NULL;
        }
        Size = *((long*)VocBlk.Size) & 0xFFFFFF;
        if (VocBlk.Type == VOC_DATA) {
            if (read(Handle,&VocDta,sizeof(VocDta)) != sizeof(VocDta)) {
                dError = ERR_FILEIO;
                return NULL;
            }
            if (VocDta.Format != VOC_FMT_8BITS) {
                dError = ERR_FORMAT;
                return NULL;
            }
            Size -= sizeof(VocDta);
        }
        else if (VocBlk.Type == VOC_EXTINFO) {
            if (read(Handle,&VocExt,sizeof(VocExt)) != sizeof(VocExt)) {
                dError = ERR_FILEIO;
                return NULL;
            }
            if (VocExt.Mode != VOC_MODE_MONO) {
                dError = ERR_FORMAT;
                return NULL;
            }
        }
        else {
            if (lseek(Handle,Size,SEEK_CUR) < 0) {
                dError = ERR_FILEIO;
                return NULL;
            }
        }
    } while (VocBlk.Type != VOC_DATA);

    /* load VOC PCM samples */
    if (!(SampPtr = dLoadSampleFile(Handle,Size,SF_8BITS|SF_UNSIGNED)))
        return NULL;
    SampPtr->Volume = 64;
    SampPtr->Rate = (word)(1000000L/(256-VocDta.TimeConst));

    return SampPtr;
}


/*------------- Amiga 8-bit RAW sample files Import Routines --------------*/

/****************************************************************************
*
* Function:     RAWLoadSample
* Parameters:   Handle      - DOS file handle
*               Length      - file length
*
* Returns:      Sample address or NULL when an error has occurred
*               while loading the file.
*
* Description:  Load 8-bit mono signed RAW sample files.
*
****************************************************************************/

static Sample *RAWLoadSample(int Handle, long Length)
{
    return dLoadSampleFile(Handle,Length,SF_8BITS|SF_SIGNED);
}


/*------------- Amiga IFF/8SVX sample format Import Routines --------------*/

/****************************************************************************
*
* Function:     IFFLoadSample
* Parameters:   Handle      - DOS file handle
*               Length      - file length
*
* Returns:      Sample address or NULL when an error has occurred
*               while loading the file.
*
* Description:  Load 8-bit mono signed IFF sample files.
*
****************************************************************************/

static Sample *IFFLoadSample(int Handle, long Length)
{
    Sample *SampPtr;
    IffHeader Header;
    IffBlock Block;
    IffVHdr VHdr;
    IffChan Chan;

    /* read IFF/8SVX header */
    if (read(Handle,&Header,sizeof(Header)) != sizeof(Header)) {
        dError = ERR_FILEIO;
        return NULL;
    }
    if (Header.ID != IFF_FORM || Header.Type != IFF_8SVX) {
        dError = ERR_FORMAT;
        return NULL;
    }

    /* seek IFF/8SVX BODY chunk */
    SampPtr = NULL;
    VHdr.Rate = MIDCFREQ;
    Header.Length = LSWAP(Header.Length);
    if (Header.Length & 1) Header.Length++;
    Header.Length -= sizeof(Header.Type);
    while (Header.Length) {
        if (read(Handle,&Block,sizeof(Block)) != sizeof(Block)) {
            dError = ERR_FILEIO;
            return NULL;
        }
        Block.Length = LSWAP(Block.Length);
        if (Block.Length & 1) Block.Length++;
        Header.Length -= sizeof(Block) + Block.Length;
        if (Block.ID == IFF_VHDR) {
            if (Block.Length != sizeof(VHdr)) {
                dError = ERR_FORMAT;
                return NULL;
            }
            if (read(Handle,&VHdr,sizeof(VHdr)) != sizeof(VHdr)) {
                dError = ERR_FILEIO;
                return NULL;
            }
            VHdr.Rate = WSWAP(VHdr.Rate);
            if (VHdr.Format != 0) {
                dError = ERR_FORMAT;
                return NULL;
            }
        }
        else if (Block.ID == IFF_CHAN) {
            if (Block.Length != sizeof(Chan)) {
                dError = ERR_FORMAT;
                return NULL;
            }
            if (read(Handle,&Chan,sizeof(Chan)) != sizeof(Chan)) {
                dError = ERR_FILEIO;
                return NULL;
            }
            Chan.Channels = LSWAP(Chan.Channels);
            Chan.Channels = (Chan.Channels & 0x01) +
                ((Chan.Channels & 0x02) >> 1) +
                ((Chan.Channels & 0x04) >> 2) +
                ((Chan.Channels & 0x08) >> 3);
            if (Chan.Channels != 1) {
                dError = ERR_FORMAT;
                return NULL;
            }
        }
        else if (Block.ID == IFF_BODY) {
            if (!(SampPtr = dLoadSampleFile(Handle,Block.Length,SF_8BITS|SF_SIGNED)))
                return NULL;
            SampPtr->Volume = 64;
            SampPtr->Rate = VHdr.Rate;
            break;
        }
        else if (lseek(Handle,Block.Length,SEEK_CUR) < 0) {
            dError = ERR_FILEIO;
            return NULL;
        }
    }

    if (!SampPtr) dError = ERR_FORMAT;
    return SampPtr;
}


/*------------------- Module/Sample Import Routines -----------------------*/

/****************************************************************************
*
* Function:     dImportModuleFile
* Parameters:   Handle  - DOS file handle
*               Length  - Module length
*               Form    - Module format
*
* Returns:      Music module address or NULL when an error has occurred
*               while loading the file.
*
* Description:  Load multichannel music modules from disk.
*
****************************************************************************/

DSM *dImportModuleFile(int Handle, long Length, int Form)
{
    return (Form == FORM_DSM) ? dLoadModuleFile(Handle,Length) :
        (Form == FORM_MOD) ? MODLoadModule(Handle,Length) :
        (Form == FORM_S3M) ? S3MLoadModule(Handle,Length) :
        (Form == FORM_MTM) ? MTMLoadModule(Handle,Length) :
        (Form == FORM_669) ? GG9LoadModule(Handle,Length) :
        (Form == FORM_STM) ? STMLoadModule(Handle,Length) :
        NULL;
}


/****************************************************************************
*
* Function:     dImportSampleFile
* Parameters:   Filename    - Sample filename
*               Length      - Sample length
*               Form        - Sample format
*
* Returns:      Sample address or NULL when an error has occurred
*               while loading the file.
*
* Description:  Load 8-bit mono digital samples from disk.
*
****************************************************************************/

Sample *dImportSampleFile(int Handle, long Length, int Form)
{
    return (Form == FORM_WAV) ? WAVLoadSample(Handle,Length) :
        (Form == FORM_VOC) ? VOCLoadSample(Handle,Length) :
        (Form == FORM_IFF) ? IFFLoadSample(Handle,Length) :
        (Form == FORM_RAW) ? RAWLoadSample(Handle,Length) :
        NULL;
}


/****************************************************************************
*
* Function:     dImportModule
* Parameters:   Filename    - Module filename
*               Form        - Module format
*
* Returns:      Music module address or NULL when an error has occurred
*               while loading the file.
*
* Description:  Load multichannel music modules from disk.
*
****************************************************************************/

DSM *dImportModule(char *Filename, int Form)
{
    int Handle;
    DSM *Module;
    long Length;

    if ((Handle = open(Filename,O_RDONLY|O_BINARY)) < 0) {
        dError = ERR_NOFILE;
        return NULL;
    }
    if ((Length = filelength(Handle)) < 0) {
        dError = ERR_FILEIO;
        close(Handle);
        return NULL;
    }
    Module = dImportModuleFile(Handle,Length,Form);
    close(Handle);
    return Module;
}


/****************************************************************************
*
* Function:     dImportSample
* Parameters:   Filename    - Sample filename
*               Form        - Sample format
*
* Returns:      Sample address or NULL when an error has occurred
*               while loading the file.
*
* Description:  Load 8-bit mono digital samples from disk.
*
****************************************************************************/

Sample *dImportSample(char *Filename, int Form)
{
    int Handle;
    Sample *SampPtr;
    long Length;

    if ((Handle = open(Filename,O_RDONLY|O_BINARY)) < 0) {
        dError = ERR_NOFILE;
        return NULL;
    }
    if ((Length = filelength(Handle)) < 0) {
        dError = ERR_FILEIO;
        close(Handle);
        return NULL;
    }
    SampPtr = dImportSampleFile(Handle,Length,Form);
    close(Handle);
    return SampPtr;
}

