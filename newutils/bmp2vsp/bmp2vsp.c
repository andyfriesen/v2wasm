
#include <stdio.h>
#include "graph.h"

word *buf;
int width,height;

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
     r>>=3; g>>=3; b>>=3;
     buf[y*width+x]=(r<<10)+(g<<5)+b;
    }
   fread(temp,1,width%4,f);
  }
 fclose(f);
}

word getdot(int x,int y)
{
 return buf[y*width+x];
}

main(int argcount, char *arg[])
{
 FILE *f;
 int curtile,tx,ty,x,y;
 word c,r,g,b;
 word ver,maxtiles;

 if (argcount!=4)
  {
   cprintf("BMP2VSP v0.000  by the Speed Bump (aka Andy Friesen)\r\n");
   cprintf("Based upon PCX2VSP by vecna\r\n\r\n");
   cprintf("  Syntax: BMP2VSP source.bmp dest.vsp numtiles\r\n\r\n");
   cprintf("Also, make sure that there are 18 tiles per row.\r\n");
   exit(0);
  }

 loadbmp(arg[1]);

 f=fopen(arg[2],"wb");
 if (!f)
  {cprintf("Unable to write to vsp %s",arg[2]);
   exit(255);                                     }


 ver=4;
 maxtiles=atoi(arg[3]);
 fwrite(&ver,1,2,f);
 fwrite(&maxtiles,1,2,f);
 curtile=1;

 tx=0; ty=0;

 while (curtile<=maxtiles)
  {
   // 18 tiles wide
   for (y=0; y<16; y++)
    for (x=0; x<16; x++)
     {
      c=getdot(tx*17+x+1,ty*17+y+1);
      fwrite(&c,1,2,f);
     }
   curtile++;
   tx++;
   if (tx==18)
    {
     tx=0;
     ty++;
    }
  }

 free(buf);
 fclose(f);
}
