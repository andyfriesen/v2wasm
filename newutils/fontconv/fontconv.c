#include <stdio.h>

#define byte unsigned char
#define word unsigned short int

struct fontstruct
{
  byte width, height;                  // Font character dimensions
  word *data;                          // Font bitmap data
};

char pal[768];

int subsets;
struct fontstruct font;

void err(char *s)
{
 cprintf(s);
 exit(-1);
}

void LoadFont(char *fname)
{
 FILE *f;
 char *buf;
 int ver;
 int z,c;
 long int i;
 word d;
 int x,y;

 if (!(f=fopen(fname,"rb"))) err("Unable to load FNT");
 ver=getc(f);
 if (ver!=1) err("This will convert version 1 (8 bit color) fonts to version 2 (hicolor).\r\nThis font is already converted.");

 fread(&font.width,1,2,f);
 fread(&font.height,1,2,f);
 fread(&subsets,1,2,f);

 z=subsets*font.width*font.height*96;

 buf         = (char *)malloc(z);
 font.data   = (word *)malloc(z*2);

 fread(buf,1,z,f);

 for (i=0; i<z; i++)
  {
   c=buf[i];
   d =((pal[c*3+0])>>1)<<10;    // red
   d+=((pal[c*3+1])>>1)<<5;     // green
   d+=((pal[c*3+2])>>1);        // blue
   font.data[i]=d;
  }

 free(buf);
 fclose(f);
}

void SaveFont(char *fname)
{
 FILE *f;
 int ver;

 if (!(f=fopen(fname,"wb"))) err("Unable to save FNT");

 ver=2;
 fwrite(&ver,1,1,f);
 fwrite(&font.width,1,2,f);
 fwrite(&font.height,1,2,f);
 fwrite(&subsets,1,2,f);
 fwrite(font.data,1,subsets*font.width*font.height*192,f);

 fclose(f);
}

void LoadPal(char *fname)
{
 FILE *f;

 if (!(f=fopen(fname,"rb"))) err("Unable to load palette file");
 fread(&pal,768,1,f);
 fclose(f);
}

int main(int argct, char *arg[])
{
 if (argct!=4)
  err("Syntax: FONTCONV source.fnt dest.fnt palette.pal");

 LoadPal(arg[3]);
 LoadFont(arg[1]);
 SaveFont(arg[2]);
}
