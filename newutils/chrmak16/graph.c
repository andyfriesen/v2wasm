// graph.c
#include <dpmi.h>
#include <sys\nearptr.h>
#include <go32.h>

#define word unsigned short int
#define byte unsigned char

typedef struct {
 word ModeAttributes;
 byte WinAAttributes;
 byte WinBAttributes;
 word WinGranularity;
 word WinSize;
 word WinASegment;
 word WinBSegment;
 word WinFuncPtr[2]; // pointer
 word BytesPerScanLine;

 word XRes;
 word YRes;
 byte XCharSize;
 byte YCharSize;
 byte NumberOfPlanes;
 byte BitsPerPixel;
 byte NumberOfBanks;
 byte MemoryModel;
 byte BankSize;
 byte NumberOfImagePages;
 byte Reserved;

 byte RedMaskSize;
 byte RedFieldPosition;
 byte GreenMaskSize;
 byte GreenFieldPosition;
 byte BlueMaskSize;
 byte BlueFieldPosition;
 byte RsvdMasksize;
 byte DirectColorModeInfo;
 byte reserved[216];
} ModeInfoBlock;

typedef struct {
 char VESASignature[4];
 word VESAVersion;
 word OEMStrintptr[2];
 char Capabilities[4];
 word VideoModePtr[2];
 word TotalMemory;
 char reserved[236];
} VGAInfoBlock;

__dpmi_regs graphregs;

//              DATA

unsigned int *scr; // for 24/32 bit modes
word *scrw;        // for 16 bit modes
word *physscr;     // physical screen pointer
word wingranularity;
word modenum;
word numbanks;
unsigned int scrsize; // number of bytes in screen
word xres,yres;
byte bshift,gshift,rshift;
byte colordepth;
byte getpixelr,getpixelg,getpixelb;

//              CODE

ModeInfoBlock *getmodeinfo(word mode)
{
 ModeInfoBlock info;

 graphregs.x.ax=0x4F01;
 graphregs.x.cx=mode;
 graphregs.x.es=__tb/16;
 graphregs.x.di=0;
 __dpmi_int(0x10,&graphregs);
 dosmemget(__tb, sizeof(ModeInfoBlock), &info);
 return &info;
}

VGAInfoBlock *getvgainfo()
{
 VGAInfoBlock info;
 graphregs.x.ax=0x4F00;
 graphregs.x.es=__tb/16;
 graphregs.x.di=0;
 __dpmi_int(0x10,&graphregs);
 dosmemget(__tb, sizeof(VGAInfoBlock), &info);
 return &info;
}

void setvesamode(word modenum)
{
 graphregs.x.ax=0x4F02;
 graphregs.x.bx=modenum;
 __dpmi_int(0x10,&graphregs);
}

int initgraph(word x,word y)
/* finds a graphics mode of (x,y)and sets it. returns the VESA mode number on success, 0 if not
   it'll get a 16 bit mode ideally, if it can't find it it'll use a 24 or 32 bit mode
   since I don't know crap all about protected mode stuff. It calls a (real mode)
   program called GETMODE.EXE to find the right mode for it (I can't make it work
   in C for some reason)
*/
{
 ModeInfoBlock *modeinfo;
 int *p;
 int i,m;
 char sys[255];
 char t[255];
 char a[] = "GETMODE ";

 strcpy(sys,a);

 itoa(x,t,10);
 strcat(sys,t);
 strcat(sys," "); // sys:='GETMODE '+inttostr(x)+' '+inttostr(y);
 itoa(y,t,10);
 strcat(sys,t);  // pascal beats C hands down when it comes to strings :)

 m=system(sys)&255; // find out which mode number to use
 m+=0x100; // system only returns lower 8 bits

 modenum=m;

 setvesamode(m); // ACTUALLY SET THE GRAPHICS MODE!!!

 // set some variables (so the program knows what it's got on the screen

 modeinfo=getmodeinfo(modenum);

 xres=x; yres=y;
 wingranularity=modeinfo->WinGranularity;
 numbanks=modeinfo->NumberOfBanks;
 bshift=modeinfo->BlueFieldPosition;
 gshift=modeinfo->GreenFieldPosition;
 rshift=modeinfo->RedFieldPosition;

 // if it's more than 15 bit, then we IGNORE THE EXTRA BITS
 // (doesn't look as good but infinitely more flexible)
 bshift+=(modeinfo->BlueMaskSize)-5;
 gshift+=(modeinfo->GreenMaskSize)-5;
 rshift+=(modeinfo->RedMaskSize)-5;

 colordepth=modeinfo->BitsPerPixel;

 setvesamode(modenum);

 scrsize=(xres*yres*(colordepth/8));
 scrw=(word *)malloc(scrsize);
 scr=scrw;
 __djgpp_nearptr_enable();
 physscr=modeinfo->WinASegment*16+__djgpp_conventional_base;
// if (!scr) err("Unable to allocate memory for graphics");

 memset(scrw,0,scrsize);
}

void putpixel(int x,int y,int c)
{
 // it really sucks that this has to be done this way :(
 // it's really slow, but it works
 // NOTE TO SELF: convert this to ASM
 int r,g,b;
 b=c&31;
 g=(c>>5)&31;
 r=(c>>10)&31; // first separate them, then recombine (to allow for different hardware)
 scrw[y*xres+x]=(r<<rshift)+(g<<gshift)+(b<<bshift);
}

void getpixel(int x,int y)
{
 int a;
 a=scrw[y*xres*2+x];
 getpixelr=(a>>rshift)&63;
 getpixelg=(a>>gshift)&63;
 getpixelb=(a>>bshift)&63;
}

void closegraph()
// return to text mode (duh!)
{
 graphregs.x.ax=3;
 __dpmi_int(0x10,&graphregs);
}

int showpage(void)
/* copies the double buffer into the actual video memory
   this is more difficult because page flipping is necessary
   until I figure out how to do a linear framebuffer (which
   won't be soon)                                             */
{
 int curpage,endbank;

 endbank=(yres*xres*2)/wingranularity/1024;
 curpage=0;
 for (curpage=0; curpage<endbank; curpage+=wingranularity>>6)
  {
   graphregs.x.ax=0x4f05;
   graphregs.x.bx=0;
   graphregs.x.dx=curpage;
   __dpmi_int(0x10,&graphregs);
   memcpy(physscr,scrw+(curpage<<15),65536);
  }

 graphregs.x.ax=0x4F05;
 graphregs.x.bx=0;
 graphregs.x.dx=endbank;
 __dpmi_int(0x10,&graphregs);
 memcpy(physscr,scrw+(endbank<<15),scrsize&65535);

 return 0;
}

void clearscreen()
{
 memset(scrw,0,scrsize);
}
