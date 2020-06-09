// render.c
// basically all the tile drawing routines go here

#include <stdio.h>
#include "main.h"
#include "graph.h"

int xwin,ywin; // window coords

typedef struct
{
  unsigned short start;              // strand start
  unsigned short finish;             // strand end
  unsigned short delay;              // tile-switch delay
  unsigned short mode;               // tile-animation mode
} vspanim_r;

vspanim_r vspanim[100];

void loadvsp(char *fname)
{
 FILE *f;
 byte *p;
 word i,j,k;
 short ver;
 char pal[768];

 if (!(f=fopen(fname,"rb"))) err("VSP file not found.");
 fread(&ver, 1, 2, f);

 if (ver==2)
  {
   fread(&pal,1,768,f);
   fread(&numtiles,1,2,f);

   p=(byte *)malloc(256*numtiles);
   vsp=(word *)malloc(512*numtiles);

   if (!vsp) err("Unable to allocate VSP memory");
   fread(p,256,numtiles,f);
   for (i=0; i<numtiles*256; i++)
    {
     j=p[i];
     k =(pal[j*3+0]>>1)<<10; // red
     k+=(pal[j*3+1]>>1)<<5;  // green
     k+=(pal[j*3+2]>>1);     // blue
     vsp[i]=k;
    }
   free(p);
   fread(&vspanim,8,100,f);
   fclose(f);
  }
 else if (ver==4)
  err("This vsp is already in version 4 (highcolor non-compressed");
 else
   err("Unable to decompress VSPs at this time.");
}

void savevsp(char *fname)
{
 FILE *f;
 short i;
 word j;

 if (!(f=fopen(fname,"wb"))) err("Unable to create VSP");
 i=4; // highcolor, non-compressed
 fwrite(&i,1,2,f);
 fwrite(&numtiles,1,2,f);
 fwrite(vsp,512,numtiles,f);
 fwrite(&vspanim,8,100,f);
 fclose(f);
}


