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

#ifndef VC_H
#define VC_H

extern int v2_touchy;  // exit to system at first sign of trouble?

extern int mapevents;  // number of map events in this VC

extern int hookretrace;
extern int hooktimer;

extern int stralloc;

extern void RunSystemAutoexec();

typedef struct {
    char fname[40];
    char argtype[20];
    int numargs, numlocals;
    int returntype;
    int syscodeofs;
} funcdecl;

extern funcdecl *funcs;
extern int numfuncs;

/*
extern int mapevents;
extern int hookretrace, hooktimer;
*/

typedef struct {
    char vname[40];
    int vsofs;
    int arraylen;
} strdecl;

extern strdecl *str;
extern int numstr;

typedef struct {
    char vname[40];
    int varstartofs;
    int arraylen;
} vardecl;

extern vardecl *vars;
extern int numvars;

extern int invc;
extern char *mapvc;
extern char kill;

extern quad *vcsp;
extern quad *vcstack;

extern void LoadSystemVC();
extern void LoadMapVC(VFILE *f);
extern void ReadVCVar();
extern void WriteVCVar();
extern void ExecuteEvent(int);
extern void ExecuteUserFunc(int);

extern void CheckHookTimer();
extern void HookRetrace();
extern void HookTimer();
extern void HookKey(int script);

#endif  // VC_H