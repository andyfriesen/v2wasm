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

// ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
// ³                          The VERGE Engine                           ³
// ³              Copyright (C)1998 BJ Eirich (aka vecna)                ³
// ³                    Timer / PIC contoller module                     ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

// ChangeLog
// <tSB>  Nov 4, '00 - rewritten to work in Win32.  Borrows heavily from vecna's
// winv1.

#include "verge.h"

#include <mmsystem.h>

// ================================= Data ====================================

unsigned int systemtime = 0, timer_count = 0, hktimer = 0, vctimer = 0;
Timer timer;

// ================================= Code ====================================

Timer::Timer() { curtimer = 0; }

int Timer::Init(int Hz, LPTIMECALLBACK TimeProc) {
    if (curtimer) timeKillEvent(curtimer);
    curtimer =
        timeSetEvent(1000 / Hz, 0, (LPTIMECALLBACK)TimeProc, 0, TIME_PERIODIC);

    if (curtimer == 0) return 0;

    return 1;
}

void Timer::ShutDown() {
    if (curtimer) timeKillEvent(curtimer);
    curtimer = 0;
}

Timer::~Timer() { ShutDown(); }

void CALLBACK TimeProc(
    UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2) {
    if (!bActive) return;  // bleh
    systemtime++;
    timer_count++;
    hktimer++;
    vctimer++;

    if (cpu_watch) CPUTick();
    CheckTileAnimation();
    //    CheckHookTimer(); // is this the best spot for this?  I dunno.
    //    Certainly better than in w_graph.cpp (where it hampers class
    //    containment)
    // if (callback)	callback();
    //	MD_Update();

    //	outp(0x20,0x20);
}

int InitTimer() {
    systemtime = timer_count = hktimer = vctimer = 0;
    if (timer.Init(100, (LPTIMECALLBACK)TimeProc))
        return 1;
    else
        return 0;
}

void ShutdownTimer() { timer.ShutDown(); }
