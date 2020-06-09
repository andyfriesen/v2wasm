/*
Copyright (C) 1998 BJ Eirich (aka vecna)
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
// Wee. A generic #include. I feel so warm and fuzzy inside. :)

// #include "mss.h"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <aen, may 5>
// + added VERSION tag and made all text occurences of version use it
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef VERGE_INC
#define VERGE_INC

#define VERSION "2.6rev2.1"

/*#ifdef _WIN32
//  #include <windows.h>
  #define WIN32_LEAN_AND_MEAN
  #define delay Sleep
#endif*/

#ifndef _WIN32
#include <dos.h>
#endif

/* // woohoo, don't need 'em, don't want 'em. ;)
extern "C" {
//	#include <stdio.h>
//	#include <stdlib.h>
//	#include <string.h>
}
*/

/*#define byte unsigned char // predefined somewhere in windows' stuff
#define word unsigned short int*/
#define quad unsigned int

#define TRUE 1
#define FALSE 0

extern void Sys_Error(const char *message, ...);

#include "linked.h"

#include "a_memory.h"

#include "memstr.h"  // TODO: remove this?  I don't like it, and only the fonts use it right now.
#include "strk.h"
#include "vector.h"

#include "console.h"
#include "engine.h"
#include "entity.h"
//#include "fli.h"
#include "font.h"
#include "image.h"
#include "keyboard.h"
#include "main.h"
#include "message.h"
#include "mouse.h"

#include "render.h"
#include "sound.h"
#include "timer.h"
#include "vc.h"
#include "vfile.h"
#include "w_graph.h"
#include "w_input.h"

extern GrDriver gfx;
extern Input input;

extern HWND hWnd;
extern bool bActive;

extern int hicolor;

extern char *strbuf;

extern void V_memset(void *dest, int fill, int count);
extern void V_memcpy(void *dest, const void *src, int count);
extern int V_memcmp(const void *m1, const void *m2, int count);
extern void V_strcpy(char *dest, const char *src);
extern void V_strncpy(char *dest, const char *src, int count);
extern int V_strlen(const char *str);
extern void V_strcat(char *dest, char *src);
extern int V_strcmp(const char *s1, const char *s2);
extern int V_atoi(const char *str);
extern float V_atof(const char *str);

#endif  // VERGE_INC
