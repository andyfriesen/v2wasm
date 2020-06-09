// CHRMAK16.C
// This is a gigantic lump of poopoo and I am well aware of it! --tSB

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#define word unsigned short int
#define byte unsigned char

FILE *bmp,*mak,*chr;
char token[100],makefile[100],bmpfile[100],chrfile[100];

typedef struct
{
  byte *imagedata;                   // CHR frame data
  word fxsize, fysize;                // frame x/y dimensions
  word hx, hy;                        // x/y obstruction hotspot
  word hw, hh;                        // hotspot width/height
  word totalframes;                   // total # of frames.
  word lidle, ridle;
  word uidle, didle;
  char lanim[100];
  char ranim[100];
  char uanim[100];
  char danim[100];
} chrdata;

chrdata c;
int perrow;

word *buf;          // pointer to the BMP data
int width,height;

word *cData,*rData; // compressed/raw CHR data

void err(char *s)
{
 cprintf(s);
 exit(255);
}

void gettoken()
{
 fscanf(mak,"%s",token);
}

int tokenis(char *s)
{
 return !strcmp(s,token);
}

void WriteCompressedLayer2(word *dest,int numwords,int& bufsize,word *src)
{
  int i;
  word byt,samect;
  byte repcode;

  i=0; bufsize=0;
  do
  {   byt=src[i++];
      samect=1;
      while (samect<255 && i<numwords && byt==src[i])
      {
         samect++;
         i++;
      }
      if (samect>1 || ((byt&0xFF00)==0xFF00))  // oopsies! - tSB
      {
         *dest++=(0xFF00)+samect;
         bufsize+=2;
      }
      *dest++=byt;
      bufsize+=2;
  } while (i<numwords);
}

void parsemakefile(char *fname)
{
 int done=0;

 mak=fopen(fname,"rb");
 while (!done)
  {
   gettoken();
   if (tokenis("bmpname"))
    {
     gettoken();
     strcpy(bmpfile,token);
     continue;
    }
   else if (tokenis("chrname"))
    {
     gettoken();
     strcpy(chrfile,token);
     continue;
    }
   else if (tokenis("frame_w"))
    {
     gettoken();
     c.fxsize=atoi(token);
     continue;
    }
   else if (tokenis("frame_h"))
    {
     gettoken();
     c.fysize=atoi(token);
     continue;
    }
   else if (tokenis("hot_x"))
    {
     gettoken();
     c.hx=atoi(token);
     continue;
    }
   else if (tokenis("hot_y"))
    {
     gettoken();
     c.hy=atoi(token);
     continue;
    }
   else if (tokenis("hot_w"))
    {
     gettoken();
     c.hw=atoi(token);
     continue;
    }
   else if (tokenis("hot_h"))
    {
     gettoken();
     c.hh=atoi(token);
     continue;
    }
   else if (tokenis("per_row"))
    {
     gettoken();
     perrow=atoi(token);
     continue;
    }
   else if (tokenis("total_frames"))
    {
     gettoken();
     c.totalframes=atoi(token);
     continue;
    }
   else if (tokenis("lidle"))
    {
     gettoken();
     c.lidle=atoi(token);
     continue;
    }
   else if (tokenis("ridle"))
    {
     gettoken();
     c.ridle=atoi(token);
     continue;
    }
   else if (tokenis("uidle"))
    {
     gettoken();
     c.uidle=atoi(token);
     continue;
    }
   else if (tokenis("didle"))
    {
     gettoken();
     c.didle=atoi(token);
     continue;
    }
   else if (tokenis("lscript"))
    {
     gettoken();
     strcpy(c.lanim,token);
     continue;
    }
   else if (tokenis("rscript"))
    {
     gettoken();
     strcpy(c.ranim,token);
     continue;
    }
   else if (tokenis("uscript"))
    {
     gettoken();
     strcpy(c.uanim,token);
     continue;
    }
   else if (tokenis("dscript"))
    {
     gettoken();
     strcpy(c.danim,token);
     continue;
    }
   else if (tokenis("end"))
    done=1;
  }
}

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

 width; height;

 fseek(f,54,SEEK_SET);

 for (y=height-1; y>=0; y--)
  {
   for (x=0; x<=width-1; x++)
    {
     fread(&b,1,1,f);
     fread(&g,1,1,f);
     fread(&r,1,1,f);
     r=r>>3; g=g>>2; b=b>>3;
     buf[y*width+x]=(r<<11)+(g<<5)+b;
    }
   fread(&temp,1,width&3,f);
  }
 fclose(f);
}

word getdot(int x,int y)
{
 return buf[y*width+x];
}

void dumpframe(int x1,int y1)
{
 int x,y;
 word a;
 static int offset=0;

 for (y=0; y<c.fysize; y++)
  for (x=0; x<c.fxsize; x++)
   {
    a=getdot(x+x1,y+y1);
    rData[offset++]=a;
//    fwrite(&a,1,2,chr);
//    putpixel(x+x1,y+y1,a);
   }
}

void makechr(void)
{
 int fx,fy,x,y;
 int curframe=0;
 int bufsize;
 fx=0; fy=0;

 cData=(word*)malloc(c.totalframes*c.fxsize*c.fysize*2);
 rData=(word*)malloc(c.totalframes*c.fxsize*c.fysize*2);

 // First, collect the raw pixel data in a format that we can use (an unpadded, vertical strip)
 while (curframe<c.totalframes)
  {
   dumpframe(fx*(c.fxsize+1)+1,fy*(c.fysize+1)+1);
   fx++;
   if (fx>=perrow)
    {
     fx=0;
     fy++;
    }
   curframe++;
  }

 cprintf("Compressing...\r\n");

 // Next up, we compress
 WriteCompressedLayer2(cData,c.fxsize*c.fysize*c.totalframes,bufsize,rData);
 
 // Time to write the compressed stuff
 fwrite(&bufsize,1,4,chr); // size of the compressed block
 fwrite(cData,1,bufsize,chr); // the data itself

 free(cData); // cleanup
 free(rData);
}

void writechr(char *fname)
{
 int ver=4;
 chr=fopen(fname,"wb");
 fwrite(&ver,1,1,chr);
 fwrite(&c.fxsize,1,2,chr);
 fwrite(&c.fysize,1,2,chr);
 fwrite(&c.hx,1,2,chr);
 fwrite(&c.hy,1,2,chr);
 fwrite(&c.hw,1,2,chr);
 fwrite(&c.hh,1,2,chr);
 fwrite(&c.lidle,1,2,chr);
 fwrite(&c.ridle,1,2,chr);
 fwrite(&c.uidle,1,2,chr);
 fwrite(&c.didle,1,2,chr);
 fwrite(&c.totalframes,1,2,chr);

 int i;
 
 i=strlen(c.lanim);
 fwrite(&i,1,4,chr); fwrite(&c.lanim,1,i,chr);
 i=strlen(c.ranim);
 fwrite(&i,1,4,chr); fwrite(&c.ranim,1,i,chr);                                          
 i=strlen(c.uanim);
 fwrite(&i,1,4,chr); fwrite(&c.uanim,1,i,chr);
 i=strlen(c.danim);
 fwrite(&i,1,4,chr); fwrite(&c.danim,1,i,chr);

 loadbmp(bmpfile);
 makechr();
 free(buf);
}

main(int argcount, char *arg[])
{
 cprintf("CHRMAK by the Speed Bump\r\nBased upon CHRMAK by vecna\r\n\r\n");
 cprintf(" Syntax: CHRMAK char.mak\r\n");
  
 if (argcount!=2)
  {
   cprintf(" see the enclosed sample .mak file for more details (the mak file format has changed).\r\n");
   exit(0);
  }    
 
 strcpy(makefile,arg[1]);

 cprintf("Parsing makefile...\r\n");
 parsemakefile(makefile);

 cprintf("Reading image data...\r\n");
 writechr(chrfile);

 cprintf("Complete!");
 return 0;
}
