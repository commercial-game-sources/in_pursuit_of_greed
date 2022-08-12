/****************************************************************************/
/*                                                                          */
/* Layer Editor for Greed I                                                 */
/*                                                                          */
/* Copyright(C) 1995 by Robert Morgan of Channel 7                          */
/*                                                                          */
/****************************************************************************/

#include <STDIO.H>
#include <DOS.H>
#include <STDARG.H>
#include <STRING.H>
#include <STDLIB.H>
#include <CONIO.H>


/**** CONSTANTS ****/

#define VERSION          "1.017"
#define NUMLAYERS        18
#define XMAX             64
#define YMAX             64
#define TEXTMODE         0x03
#define VGAMODE          0x13
#define PEL_WRITE_ADR    0x3c8
#define PEL_READ_ADR     0x3c7
#define PEL_DATA         0x3c9
#define SCREEN           0xA000
#define SCREENWIDTH      320
#define SCREENHEIGHT     200
#define OUTBYTE(x,y)     (outp((unsigned)(x),(unsigned)(y)))
#define OUTWORD(x,y)     (outpw((unsigned)(x),(unsigned)(y)))
#define INBYTE(x)        (inp((unsigned)(x)))
#define INWORD(x)        (inpw((unsigned)(x)))
#define CLI              _disable()
#define STI              _enable()
#define MAXFONTCHARS     94
#define FONTWIDTH        5
#define TEXTCOLOR        5
#define BUFX             190
#define BUFY             190

 /* text modes */
#define TEXT_NORMAL      0
#define TEXT_NOOVERWRITE 1
#define TEXT_CENTER      2

 /* flags */
#define F_RIGHT          (1<<0)
#define F_LEFT           (1<<1)
#define F_UP             (1<<2)
#define F_DOWN           (1<<3)
#define F_TRANSPARENT    (1<<4)
#define F_NOCLIP         (1<<5)
#define F_NOBULLETCLIP   (1<<6)
#define F_DAMAGE         (1<<7)

 /* layer names */
#define NORTHWALL        0
#define NORTHFLAGS       1
#define WESTWALL         2
#define WESTFLAGS        3
#define FLOOR            4
#define FLOORFLAGS       5
#define CEILING          6
#define CEILINGFLAGS     7
#define FLOORHEIGHT      8
#define CEILINGHEIGHT    9
#define FLOORDEF         10
#define FLOORDEFFLAGS    11
#define CEILINGDEF       12
#define CEILINGDEFFLAGS  13
#define LIGHTS           14
#define EFFECTS          15
#define SPRITES          16
#define SLOPES           17


char *layernames[NUMLAYERS]=
 {
  "NORTHWALL    ",
  "",
  "WESTWALL     ",
  "",
  "FLOOR        ",
  "",
  "CEILING      ",
  "",
  "FLOORHEIGHT  ",
  "CEILINGHEIGHT",
  "FLOORDEF     ",
  "",
  "CEILINGDEF   ",
  "",
  "LIGHTS       ",
  "EFFECTS      ",
  "SPRITES      ",
  "SLOPES       "
  };

/**** TYPES ****/

typedef unsigned char     byte;
typedef byte              layerdata_t;
typedef layerdata_t       layer_t[YMAX][XMAX];
typedef enum {false,true} boolean;
typedef short             word;
typedef unsigned long     longint;

 /* CPR file format header */
typedef struct
 {
  int signature;
  byte version;
  int  width, height;
  byte flags, headersize;
  byte indicator;
  int  orgx, orgy;
  } CPR_HEADER;

 /* FON character */
typedef struct
 {
  byte width;
  word lines[16];
  } fontchartype;

 /* FON file format header */
typedef struct
 {
  char signature[4];
  int  height;
  int  asciistart;
  int  nchars;
  fontchartype fontchar[MAXFONTCHARS];
  } fonttype;


/**** VARIABLES ****/

layer_t far *layers[NUMLAYERS]; // map layers
char         filename[13], printstr[128];
boolean      quit, messageready;
byte         colors[768];
byte far     *screen, *ylookup[SCREENHEIGHT], buffer[BUFY][BUFX],
	     *bufylookup[BUFY];
fonttype     font;
int          tcolor, bkcolor, fontmode, fontavgwidth, currentvalue[NUMLAYERS],
	     currentx, currenty, currentlayer;


/**** FUNCTIONS ****/

 /* protos */
void Error(char *error,...);
void ReadFile(char *filename,word length,void *buffer);


/* GRAPHICS *****************************************************************/

void SetPalette(byte *palette)
{
 int i;

 while (inp(0x3da) & 0x08) ;
 while (!(inp(0x3da) & 0x08)) ;
 while (!(inp(0x3da) & 0x01)) ;
 OUTBYTE(PEL_WRITE_ADR,0);
 for (i=0;i<768;i++)
  OUTBYTE(PEL_DATA,*palette++);
 }


void FillPalette(int red, int green, int blue)
{
 int i;

 while (inp(0x3da) & 0x08) ;
 while (!(inp(0x3da) & 0x08)) ;
 while (!(inp(0x3da) & 0x01)) ;
 OUTBYTE(PEL_WRITE_ADR,0);
 for (i=0;i<256;i++)
  {
   OUTBYTE(PEL_DATA,red);
   OUTBYTE(PEL_DATA,green);
   OUTBYTE(PEL_DATA,blue);
   }
 }


void LoadCPR(char *s)
{
 int        x, y, i, data, count, orgx, orgy, indicator;
 char       str1[14];
 FILE       *f;
 CPR_HEADER h;
 byte far   *t;

 FillPalette(0,0,0);
 f=fopen(s,"rb");
 if (f==NULL)
  Error("%s not found");
 if (!fread(&h,sizeof(CPR_HEADER),1,f) ||
  h.signature!=19794 || fseek(f,h.headersize,0) ||
  ((h.flags & 1) && !fread(colors,768,1,f)))
   Error("Error reading %s");
 if (h.version==4)
  {
   indicator=255;
   orgx=0;
   orgy=0;
   }
 else if (h.version==5)
  {
   indicator=h.indicator;
   orgx=h.orgx;
   orgy=h.orgy;
   memset(screen,0,SCREENHEIGHT*SCREENWIDTH);
   }
 else Error("Invalid %s CPR file version",s);
 y=orgy;
 x=orgx;
 t=ylookup[y]+x;
 while (y<h.height)
  {
   data=fgetc(f);
   if (data==indicator)
    {
     count=fgetc(f);
     data=fgetc(f);
     for (i=0;i<count;i++)
      {
       *t++=data;
       ++x;
       if (x==h.width)
	{
	 x=orgx;
	 ++y;
	 t=ylookup[y]+x;
	 }
       }
     }
   else if (data>=0)
    {
     *t++=data;
     ++x;
     if (x==h.width)
      {
       x=orgx;
       ++y;
       t=ylookup[y]+x;
       }
     }
   else Error("%s corrupted",s);
   }
 fclose(f);
 SetPalette(colors);
 }


void LoadFON(char *s,int setwidth)
{
 ReadFile(s,sizeof(fonttype),&font);
 fontavgwidth=setwidth;
 }


int FontStrLen(char *s)
{
 int i, j, result;

 i=0;
 result=0;
 j=font.asciistart;
 while (s[i])
  {
   result+=font.fontchar[s[i]-j].width;
   ++i;
   }
 return result;
 }


void PrintXY(int x1,int y1,char *s)
{
 int           i, h, j, k, width, lines;
 int           x, y, l, letter;
 fontchartype *f;
 byte         *dest;

 if (fontmode & TEXT_CENTER)
  {
   i=FontStrLen(s)>>1;
   x1-=i;
   }
 l=strlen(s);
 x=x1;
 h=font.height;
 for (k=0; k<l; k++)
  {
   y=y1;
   letter=s[k]-font.asciistart;
   f=&font.fontchar[letter];
   width=f->width;
   for (i=0;i<h;i++,y++)
    {
     lines=f->lines[i];
     dest=ylookup[y]+x;
     for (j=0;j<width;j++,dest++)
      {
       if (lines & (1<<j))
	*dest=tcolor;
       else if (bkcolor!=255)
	*dest=bkcolor;
       }
     }
   x+=width;
   }
 width=l*fontavgwidth+x1;
 if (!(fontmode & TEXT_NOOVERWRITE) && x<width && bkcolor!=255)
  {
   y=y1;
   width=width-x+1;
   dest=ylookup[y]+x;
   for (i=0;i<h;i++,y++,dest+=320)
    memset(dest,bkcolor,width);
   }
 }


void PrintfXY(int x,int y,char *sfmt,...)
{
 va_list argptr;

 va_start(argptr,sfmt);
 vsprintf(printstr,sfmt,argptr);
 va_end(argptr);
 PrintXY(x,y,printstr);
 }


/* MISC & ERROR *************************************************************/

void SetVidMode(int mode)
{
 union REGS r;

 r.x.ax=mode;
 int86(0x10,&r,&r);
 }


void Error(char *error,...)
{
 va_list   argptr;
 byte far *vidmode;

 vidmode=MK_FP(0x40,0x49);
 if (*vidmode!=TEXTMODE)
  SetVidMode(TEXTMODE);
 printf("Error: ");
 va_start(argptr,error);
 vprintf(error,argptr);
 va_end(argptr);
 while (kbhit())
  getch();
 exit(255);
 }


word ReadKey(void)
{
 union REGS r;

 r.h.ah=0x10;
 int86(0x16,&r,&r);
 return r.x.ax;
 }


void DeleteMessage(void)
{
 int i;

 for (i=193;i<200;i++)
  memset(ylookup[i]+201,0,119);
 messageready=false;
 }


void Message(char *error,...)
{
 va_list argptr;
 char    s[40];

 if (messageready) DeleteMessage();
 va_start(argptr,error);
 vsprintf(s,error,argptr);
 PrintXY(201,193,s);
 messageready=true;
 va_end(argptr);
 }


boolean AskQuestion(char *s)
{
 int     ans, c;
 boolean result;

 Message(s);
 ans=ReadKey();
 c=ans & 255;
 if (c=='Y' || c=='y')
  result=true;
 else
  result=false;
 while (kbhit())
  getch();
 DeleteMessage();
 return result;
 }


boolean AskFileName(char *s,char *name)
{
 char str[40];
 int  cursor, cindex, c, ans;

 strcpy(str,s);
 strcat(str,name);
 cindex=strlen(s);
 cursor=strlen(str)-1;
 while (cursor>cindex && str[cursor]==' ')
  --cursor;
 ++cursor;
 while (1)
  {
   DeleteMessage();
   str[cursor]='_';
   Message(str);
   ans=ReadKey();
   c=ans & 255;
   switch (c)
    {
     case 0x00:
     case 0xE0:
      c=(word)ans>>8;
      switch (c)
       {
	case 75:
	case 83:
	 if (cursor>cindex)
	  {
	   str[cursor]=' ';
	   --cursor;
	   str[cursor]='_';
	   }
	 break;
	default:
	 break;
	}
      break;
     case 8:
      if (cursor>cindex)
       {
	str[cursor]=' ';
	--cursor;
	str[cursor]='_';
	}
      break;
     case 13:
      str[cursor]=' ';
      strcpy(name,&str[strlen(s)]);
      DeleteMessage();
      return true;
     case 27:
      DeleteMessage();
      return false;
     default:
      str[cursor]=c;
      if (cursor-cindex<12)
       cursor++;
      str[cursor]='_';
      break;
     }
   }
 }


/* CONVERSION ***************************************************************/

void Convert1(char *s)
{
 FILE    *f;
 int     result, dest, source, i, j;
 layer_t *l[4];


 if (stricmp(s,"IGNORE")==0) return;
 for (i=0;i<4;i++)
  {
   l[i]=(layer_t*)malloc(sizeof(layer_t));
   if (l[i]==NULL) Error("Out of memory in Convert");
   memset(l[i],0,sizeof(layer_t));
   }
 f=fopen(s,"rt");
 if (f==NULL) Error("Error loading %s",s);
 do
  {
   result=fscanf(f,"%i %i\n",&dest,&source);

   for (i=0;i<YMAX;i++)
    for (j=0;j<XMAX;j++)
     if ((*layers[NORTHWALL])[i][j]==source && !(*l[0])[i][j])
      {
       (*layers[NORTHWALL])[i][j]=dest;
       (*l[0])[i][j]=1;
       }
   for (i=0;i<YMAX;i++)
    for (j=0;j<XMAX;j++)
     if ((*layers[WESTWALL])[i][j]==source && !(*l[1])[i][j])
      {
       (*layers[WESTWALL])[i][j]=dest;
       (*l[1])[i][j]=1;
       }
   for (i=0;i<YMAX;i++)
    for (j=0;j<XMAX;j++)
     if ((*layers[FLOORDEF])[i][j]==source && !(*l[2])[i][j])
      {
       (*layers[FLOORDEF])[i][j]=dest;
       (*l[2])[i][j]=1;
       }
   for (i=0;i<YMAX;i++)
    for (j=0;j<XMAX;j++)
     if ((*layers[CEILINGDEF])[i][j]==source && !(*l[3])[i][j])
      {
       (*layers[CEILINGDEF])[i][j]=dest;
       (*l[3])[i][j]=1;
       }
   } while (result==2);
 fclose(f);
 for (i=0;i<4;i++)
  free(l);
 }


void Convert2(char *s)
{
 FILE    *f;
 int     result, dest, source, i, j;
 layer_t *l[4];


 if (stricmp(s,"IGNORE")==0) return;
 for (i=0;i<4;i++)
  {
   l[i]=(layer_t*)malloc(sizeof(layer_t));
   if (l[i]==NULL) Error("Out of memory in Convert");
   memset(l[i],0,sizeof(layer_t));
   }
 f=fopen(s,"rt");
 if (f==NULL) Error("Error loading %s",s);
 do
  {
   result=fscanf(f,"%i %i\n",&dest,&source);

   for (i=0;i<YMAX;i++)
    for (j=0;j<XMAX;j++)
     if ((*layers[FLOOR])[i][j]==source && !(*l[0])[i][j])
      {
       (*layers[FLOOR])[i][j]=dest;
       (*l[0])[i][j]=1;
       }
   for (i=0;i<YMAX;i++)
    for (j=0;j<XMAX;j++)
     if ((*layers[CEILING])[i][j]==source && !(*l[1])[i][j])
      {
       (*layers[CEILING])[i][j]=dest;
       (*l[1])[i][j]=1;
       }
   } while (result==2);
 fclose(f);
 for (i=0;i<4;i++)
  free(l);
 }


void InitMEDIT(void)
{
 int i;

  /* initialize graphical data */
 screen=MK_FP(0xA000,0);
 for (i=0;i<SCREENHEIGHT;i++)
  ylookup[i]=screen+i*SCREENWIDTH;
 for (i=0;i<BUFY;i++)
  bufylookup[i]=buffer[i];

  /* init font info */
 tcolor=TEXTCOLOR;
 bkcolor=0;

  /* init cursor info */
 currentx=31;
 currenty=31;
 memset(currentvalue,0,sizeof(currentvalue));
 currentlayer=0;

  /* init layers */
 for (i=0;i<NUMLAYERS;i++)
  {
   layers[i]=(layer_t*)malloc(sizeof(layer_t));
   if (layers[i]==NULL)
    Error("Out of memory for layers");
   }
 }


long getlong(FILE *f)
{
 long l;

 fread(&l,4,1,f);
 return l;
 }


void ConvertLVL(char *s)
{
 FILE *f;
 long  l;
 int   i, j;
 byte  flags;

 f=fopen(s,"rb");
 if (f==NULL) Error("Error opening %s",s);

 for (i=0;i<YMAX;i++)
  for (j=0;j<XMAX;j++)
   {
    l=getlong(f);
    (*layers[WESTWALL])[i][j]=l&255;
    l>>=8;
    flags=0;
    if (l & 0x1)
     flags|=F_TRANSPARENT;
    if (l & 0x2)
     flags|=F_NOCLIP;
    if (l & 0x4)
     flags|=F_LEFT;
    if (l & 0x8)
     flags|=F_RIGHT;
    if (l & 0x10)
     flags|=F_NOBULLETCLIP;
    (*layers[WESTFLAGS])[i][j]=flags;
    }
 for (i=0;i<YMAX;i++)
  for (j=0;j<XMAX;j++)
   {
    l=getlong(f);
    (*layers[NORTHWALL])[i][j]=l&255;
    l>>=8;
    flags=0;
    if (l & 0x1)
     flags|=F_TRANSPARENT;
    if (l & 0x2)
     flags|=F_NOCLIP;
    if (l & 0x4)
     flags|=F_LEFT;
    if (l & 0x8)
     flags|=F_RIGHT;
    if (l & 0x10)
     flags|=F_NOBULLETCLIP;
    (*layers[NORTHFLAGS])[i][j]=flags;
    }
 for (i=0;i<YMAX;i++)
  for (j=0;j<XMAX;j++)
   {
    l=getlong(f);
    (*layers[FLOOR])[i][j]=l&255;
    l>>=8;
    flags=0;
    (*layers[FLOORFLAGS])[i][j]=flags;
    }
 for (i=0;i<YMAX;i++)
  for (j=0;j<XMAX;j++)
   {
    l=getlong(f);
    (*layers[CEILING])[i][j]=l&255;
    l>>=8;
    flags=0;
    if (l & 0x1)
     flags|=F_TRANSPARENT;
    (*layers[CEILINGFLAGS])[i][j]=flags;
    }
 for (i=0;i<YMAX;i++)
  for (j=0;j<XMAX;j++)
   (*layers[FLOORHEIGHT])[i][j]=fgetc(f);
 for (i=0;i<YMAX;i++)
  for (j=0;j<XMAX;j++)
   (*layers[CEILINGHEIGHT])[i][j]=fgetc(f);
 for (i=0;i<YMAX;i++)
  for (j=0;j<XMAX;j++)
   {
    l=getlong(f);
    (*layers[FLOORDEF])[i][j]=l&255;
    }
 for (i=0;i<YMAX;i++)
  for (j=0;j<XMAX;j++)
   {
    l=getlong(f);
    (*layers[CEILINGDEF])[i][j]=l&255;
    }
 for (i=0;i<YMAX;i++)
  for (j=0;j<XMAX;j++)
   (*layers[LIGHTS])[i][j]=fgetc(f);
 for (i=0;i<YMAX;i++)
  for (j=0;j<XMAX;j++)
   {
    l=getlong(f);
    (*layers[SPRITES])[i][j]=l&255;
    }
 for (i=0;i<YMAX;i++)
  for (j=0;j<XMAX;j++)
   {
    l=getlong(f);
    l&=255;
    if (l<32)
     (*layers[EFFECTS])[i][j]=l;
    else
     (*layers[SLOPES])[i][j]=l;
    }
 fclose(f);
 }


void CreateNewLAY(void)
{
 int i, j;

 for (i=0;i<NUMLAYERS;i++)
  memset(layers[i],0,sizeof(layer_t));

 for (i=0;i<YMAX;i++)
  for (j=0;j<XMAX;j++)
   (*layers[CEILINGHEIGHT])[i][j]=255;

 for (i=0;i<YMAX;i++)
  for (j=0;j<XMAX;j++)
   (*layers[LIGHTS])[i][j]=63;

 for (i=0;i<YMAX;i++)
  for (j=0;j<XMAX;j++)
   (*layers[CEILINGDEF])[i][j]=1;

 for (i=0;i<YMAX;i++)
  for (j=0;j<XMAX;j++)
   (*layers[FLOORDEF])[i][j]=1;
 }


/* DISK IO ******************************************************************/

void ReadLevel(char *s)
{
 FILE *f;
 int  i, result;
 char fname[20];

 strcpy(fname,s);

 /* upcase the string */
 i=0;
 while (fname[i]!=0)
  {
   if (fname[i]>='a' && fname[i]<='z')
    fname[i]+='A'-'a';
   i++;
   }

 /* find filename extention */
 i=0;
 while (fname[i]!=0 && fname[i]!='.') i++;
 if (fname[i]==0)
  strcat(fname,".LAY");                 // assume new file format
 else if (strcmp(&fname[i],".LVL")==0)  // old file format
  {
   CreateNewLAY();
   ConvertLVL(fname);
   strcpy(&fname[i],".LAY");
   sprintf(filename,"%-13s",fname);
   PrintfXY(233,10,filename);
   return;
   }

 sprintf(filename,"%-13s",fname);
 PrintfXY(233,10,filename);

 /* open and read */
 f=fopen(fname,"rb+");
 if (f==NULL)
  {
   CreateNewLAY();
   return;
   }
 for (i=0;i<NUMLAYERS;i++)
  if (!fread(layers[i],sizeof(layer_t),1,f))
   Error("Reading %s, layer %i",fname,i);
 fclose(f);
 }


void ReadFile(char *filename,word length,void *buffer)
{
 FILE *f;

 f=fopen(filename,"rb");
 if (f==NULL)
  Error("Error opening %s",filename);
 if (fread(buffer,length,1,f)==0)
  Error("Error reading %s",filename);
 fclose(f);
 }


void SaveLAYFile(void)
{
 int   i;
 FILE *f;

 if (!AskFileName("Save: ",filename))
  return;
 i=0;
 while (filename[i]!=0)
  {
   if (filename[i]>='a' && filename[i]<='z')
    filename[i]+='A'-'a';
   i++;
   }
 PrintfXY(233,10,filename);
 f=fopen(filename,"wb");
 if (f==NULL)
  Error("Error saving %s",filename);
 for (i=0;i<NUMLAYERS;i++)
  if (!fwrite(layers[i],sizeof(layer_t),1,f))
   Error("Error writing %s",filename);
 fclose(f);
 Message("File Saved");
 }


void LoadLAYFile(void)
{
 int   i;
 FILE *f;

 if (!AskFileName("Load: ",filename))
  return;

 i=0;
 while (filename[i]!=0)
  {
   if (filename[i]>='a' && filename[i]<='z')
    filename[i]+='A'-'a';
   i++;
   }
 PrintfXY(233,10,filename);
 f=fopen(filename,"rb");
 if (f==NULL)
  Message("%s not found",filename);
 for (i=0;i<NUMLAYERS;i++)
  if (!fread(layers[i],sizeof(layer_t),1,f))
   Error("Error reading %s",filename);
 fclose(f);
 Message("File Loaded");
 }


/* Buffer Functions *********************************************************/

void BlitBuffer(void)
{
 int i;

 for (i=0;i<BUFY;i++)
  memcpy(ylookup[i+5]+5,bufylookup[i],BUFY);
 }


void ResetBuffer(void)
{
 int i, j;

 memset(buffer,0,sizeof(buffer));
 for (i=0;i<BUFY;i+=10)
  memset(bufylookup[i],1,BUFX);
 for (j=0;j<BUFX;j+=10)
  for (i=0;i<BUFY;i++)
   *(bufylookup[i]+j)=1;
 }


void DrawBuffer(void)
{
 int tilex, tiley, color, x, y, i, j, upper, lower, cw, ch;

 ResetBuffer();
 y=0;

 switch (currentlayer)
  {
   case FLOORDEF:
   case CEILINGDEF:
    upper=CEILINGDEF;
    lower=FLOORDEF;
    break;
   case CEILINGHEIGHT:
   case FLOORHEIGHT:
    upper=CEILINGHEIGHT;
    lower=FLOORHEIGHT;
    break;
   default:
    upper=CEILING;
    lower=FLOOR;
    break;
   }

 for (tiley=currenty-9;tiley<=currenty+9;tiley++,y+=10)
  if (tiley>=0 && tiley<YMAX)
   {
    x=0;
    for (tilex=currentx-9;tilex<=currentx+9;tilex++,x+=10)
     if (tilex>=0 && tilex<XMAX)
      {
       color=(*layers[lower])[tiley][tilex];
       if (color)
	{
	 color=(color%240) + 16;
	 for (i=1;i<10;i++)
	  memset(bufylookup[y+i]+x+1,color,9);
	 }

       color=(*layers[upper])[tiley][tilex];
       if (color)
	{
	 color=(color%240) + 16;
	 for (i=1;i<10;i++)
	  for (j=10-i;j>0;j--)
	   *(bufylookup[y+i]+x+j)=color;
	 }

       color=(*layers[NORTHWALL])[tiley][tilex];
       if (color)
	{
	 if (lower!=FLOOR)
	  color=5;
	 else
	  color=(color%240) + 16;
	 memset(bufylookup[y]+x,color,10);
	 memset(bufylookup[y+1]+x,color,10);
	 }

       color=(*layers[WESTWALL])[tiley][tilex];
       if (color)
	{
	 if (lower!=FLOOR)
	  color=5;
	 else
	  color=(color%240) + 16;
	 for (i=0;i<10;i++)
	  *(bufylookup[y+i]+x)=color;
	 for (i=0;i<10;i++)
	  *(bufylookup[y+i]+x+1)=color;
	 }

       color=(*layers[SPRITES])[tiley][tilex];
       if (color)
	{
	 color=(color%240) + 16;
	 *(bufylookup[y+4]+x+4)=color;
	 *(bufylookup[y+5]+x+4)=color;
	 *(bufylookup[y+6]+x+4)=color;
	 *(bufylookup[y+4]+x+5)=color;
	 *(bufylookup[y+5]+x+5)=color;
	 *(bufylookup[y+6]+x+5)=color;
	 *(bufylookup[y+4]+x+6)=color;
	 *(bufylookup[y+5]+x+6)=color;
	 *(bufylookup[y+6]+x+6)=color;
	 }
       if ((*layers[SLOPES])[tiley][tilex])
	*(bufylookup[y+5]+x+5)=3;
       }
     else
      for (i=0;i<10;i++)
       memset(bufylookup[y+i]+x,0,10);
    }
  else
   for (i=0;i<10;i++)
    memset(bufylookup[y+i],0,BUFX);

 if (currentlayer==WESTWALL)
  {
   cw=4;
   ch=12;
   }
 else if (currentlayer==NORTHWALL)
  {
   cw=12;
   ch=4;
   }
 else
  {
   cw=13;
   ch=13;
   }
 memset(bufylookup[89]+89,6,cw);
 memset(bufylookup[88+ch]+89,6,cw);
 for (i=90;i<89+ch;i++)
  {
   *(bufylookup[i]+89)=6;
   *(bufylookup[i]+88+cw)=6;
   }
 }


/* Edit Functions ***********************************************************/

void FloodFill(void)
{
 byte *x, *y, *processed;
 int   index, total, curx, cury;

 index=0;
 total=1;

 x=(byte *)malloc(YMAX*XMAX);
 if (x==NULL)
  Error("Out of memory for flood fill (1)");
 y=(byte *)malloc(YMAX*XMAX);
 if (y==NULL)
  Error("Out of memory for flood fill (2)");
 processed=(byte *)malloc(YMAX*XMAX);
 if (processed==NULL)
  Error("Out of memory for flood fill (3)");

 memset(x,0,XMAX*YMAX);
 memset(y,0,XMAX*YMAX);
 memset(processed,0,XMAX*YMAX);

 x[0]=currentx;
 y[0]=currenty;
 processed[currenty*XMAX+currentx]=1;
 while (index<total)
  {
   curx=x[index];
   cury=y[index];
   index++;
   if (!(*layers[NORTHWALL])[cury][curx] && cury-1>=0)
    {
     if (!processed[(cury-1)*XMAX+curx])
      {
       x[total]=curx;
       y[total]=cury-1;
       total++;
       processed[(cury-1)*XMAX+curx]=1;
       }
     }
   if (!(*layers[WESTWALL])[cury][curx] && curx-1>=0)
    {
     if (!processed[(cury)*XMAX+curx-1])
      {
       x[total]=curx-1;
       y[total]=cury;
       total++;
       processed[(cury)*XMAX+curx-1]=1;
       }
     }
   if (cury<YMAX-1 && !(*layers[NORTHWALL])[cury+1][curx])
    {
     if (!processed[(cury+1)*XMAX+curx])
      {
       x[total]=curx;
       y[total]=cury+1;
       total++;
       processed[(cury+1)*XMAX+curx]=1;
       }
     }
   if (curx<XMAX-1 && !(*layers[WESTWALL])[cury][curx+1])
    {
     if (!processed[(cury)*XMAX+curx+1])
      {
       x[total]=curx+1;
       y[total]=cury;
       total++;
       processed[(cury)*XMAX+curx+1]=1;
       }
     }
   if (total>=XMAX*YMAX)
    Error("Flood fill overflow");
   (*layers[currentlayer])[cury][curx]=currentvalue[currentlayer];
   }
 free(x);
 free(y);
 free(processed);
 }


void UpdateLocalInfo(void)
{
 char *s;
 byte flags;

 PrintXY(233,20,layernames[currentlayer]);
 PrintfXY(233,30,"%3i",currentvalue[currentlayer]);
 PrintfXY(263,30,"%2i",currentx);
 PrintfXY(288,30,"%2i",currenty);
 PrintfXY(236,52,"%3i",(*layers[NORTHWALL])[currenty][currentx]);
 PrintfXY(255,52,"%3i",(*layers[WESTWALL])[currenty][currentx]);
 PrintfXY(236,63,"%3i",(*layers[FLOOR])[currenty][currentx]);
 PrintfXY(255,63,"%3i",(*layers[CEILING])[currenty][currentx]);
 PrintfXY(236,74,"%3i",(*layers[FLOORHEIGHT])[currenty][currentx]);
 PrintfXY(255,74,"%3i",(*layers[CEILINGHEIGHT])[currenty][currentx]);
 PrintfXY(236,85,"%3i",(*layers[FLOORDEF])[currenty][currentx]);
 PrintfXY(255,85,"%3i",(*layers[CEILINGDEF])[currenty][currentx]);
 PrintfXY(236,96,"%3i",(*layers[LIGHTS])[currenty][currentx]);
 PrintfXY(255,96,"%3i",(*layers[EFFECTS])[currenty][currentx]);
 PrintfXY(236,107,"%3i",(*layers[SPRITES])[currenty][currentx]);
 PrintfXY(255,107,"%3i",(*layers[SLOPES])[currenty][currentx]);

 switch (currentlayer)
  {
   case NORTHWALL:
   case WESTWALL:
   case FLOORDEF:
   case CEILINGDEF:
   case FLOOR:
   case CEILING:
    flags=(*layers[currentlayer+1])[currenty][currentx];
    if (flags & F_RIGHT)
     s="Yes";
    else
     s="No ";
    PrintXY(236,130,s);
    if (flags & F_LEFT)
     s="Yes";
    else
     s="No ";
    PrintXY(255,130,s);
    if (flags & F_UP)
     s="Yes";
    else
     s="No ";
    PrintXY(236,141,s);
    if (flags & F_DOWN)
     s="Yes";
    else
     s="No ";
    PrintXY(255,141,s);
    if (flags & F_TRANSPARENT)
     s="Yes";
    else
     s="No ";
    PrintXY(236,152,s);
    if (flags & F_NOCLIP)
     s="Yes";
    else
     s="No ";
    PrintXY(255,152,s);
    if (flags & F_NOBULLETCLIP)
     s="Yes";
    else
     s="No ";
    PrintXY(236,163,s);
    if (flags & F_DAMAGE)
     s="Yes";
    else
     s="No ";
    PrintXY(255,163,s);
    break;
   default:
    PrintXY(236,130,"No ");
    PrintXY(255,130,"No ");
    PrintXY(236,141,"No ");
    PrintXY(255,141,"No ");
    PrintXY(236,152,"No ");
    PrintXY(255,152,"No ");
    PrintXY(236,163,"No ");
    PrintXY(255,163,"No ");
    break;
   }
 }



/* MAIN *********************************************************************/

void MainLoop(void)
{
 int c, ans, i;

 UpdateLocalInfo();
 DrawBuffer();
 BlitBuffer();
 while (!quit)
  {
   ans=ReadKey();
   if (messageready) DeleteMessage();
   c=ans & 255;
   switch (c)
    {
     case 0x00:
     case 0xE0:
      c=(word)ans>>8;
      switch (c)
       {
	 /* keypad controls */
	case 75:
	 if (currentx>0)
	  {
	   --currentx;
	   if (*(byte*)MK_FP(0x0040,0x0017)&64)
	    (*layers[currentlayer])[currenty][currentx]=currentvalue[currentlayer];
	   UpdateLocalInfo();
	   DrawBuffer();
	   BlitBuffer();
	   }
	 break;
	case 77:
	 if (currentx<XMAX-1)
	  {
	   ++currentx;
	   if (*(byte*)MK_FP(0x0040,0x0017)&64)
	    (*layers[currentlayer])[currenty][currentx]=currentvalue[currentlayer];
	   UpdateLocalInfo();
	   DrawBuffer();
	   BlitBuffer();
	   }
	 break;
	case 72:
	 if (currenty>0)
	  {
	   --currenty;
	   if (*(byte*)MK_FP(0x0040,0x0017)&64)
	    (*layers[currentlayer])[currenty][currentx]=currentvalue[currentlayer];
	   UpdateLocalInfo();
	   DrawBuffer();
	   BlitBuffer();
	   }
	 break;
	case 80:
	 if (currenty<YMAX-1)
	  {
	   ++currenty;
	   if (*(byte*)MK_FP(0x0040,0x0017)&64)
	    (*layers[currentlayer])[currenty][currentx]=currentvalue[currentlayer];
	   UpdateLocalInfo();
	   DrawBuffer();
	   BlitBuffer();
	   }
	 break;
	case 73:
	 currenty-=10;
	 if (currenty<0) currenty=0;
	 if (*(byte*)MK_FP(0x0040,0x0017)&64)
	  (*layers[currentlayer])[currenty][currentx]=currentvalue[currentlayer];
	 UpdateLocalInfo();
	 DrawBuffer();
	 BlitBuffer();
	 break;
	case 81:
	 currenty+=10;
	 if (currenty>=YMAX) currenty=YMAX;
	 if (*(byte*)MK_FP(0x0040,0x0017)&64)
	  (*layers[currentlayer])[currenty][currentx]=currentvalue[currentlayer];
	 UpdateLocalInfo();
	 DrawBuffer();
	 BlitBuffer();
	 break;
	case 71:
	 currentx-=10;
	 if (currentx<0) currentx=0;
	 if (*(byte*)MK_FP(0x0040,0x0017)&64)
	  (*layers[currentlayer])[currenty][currentx]=currentvalue[currentlayer];
	 UpdateLocalInfo();
	 DrawBuffer();
	 BlitBuffer();
	 break;
	case 79:
	 currentx+=10;
	 if (currentx>=YMAX) currentx=YMAX;
	 if (*(byte*)MK_FP(0x0040,0x0017)&64)
	  (*layers[currentlayer])[currenty][currentx]=currentvalue[currentlayer];
	 UpdateLocalInfo();
	 DrawBuffer();
	 BlitBuffer();
	 break;

	 /* flood/fill commands */
	case 33: // alt-f
	 if (AskQuestion("Fill? (y/n)"))
	  {
	   memset(layers[currentlayer],currentvalue[currentlayer],sizeof(layer_t));
	   UpdateLocalInfo();
	   Drawbuffer();
	   BlitBuffer();
	   }
	 break;

	case 36: // alt-j
	 if (AskQuestion("Flood? (y/n)"))
	  {
	   FloodFill();
	   UpdateLocalInfo();
	   Drawbuffer();
	   BlitBuffer();
	   }
	 break;

	 /* flag setting commands */
	case 59:
	 switch (currentlayer)
	  {
	   case NORTHWALL:
	   case WESTWALL:
	   case FLOORDEF:
	   case CEILINGDEF:
	   case FLOOR:
	   case CEILING:
	    (*layers[currentlayer+1])[currenty][currentx]^=F_RIGHT;
	    UpdateLocalInfo();
	    break;
	   }
	 break;
	case 60:
	 switch (currentlayer)
	  {
	   case NORTHWALL:
	   case WESTWALL:
	   case FLOORDEF:
	   case CEILINGDEF:
	   case FLOOR:
	   case CEILING:
	    (*layers[currentlayer+1])[currenty][currentx]^=F_LEFT;
	    UpdateLocalInfo();
	    break;
	   }
	 break;
	case 61:
	 switch (currentlayer)
	  {
	   case NORTHWALL:
	   case WESTWALL:
	   case FLOORDEF:
           case CEILINGDEF:
	   case FLOOR:
           case CEILING:
            (*layers[currentlayer+1])[currenty][currentx]^=F_UP;
            UpdateLocalInfo();
            break;
           }
         break;
        case 62:
         switch (currentlayer)
          {
	   case NORTHWALL:
           case WESTWALL:
	   case FLOORDEF:
           case CEILINGDEF:
           case FLOOR:
           case CEILING:
            (*layers[currentlayer+1])[currenty][currentx]^=F_DOWN;
            UpdateLocalInfo();
            break;
           }
	 break;
        case 64:
         switch (currentlayer)
          {
	   case NORTHWALL:
           case WESTWALL:
            (*layers[currentlayer+1])[currenty][currentx]^=F_NOCLIP;
            UpdateLocalInfo();
            break;
           }
         break;
        case 63:
         switch (currentlayer)
          {
           case NORTHWALL:
           case WESTWALL:
           case FLOORDEF:
           case CEILINGDEF:
           case CEILING:
            (*layers[currentlayer+1])[currenty][currentx]^=F_TRANSPARENT;
	    UpdateLocalInfo();
            break;
           }
         break;
        case 65:
         switch (currentlayer)
          {
           case NORTHWALL:
           case WESTWALL:
            (*layers[currentlayer+1])[currenty][currentx]^=F_NOBULLETCLIP;
	    UpdateLocalInfo();
            break;
	   }
         break;
        case 66:
         switch (currentlayer)
          {
           case NORTHWALL:
           case WESTWALL:
           case FLOOR:
           case CEILING:
            (*layers[currentlayer+1])[currenty][currentx]^=F_DAMAGE;
	    UpdateLocalInfo();
            break;
           }
         break;

         /* file commands */
        case 31: // alt-s
	 SaveLAYFile();
         break;
        case 38: // alt-l
         LoadLayFile();
         UpdateLocalInfo();
         Drawbuffer();
         BlitBuffer();
         break;


         /* quit command */
	case 45: // alt-x
        case 16: // alt-q
         if (AskQuestion("Quit? (y/n)")) quit=true;
         break;

        }
      break;

       /* value commands */
     case ' ':
      (*layers[currentlayer])[currenty][currentx]=currentvalue[currentlayer];
      UpdateLocalInfo();
      Drawbuffer();
      BlitBuffer();
      break;
     case '.':
     case '+':
      currentvalue[currentlayer]++;
      currentvalue[currentlayer]&=255;
      UpdateLocalInfo();
      break;
     case ',':
     case '-':
      currentvalue[currentlayer]--;
      currentvalue[currentlayer]&=255;
      UpdateLocalInfo();
      break;
     case 'z':
     case 'Z':
      currentvalue[currentlayer]=(*layers[currentlayer])[currenty][currentx];
      UpdateLocalInfo();
      break;
     case 'x':
     case 'X':
      (*layers[currentlayer])[currenty][currentx]=0;
      UpdateLocalInfo();
      DrawBuffer();
      BlitBuffer();
      break;
     case 'c':
     case 'C':
      for (i=0;i<NUMLAYERS;i++)
       currentvalue[i]=(*layers[i])[currenty][currentx];
      UpdateLocalInfo();
      break;
     case 'v':
     case 'V':
      for (i=0;i<NUMLAYERS;i++)
       (*layers[i])[currenty][currentx]=currentvalue[i];
      UpdateLocalInfo();
      DrawBuffer();
      BlitBuffer();
      break;

      /* layer commands */
     case 9:
      switch (currentlayer)
       {
        case NORTHWALL:
         currentlayer=WESTWALL;
         break;
        case WESTWALL:
         currentlayer=NORTHWALL;
         break;
        case FLOOR:
         currentlayer=CEILING;
	 break;
        case CEILING:
         currentlayer=FLOOR;
         break;
        case FLOORHEIGHT:
         currentlayer=CEILINGHEIGHT;
         break;
        case CEILINGHEIGHT:
         currentlayer=FLOORHEIGHT;
         break;
        case CEILINGDEF:
         currentlayer=FLOORDEF;
         break;
        case FLOORDEF:
         currentlayer=CEILINGDEF;
         break;
        }
      UpdateLocalInfo();
      DrawBuffer();
      BlitBuffer();
      break;
     case '1':
      currentlayer=NORTHWALL;
      UpdateLocalInfo();
      Drawbuffer();
      BlitBuffer();
      break;
     case '2':
      currentlayer=WESTWALL;
      UpdateLocalInfo();
      Drawbuffer();
      BlitBuffer();
      break;
     case '3':
      currentlayer=FLOOR;
      UpdateLocalInfo();
      Drawbuffer();
      BlitBuffer();
      break;
     case '4':
      currentlayer=CEILING;
      UpdateLocalInfo();
      Drawbuffer();
      BlitBuffer();
      break;
     case '5':
      currentlayer=FLOORHEIGHT;
      UpdateLocalInfo();
      Drawbuffer();
      BlitBuffer();
      break;
     case '6':
      currentlayer=CEILINGHEIGHT;
      UpdateLocalInfo();
      Drawbuffer();
      BlitBuffer();
      break;
     case '7':
      currentlayer=FLOORDEF;
      UpdateLocalInfo();
      Drawbuffer();
      BlitBuffer();
      break;
     case '8':
      currentlayer=CEILINGDEF;
      UpdateLocalInfo();
      Drawbuffer();
      BlitBuffer();
      break;
     case '9':
      currentlayer=LIGHTS;
      UpdateLocalInfo();
      Drawbuffer();
      BlitBuffer();
      break;
     case '0':
      currentlayer=EFFECTS;
      UpdateLocalInfo();
      Drawbuffer();
      BlitBuffer();
      break;
     case 'q':
     case 'Q':
      currentlayer=SPRITES;
      UpdateLocalInfo();
      Drawbuffer();
      BlitBuffer();
      break;
     case 'w':
     case 'W':
      currentlayer=SLOPES;
      UpdateLocalInfo();
      Drawbuffer();
      BlitBuffer();
      break;

      /* quit */
     case 27:
      if (AskQuestion("Quit? (y/n)")) quit=true;
      break;
     }
   }
 }


cdecl main(int argc, char **argv)
{
 /* Initialize */
 InitMEDIT();
 SetVidMode(VGAMODE);
 LoadCPR("MEDIT.CPR");
 LoadFON("BLOCK.FON",FONTWIDTH);

 /* check parameters */
 if (argc==1) Error("No LAY/LVL file specified");
  else if (argc>4) Error("Too many parameters");
  else
  {
   ReadLevel(argv[1]);
   if (argc>=3) Convert1(argv[2]);
   if (argc>=4) Convert2(argv[3]);
   MainLoop();
   }

 /* end message */
 SetVidMode(TEXTMODE);
 printf("MEDIT (v%s)\n"
	"LAY/LVL File Editor\n"
	"Copyright (C) 1995 by Robert Morgan of Channel 7\n"
	"All rights reserved.\n\n",VERSION);
 }
