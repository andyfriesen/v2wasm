// main.c

#include <stdio.h>

#define word unsigned short int
#define byte unsigned char

word *vsp;
word numtiles;

void err(char *s)
{
 free(vsp);
 cprintf(s);
 exit(0);
}

main(int paramcount,char *param[])
{
 if (paramcount!=3)
  {
   cprintf("VSPConv V1.0 by the Speed Bump (aka Andy Friesen)\r\n");
   cprintf("\r\nSyntax:\r\n\r\nVSPConv.exe 8bit.vsp 16bit.v16\r\n");
   exit(0);
  }
 loadvsp(param[1]);
 savevsp(param[2]);
 free(vsp);
}
