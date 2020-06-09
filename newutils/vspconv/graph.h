#define word unsigned short int
#define byte unsigned char
extern word *scrw;         // for 16 bit modes
extern word modenum;
extern word xres,yres;
extern byte bshift,gshift,rshift;
extern byte colordepth;
extern byte getpixelr,getpixelg,getpixelb;

extern int initgraph(word x,word y);
extern void closegraph();
extern void putpixel(int x,int y,int c);
extern int showpage(void);
extern void clearscreen();
