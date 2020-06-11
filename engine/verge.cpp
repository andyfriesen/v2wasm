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


   verge.c
   Copyright (C) 1998 Benjamin Eirich

 Portability:
   mingw32 - Yup.
   MSVC6   - Durn straight.
   MSVC5   - Good luck!  HAHAHA!
   Borland - beats me
   QBasic  - HAHAHAHAHA!
*/

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <tSB, Nov 7>
// + moved startup-ish stuff over to startup.cpp (might have been an icky thing
// to do, lots of externs! @_@)
// <tSB, Oct 30>
// + started porting to Win32
// <aen, apr 21>
// + changed Log() & Logp() to take variable args.
// + altered translucency lookup code a bit
// + cleaned up USER.CFG parsing
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include <stdarg.h> // va_*

#include "verge.h"
#include "wasm.h"

// gah!
extern void ParseAutoCFG();

// ================================= Win32 stuff =============================

GrDriver gfx; // DirectDraw
Input input;  // DirectInput

// ================================= Data ====================================

// declared in VDRIVER.C
char logoutput = 1; // Verbose debugging startup mode

// ================================= Code ====================================

void InitLog() {
    if (logoutput) {
        remove("verge.log");
    }
}

static FILE* Log_OpenLog() {
    FILE* f;

    f = fopen("VERGE.LOG", "aw");
    if (!f)
        Sys_Error("Log_OpenLog: unable to open VERGE.LOG");

    return f;
}

void Log(const char* message) {
    FILE* f;

    if (!logoutput)
        return;

    f = Log_OpenLog();

    if (!f)
        Sys_Error("Error logging!");

    fprintf(f, "%s\n", message);
    fflush(f);

    fclose(f);
}

// used in conjunction with LogDone()
void Logp(const char* message) {
    FILE* f;

    if (!logoutput)
        return;

    f = Log_OpenLog();

    if (!f)
        Sys_Error("Error logging!");

    fprintf(f, "%s", message);
    fflush(f);

    fclose(f);
}

void LogDone() {
    FILE* f;

    if (!logoutput)
        return;

    f = Log_OpenLog();

    fprintf(f, "... OK\n");
    fflush(f);

    fclose(f);
}

// InitSystems moved to startup.cpp, where it can have access to Win32

void LoadTransTable() {
    VFILE* vf;
    byte* translucency_table;

    if (gfx.bpp != 1)
        return; // why bother?

    /*    if (translucency_table)
          Sys_Error("Foul things are afoot."); */

    vf = vopen("TRANS.TBL");
    if (!vf && gfx.bpp == 1) {
        Sys_Error("trans.tbl not found");
    }

    translucency_table =
        (byte*)valloc(256 * 256, "translucency_table", OID_TEMP);

    vread(translucency_table, 256 * 256, vf);
    vclose(vf);
    gfx.InitLucentLUT(translucency_table); // weird, I know. ;)
    vfree(translucency_table);
}

void vmainloop() {
    CheckHookTimer();
    while (timer_count > 0) {
        timer_count--;
        //   Log(va("GameTick() %i",timer_count));
        GameTick();
    }

    if (kill) {
        FreeVSP();
        FreeMAP();
        FreeCHRList();
        vcsp = vcstack;
        kill = 0;
        LoadMAP(startmap.c_str());
    }
    Render();
    gfx.ShowPage();
}

int VMain() {
    // bleh, message pump

    // note, the argument vector doesn't exist in WinMain apps.
    // TODO: Write a parser?
    /*	if (2 == argc)
            {
                    startmap = argv[1];
            }*/

    //	memcpy(game_palette, gfx.pal, 3*256);
    Console_Init();

    Console_Printf(va("VERGE System Version %s", VERSION));
    Console_Printf("Copyright (C)1998 vecna");
    Console_Printf("");

    ParseAutoCFG();

    Logp("Loading 8 bit translucency table");
    LoadTransTable();
    LogDone();

    Logp("Loading system VC");
    LoadSystemVC();
    LogDone();
    // startmap override?
    if (startmap.length())
        LoadMAP(startmap.c_str());
    else
        RunSystemAutoexec();

    // if there is no starting map at this point, we're done.
    if (startmap.length() < 1)
        Sys_Error("");

    // don't forget to set input destination
    //	key_dest = key_game;
    // ---

    while (true) {
        vmainloop();
        wasm_nextFrame();
    }

    return 0;
}
