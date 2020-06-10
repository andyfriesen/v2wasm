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
// ³                         Messaging module                            ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

#include "verge.h"

// DATA
// /////////////////////////////////////////////////////////////////////////////////////////////////

struct message_t : public linked_node {
    int compare(void* c) { return ((message_t*)c)->expire_at > expire_at; }

    message_t(string_k s, int expire) : text(s), expire_at(expire) {}

    string_k text;
    int expire_at;
};
static linked_list messages;

// -- cpu usage --

static int cputimer = 0, frames = 0;
static char runprf[6];

// -- final numbers --

static int fps = 0;
static char profile[6];

// CODE
// /////////////////////////////////////////////////////////////////////////////////////////////////

static void Message_CheckExpirations() {
    if (!messages.number_nodes())
        return;

    messages.go_head();
    do {
        message_t* m = (message_t*)messages.current();
        if (systemtime > m->expire_at) {
            messages.go_next();

            messages.unlink((linked_node*)m);
            delete m;

            continue;
        }
        messages.go_next();

    } while (messages.current() != messages.head());
}

void RenderGUI() {
    int x, y;

    Message_CheckExpirations();

    if (messages.number_nodes()) {
        x = y = 1;
        messages.go_head();
        do {
            message_t* m = (message_t*)messages.current();

            Font_GotoXY(x, y);
            Font_Print(0, m->text.c_str());
            y += Font_GetLength(0);

            messages.go_next();

        } while (messages.current() != messages.head());
    }

    if (!cpu_watch)
        return;
    frames++;

    x = gfx.scrx - Font_GetWidth(0) * 10 - 4;
    y = gfx.scry - Font_GetLength(0) * 4 - 4;

    Font_GotoXY(x, y);
    Font_PrintImbed(0, va("   FPS:%d\n", fps));
    Font_PrintImbed(0, va("Render:%d\n", profile[1]));
    Font_PrintImbed(0, va(" PFlip:%d\n", profile[2]));
    Font_PrintImbed(0, va("   etc:%d\n", profile[0]));
}

void CPUTick() {
    cputimer++;
    runprf[cpubyte]++;
    if (cputimer == 100) {
        fps = frames;
        frames = 0;
        cputimer = 0;

        profile[0] = runprf[0];
        runprf[0] = 0;
        profile[1] = runprf[1];
        runprf[1] = 0;
        profile[2] = runprf[2];
        runprf[2] = 0;
    }
}

void Message_Send(string_k text, int duration) {
    Log(va("Message: %s", text.c_str()));

    messages.insert_tail(
        (linked_node*)new message_t(text, systemtime + duration));

    // did we go over limit? if so, remove head
    if (messages.number_nodes() > 5) {
        message_t* m = (message_t*)messages.head();
        messages.unlink((linked_node*)m);
        delete m;
    }
}
