//
#include <stdio.h>
#define word unsigned short int
#define byte unsigned char
//include "graph.h"

FILE *f;
word *buf;
int width,height;
int fontheight,fontwidth,numsubsets;

void loadbmp(char *fname)
{
 FILE *f;
 int x,y;
 byte r,g,b;
 char temp[5];

 f=fopen(fname,"rb");
 fseek(f,18,SEEK_SET);
 fread(&width,1,4,f);
 fread(&height,1,4,f);

 buf=(word *)malloc(width*height*2);

 fseek(f,54,SEEK_SET);

 for (y=height-1; y>=0; y--)
  {
   for (x=0; x<=width-1; x++)
    {
     fread(&b,1,1,f);
     fread(&g,1,1,f);
     fread(&r,1,1,f);
     r=r>>3; g=g>>3; b=b>>3;
     buf[y*width+x]=(r<<10)+(g<<5)+b;
    }
   fread(&temp,1,width&3,f);
  }
 fclose(f);
}

word getdot(int x,int y)
{
 return buf[y*width+x];
}

void dumptile(int x1,int y1)
{
 int  x,y;
 word z;

 for (y=0; y<fontheight; y++)
  for (x=0; x<fontwidth; x++)
   {
    z=getdot(x+x1,y+y1);
    fwrite(&z,1,2,f);
   }
}

main(int argcount, char *arg[])
{
 char *srcbmp,*destfnt;
 int x,y,z;
 byte ver=2;

 if (argcount!=6)
  {
   cprintf("BMP2FONT.EXE by the Speed Bump (aka Andy Friesen)\r\n");
   cprintf("Based upon PCX2FNT.EXE by vecna\r\n\r\n");
   cprintf(" Syntax: BMP2FONT source.bmp dest.fnt width height numsubsets\r\n");
   exit(0);
  }

 srcbmp=arg[1];
 destfnt=arg[2];
 fontwidth=atoi(arg[3]);
 fontheight=atoi(arg[4]);
 numsubsets=atoi(arg[5]);

 loadbmp(srcbmp);
 f=fopen(destfnt,"wb");
 fwrite(&ver,1,1,f); // version 2 - highcolor, non-compressed
 fwrite(&fontwidth,1,2,f);
 fwrite(&fontheight,1,2,f);
 fwrite(&numsubsets,1,2,f);

 for (z=0; z<numsubsets; z++)
  for (y=0; y<5; y++)
   for (x=0; x<20; x++)
    {
     if ((y*20)+x<96)
      dumptile((x*(fontwidth+1))+1,(y*(fontheight+1))+(1+(fontheight+1)*(z*5)));
    }
}
