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

// ���������������������������������������������������������������������Ŀ
// �                          The VERGE Engine                           �
// �              Copyright (C)1998 BJ Eirich (aka vecna)                �
// �                  VergeC Interpreter  Core module                    �
// �����������������������������������������������������������������������

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// NOTES:
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <tSB, Nov 10-ish>
// + lots of tweaking for the port to Win32.
// <vecna, aug 2>
// + changed vc var TIMER to address vctimer, not timer_count
//   timer_count is zeroed after each call to ExecuteEvent, and not zeroed
//   for hooked events.
// <aen, may 14>
// + added PaletteMorph() rgb truncation (<0=0, >63=63)
// <zero 5.8.99>
// + Mistake in PaletteMorph()? was not setting global pal2[], only a local
//   copy
// <aen, may 7>
// + added division-by-zero protection for Random()
// <aen, may 5>
// + altered vec's vc_Silhouette() to use new silhouette vdriver routines
//   instead of allocating mem, generating mask, calling sprite routines,
//   then freeing mem.
// + added vc_SilhouetteScale, vc_Tint, & vc_TintScale (unimplemented)
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

//#define VC_H
#include <math.h>

#include "verge.h"

#include "mouse.h"
#include "sincos.h"
#include "vccode.h"
#include "wasm.h"

#define USERFUNC_MARKER 10000

// prototypes
void CheckHookTimer();
void HookTimer();

// ================================= Data ====================================

int v2_touchy = 0;

char* sysvc = 0;
char* mapvc = 0;
char* basevc = 0; // VC pool ptrs
char* code = 0;   // VC current instruction pointer (IP)

int* globalint = 0; // system.vc global int variables
int maxint = 0;     // maximum allocated # of ints

string_k* vc_strings = 0; // vc string workspace
int stralloc = 0;

int vcreturn = 0;       // return value of last int function
string_k vcretstr = ""; // return value of last string function
int returning_type = 0; // hack to discern int from string returns

char* movescriptbuf = 0; // VC EntityMove buffer
char vctrack = 0;        // VC call tracking to verge.log

quad* vcstack = 0; // VC stack (seperate from main stack)
quad* vcsp = 0;    // VC stack pointer [esp]

int mapevents = 0;      // number of map events in this VC
int* event_offsets = 0; // map VC offset table
// char*	mapvctbl[1024]	={0};	// map VC offset table
// event offset marker

int hookretrace = 0;
int hooktimer = 0;
int invc = 0;

char kill = 0;

// FUNC/STR/VAR ARRAYS

funcdecl* funcs = 0;
int numfuncs = 0;

strdecl* str = 0;
int numstr = 0;

vardecl* vars = 0;
int numvars = 0;

// LOCAL FUNC VARS

// *****
#define NEW_LOCALS
// *****

#ifdef NEW_LOCALS // *****

//#define DEBUG_LOCALS

static int int_stack[1024 + 20];
static int int_stack_base = 0, int_stack_ptr = 0;

static string_k str_stack[1024 + 20];
static int str_stack_base = 0, str_stack_ptr = 0;

static int int_base[1024 + 20];
static int str_base[1024 + 20];
static int base_ptr = 0;

static int int_last_base = 0;
static int str_last_base = 0;

// Handle-based file access stuff --tSB
const int MAXVCFILES =
    10; // maximum number of files that can be open at once. (through VC)

typedef struct {
    VFILE* readp; // vfile, for reading
    FILE* writep; // conventional file, for writing
    int mode;     // 0 - closed, 1 - read, 2 - write
} VC_file;

VC_file vcfiles[MAXVCFILES];

static void PushBase(int ip, int sp) {
    if (base_ptr < 0 || base_ptr >= 1024)
        Sys_Error("PushBase: DEI!");

    int_base[base_ptr] = ip;
    str_base[base_ptr] = sp;

    base_ptr++;
}

static void PopBase() {
    if (base_ptr <= 0 || base_ptr > 1024)
        Sys_Error("PushBase: DEI!");

    base_ptr--;

    int_stack_base = int_base[base_ptr];
    str_stack_base = str_base[base_ptr];
}

static void PushInt(int n) {
    if (int_stack_ptr < 0 || int_stack_ptr >= 1024)
        Sys_Error("PushInt: DEI!");

    int_stack[int_stack_ptr] = n;

#ifdef DEBUG_LOCALS
    Log(va("base=%-04d, push# %d", int_stack_base, int_stack[int_stack_ptr]));
#endif
    int_stack_ptr++;
}
static int PopInt() {
    if (int_stack_ptr <= 0 || int_stack_ptr > 1024)
        Sys_Error("PopInt: DEI!");

    --int_stack_ptr;
#ifdef DEBUG_LOCALS
    Log(va("base=%-04d, pop#  %d", int_stack_base, int_stack[int_stack_ptr]));
#endif

    return int_stack[int_stack_ptr];
}

static void PushStr(string_k s) {
    if (str_stack_ptr < 0 || str_stack_ptr >= 1024)
        Sys_Error("PushStr: DEI!");

    str_stack[str_stack_ptr] = s;

#ifdef DEBUG_LOCALS
    Log(va("base=%-04d, push$ %-60s", str_stack_base,
        str_stack[str_stack_ptr].c_str()));
#endif
    str_stack_ptr++;
}
static string_k PopStr() {
    if (str_stack_ptr <= 0 || str_stack_ptr > 1024)
        Sys_Error("PopStr: DEI!");

    --str_stack_ptr;
#ifdef DEBUG_LOCALS
    Log(va("base=%-04d, pop$  %-60s", str_stack_base,
        str_stack[str_stack_ptr].c_str()));
#endif

    return str_stack[str_stack_ptr];
}

#else // OLD LOCALS

#define MAX_ARGS 20
#define MAX_LOCAL_STRINGS 10

struct lvars {
    int nargs[MAX_ARGS];
    string_k s[MAX_LOCAL_STRINGS];
};

static lvars lvar;

#endif // !def NEW_LOCALS

// PROTOTYPES
// /////////////////////////////////////////////////////////////////////////////////////

string_k ResolveString();
void ExecuteSection();
void ExecuteEvent(int i);
void ExecuteUserFunc(int i);

int ProcessOperand();   // Mutually dependant functions suck.
int ProcessIfOperand(); // Hell yeah they do, bitch.
void HandleExternFunc();
void HandleStdLib();
void ExecuteBlock();

// CODE
// ///////////////////////////////////////////////////////////////////////////////////////////

static int sys_codesize = 0;
static char* absolute_sys = 0;

static char xvc_sig[8] = "VERGE2X";

void LoadSystemIdxAndVcs() {
    VFILE* f = 0;
    int i = 0;

    Log("Initializing VC interpreter");
    f = vopen("system.idx");
    if (!f) {
        Sys_Error("Could not open system.idx.");
    }
    vread(&numvars, 4, f);
    vars = (vardecl*)valloc(
        numvars * sizeof(vardecl), "LoadSystemVC:vars", OID_VC);
    vread(vars, numvars * 48, f);
    vread(&numfuncs, 4, f);
    funcs = (funcdecl*)valloc(
        numfuncs * sizeof(funcdecl), "LoadSystemVC:funcs", OID_VC);
    vread(funcs, numfuncs * 76, f);
    vread(&numstr, 4, f);
    str =
        (strdecl*)valloc(numstr * sizeof(strdecl), "LoadSystemVC:str", OID_VC);
    vread(str, numstr * 44, f);
    vclose(f);

    f = vopen("system.vcs");
    if (!f) {
        Sys_Error("Could not open system.vcs");
    }

    i = filesize(f);
    sysvc = (char*)valloc(i, "LoadSystemVC:sysvc", OID_VC);
    vread(&numfuncs, 4, f);
    vread(&maxint, 4, f);
    vread(&stralloc, 4, f);

    globalint = (int*)valloc(maxint ? maxint * 4 : 4, "globalint", OID_VC);
    if (stralloc) {
        vc_strings = new string_k[stralloc];
    }
    vread(sysvc, i, f);
    vclose(f);
}

void LoadSystemIndex() {
    char buf[8];
    VFILE* f;
    int n;

    buf[0] = '\0';
    f = 0;

    // open system script variable/function/string offset table file
    f = vopen("system.xvc");
    if (!f) {
        // Sys_Error("Could not open system.idx.");
        numvars = 0;
        numfuncs = 0;
        numstr = 0;
        return;
    }

    vread(buf, 8, f);
    if (strncmp(buf, xvc_sig, 8)) {
        Sys_Error("LoadSystemIndex: system.xvc contains invalid signature");
    }

    // skip code offset
    vread(&n, 4, f);

    // read # variables
    vread(&numvars, 4, f);
    if (numvars) {
        vars = (vardecl*)valloc(
            numvars * sizeof(vardecl), "LoadSystemVC$vars", OID_VC);
        vread(vars, numvars * 48, f);
    }

    // read # functions
    vread(&numfuncs, 4, f);
    if (numfuncs) {
        funcs = (funcdecl*)valloc(
            numfuncs * sizeof(funcdecl), "LoadSystemVC$funcs", OID_VC);
        vread(funcs, numfuncs * 76, f);
    }

    // read # strings
    vread(&numstr, 4, f);
    if (numstr < 1)
        numstr = 1;
    str =
        (strdecl*)valloc(numstr * sizeof(strdecl), "LoadSystemVC$str", OID_VC);
    vread(str, numstr * 48, f);

    // done w/ this file
    vclose(f);
}

void LoadSystemCode() {
    char buf[8];
    VFILE* f;
    int code_offset;

    buf[0] = '\0';
    f = 0;
    code_offset = 0;

    // open system script code file
    f = vopen("system.xvc");
   if (!f) {
        // Sys_Error("Could not open system.vcs");
        sys_codesize = 0;
        sysvc = 0;
        absolute_sys = sysvc;
        numfuncs = 0;
        maxint = 0;
        stralloc = 0;
        return;
    }

    vread(buf, 8, f);
    if (strncmp(buf, xvc_sig, 8)) {
        Sys_Error("LoadSystemCode: system.xvc contains invalid signature");
    }

    vread(&code_offset, 4, f);
    vseek(f, 0, SEEK_END);
    sys_codesize = vtell(f) - code_offset + 1;    
    // see if there's actually code present
    if (sys_codesize < 1) {
        vclose(f);

        sys_codesize = 0;
        sysvc = 0;
        absolute_sys = sysvc;
        numfuncs = 0;
        maxint = 0;
        stralloc = 0;

        return;
    }
    // seek to code position
    vseek(f, code_offset, SEEK_SET);

    // grab system script code size and allocate a buffer for it
    sysvc = (char*)valloc(sys_codesize, "LoadSystemCode$sysvc", OID_VC);
    absolute_sys = sysvc;

    // how many funcs, global ints, and global strings?
    vread(&numfuncs, 4, f);
    vread(&maxint, 4, f);
    vread(&stralloc, 4, f);

    // allocate global integer and string arrays
    if (maxint) {
        globalint = (int*)valloc(4 * maxint, "globalint", OID_VC);
    }
    if (stralloc) {
        vc_strings = new string_k[stralloc]; //(string *)valloc(sizeof(string) *
                                             // stralloc, "vc_strings", OID_VC);
    }

    // read in system script code
    vread(sysvc, sys_codesize, f);

    vclose(f);
}

void RunSystemAutoexec() {
    int n;

    for (n = 0; n < numfuncs; n++) {
        char* x = funcs[n].fname;
        V_strlwr(x);
        if (!V_strcmp(x, "autoexec"))
            break;
    }
    if (n < numfuncs)
        ExecuteUserFunc(n);
    else
        Sys_Error("No AutoExec() found in system scripts.");
}

void LoadSystemVC() {
    Log("Initializing VC interpreter");

    if (Exist("system.xvc")) {
        LoadSystemIndex();
        LoadSystemCode();
    } else {
        LoadSystemIdxAndVcs();
    }

    // initialize VC stack
    vcstack = (quad*)valloc(6000, "vcstack", OID_VC);
    vcsp = vcstack;

    movescriptbuf = (char*)valloc(256 * 256, "movescriptbuf", OID_VC);

    Log(va("system vclib init: %d funcs, %d ints (%d bytes), %d strings",
        numfuncs, numvars, maxint * 4, numstr, stralloc));

    // Set up VC files --tSB
    memset(vcfiles, 0, sizeof vcfiles);
    // RunSystemAutoexec();
}

static int map_codesize = 0;
static char* absolute_map = 0;
void LoadMapVC(VFILE* f) {
    // int codesize=0;

    vread(&mapevents, 4, f);
    if (event_offsets)
        delete[] event_offsets;
    event_offsets = new int[mapevents];
    if (!event_offsets) {
        Sys_Error("LoadMapVC: memory exhausted on event_offsets");
    }
    vread(event_offsets, 4 * mapevents, f);

    vread(&map_codesize, 4, f);
    if (map_codesize < 1)
        map_codesize = 1;
    mapvc = (char*)valloc(map_codesize, "mapvc", OID_VC);
    absolute_map = mapvc;
    vread(mapvc, map_codesize, f);
}

byte GrabC() {
    return *code++;
}

word GrabW() {
    word result;
    memcpy(&result, code, 2);
    code += 2;

    return result;
}

quad GrabD() {
    quad result;
    memcpy(&result, code, 4);
    code += 4;

    return result;
}

string_k GrabString() {
    string_k ret;
    int c;
    char temp[32 + 1]; // soften the blow

    ret = "";
    c = 0;
    while (*code) {
        temp[c++] = GrabC(); //*code++;
        if (c >= 32) {
            c = temp[c] = '\0';
            ret += temp;
        }
    }
    if (c) {
        temp[c] = '\0';
        ret += temp;
    }
    code++;

    return ret;
}

int ReadInt(char category, int loc, int ofs) {
    switch (category) {
    case op_UVAR:
        if (loc < 0 || loc >= maxint)
            Sys_Error("ReadInt: bad offset to globalint (var)");
        return globalint[loc];
    case op_UVARRAY:
        if (loc < 0 || loc >= maxint)
            Sys_Error("ReadInt: bad offset to globalint (arr)");
        return globalint[loc];
    case op_HVAR0:
        switch (loc) {
        case 0:
            return xwin;
        case 1:
            return ywin;
        case 2:
            return cameratracking;
        case 3:
            return vctimer;
        case 4:
            return input.up;
        case 5:
            return input.down;
        case 6:
            return input.left;
        case 7:
            return input.right;
        case 8:
            return input.b1;
        case 9:
            return input.b2;
        case 10:
            return input.b3;
        case 11:
            return input.b4;
        case 12:
            return gfx.scrx;
        case 13:
            return gfx.scry;
        case 14:
            return playernum;
        case 15:
            return cc;
        case 16:
            return tracker;
        case 17:
            return input.mousex; // Mouse_X();
        case 18:
            return input.mousey; // Mouse_Y();
        case 19:
            return input.mouseb; // Mouse_ButtonFlags();
        case 20:
            return vctrack;
        case 21:
            return Image_Width();
        case 22:
            return Image_Length();
        case 23:
            return GetMusicVolume(); // sound.volume;
        case 24:
            return (int)vsp;
        case 25:
            return lastent;
        case 26:
            return input
                .last_pressed; // key_lastpress; //key_last(); //last_pressed;
                               // --tSB wow, this one line has changed a bit.
                               // ^_^
        case 27:
            return layer[0].sizex;
        case 28:
            return layer[0].sizey;
        case 29:
            return 1; // vsync; -- vsynch is always on in DirectX --tSB
        case 30:
            return entities;
        case 31:
            if (gfx.bpp == 2)
                return gfx.trans_mask;
            else
                return 0;
        case 32:
            return gfx.bpp;
        }
    case op_HVAR1:
        switch (loc) {
        case 0:
            if (ofs < 0 || ofs >= gfx.scrx * gfx.scry) {
                if (v2_touchy)
                    Sys_Error(
                        "ReadInt: bad offset to screen[]: %d (base: $%08X, "
                        "%dx%d)",
                        ofs, (int)gfx.screen, gfx.scrx, gfx.scry);
                return 0;
            }
            return gfx.bpp > 1 ? ((unsigned short*)gfx.screen)[ofs]
                               : gfx.screen[ofs];
            return 0;
        case 1:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "ReadInt: bad offset to entity.x[]: %d (%d total)", ofs,
                        entities);
                return 0;
            }
            return entity[ofs].x;
        case 2:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "ReadInt: bad offset to entity.y[]: %d (%d total)", ofs,
                        entities);
                return 0;
            }
            return entity[ofs].y;
        case 3:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "ReadInt: bad offset to entity.tx[]: %d (%d total)",
                        ofs, entities);
                return 0;
            }
            return entity[ofs].tx;
        case 4:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "ReadInt: bad offset to entity.ty[]: %d (%d total)",
                        ofs, entities);
                return 0;
            }
            return entity[ofs].ty;
        case 5:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "ReadInt: bad offset to entity.facing[]: %d (%d total)",
                        ofs, entities);
                return 0;
            }
            return entity[ofs].facing;
        case 6:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "ReadInt: bad offset to entity.moving[]: %d (%d total)",
                        ofs, entities);
                return 0;
            }
            return entity[ofs].moving;
        case 7:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "ReadInt: bad offset to entity.specframe[]: %d (%d "
                        "total)",
                        ofs, entities);
                return 0;
            }
            return entity[ofs].specframe;
        case 8:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "ReadInt: bad offset to entity.speed[]: %d (%d total)",
                        ofs, entities);
                return 0;
            }
            return entity[ofs].speed;
        case 9:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "ReadInt: bad offset to entity.movecode[]: %d (%d "
                        "total)",
                        ofs, entities);
                return 0;
            }
            return entity[ofs].movecode;
        case 10:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error("ReadInt: bad offset to entidx[]: %d (%d total)",
                        ofs, entities);
                return 0;
            }
            return entidx[ofs];
        case 11:
            if (ofs < 0 || ofs >= 128) {
                if (v2_touchy)
                    Sys_Error("ReadInt: bad offset to key[]: %d", ofs);
                return 0;
            }
            return input.key[ofs]; // scantokey[ofs]];
        case 12:
            if (ofs < 0 || ofs >= numlayers) {
                if (v2_touchy)
                    Sys_Error(
                        "ReadInt: bad offset to layer.hline[]: %d (%d total)",
                        ofs, numlayers);
                return 0;
            }
            return layer[ofs].hline;

        case 13:
            return (int)(*(byte*)ofs);
        case 14:
            return (int)(*(word*)ofs);
        case 15:
            return (int)(*(quad*)ofs);

        case 16:
            if (ofs < 0 || ofs >= 3 * 256) {
                if (v2_touchy)
                    Sys_Error("ReadInt: bad offset to pal[]: %d", ofs);
                return 0;
            }
            return (int)gfx.pal[ofs];

        case 17:
            return (int)(*(char*)ofs);
        case 18:
            return (int)(*(short*)ofs);
        case 19:
            return (int)(*(int*)ofs);

        /*
        // Modified by Pyro
        case 20:
                if (ofs<0 || ofs>=entities)
                        return 0;
                        //Sys_Error("ReadInt: bad offset to entity.x");
                return chr[entity[ofs].chrindex].hx;
        case 21:
                if (ofs<0 || ofs>=entities)
                        return 0;
                        //Sys_Error("ReadInt: bad offset to entity.x");
                return chr[entity[ofs].chrindex].hy;
        case 22:
                if (ofs<0 || ofs>=entities)
                        return 0;
                        //	Sys_Error("ReadInt: bad offset to entity.x");
                return chr[entity[ofs].chrindex].hw;
        case 23:
                if (ofs<0 || ofs>=entities)
                        return 0;
                        //Sys_Error("ReadInt: bad offset to entity.x");
                return chr[entity[ofs].chrindex].hh;
        */

        case 20:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "ReadInt: bad offset to entity.isob[]: %d (%d total)",
                        ofs, entities);
                return 0;
            }
            return entity[ofs].obsmode2;

        case 21:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "ReadInt: bad offset to entity.canob[]: %d (%d total)",
                        ofs, entities);
                return 0;
            }
            return entity[ofs].obsmode1;

        case 22:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "ReadInt: bad offset to entity.autoface[]: %d (%d "
                        "total)",
                        ofs, entities);
                return 0;
            }
            return entity[ofs].face;

        case 23:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error("ReadInt: bad offset to entity.visible[]: %d (%d "
                              "total)",
                        ofs, entities);
                return 0;
            }
            return entity[ofs].visible;

        case 24:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "ReadInt: bad offset to entity.on[]: %d (%d total)",
                        ofs, entities);
                return 0;
            }
            return entity[ofs].on;
        case 25: // I hate this hacked pointer crap --tSB
            return (int)chr[entity[ofs].chrindex].imagedata; // chr_data
        case 26:
            return chr[entity[ofs].chrindex].fxsize; // entity.width
        case 27:
            return chr[entity[ofs].chrindex].fysize; // entity.height
        case 28:
            return entity[ofs].chrindex; // entity.chrindex

        case 29:
            return zones[ofs].script; // Zara made me add it!
        case 30:
            return zones[ofs].percent;
        default:
            Sys_Error("ReadInt: funky offset to var1's: %d", loc);
        }

    case op_LVAR:
        if (loc < 0 || loc > 19) {
            Sys_Error("ReadInt: bad offset to local ints: %d", loc);
        }
#ifdef NEW_LOCALS
#ifdef DEBUGLOCALS
        Log(va("op_LVAR: int_stack_base=%d, loc=%d", int_stack_base, loc));
#endif
        return int_stack[int_stack_base + loc];
#else // OLD LOCALS
        return lvar.nargs[loc];
#endif

    default:
        Sys_Error(
            "VC Execution error: Invalid ReadInt category %d", (int)category);
    }

    return 0;
}

void WriteInt(char category, int loc, int ofs, int value) {
    switch (category) {
    case op_UVAR:
        if (loc < 0 || loc >= maxint) {
            if (v2_touchy)
                Sys_Error("WriteInt: bad offset to globalint (var)");
            break;
        }
        globalint[loc] = value;
        break;
    case op_UVARRAY:
        if (loc < 0 || loc >= maxint) {
            if (v2_touchy)
                Sys_Error("WriteInt: bad offset to globalint (arr)");
            break;
        }
        globalint[loc] = value;
        break;
    case op_HVAR0:
        switch (loc) {
        case 0:
            xwin = value;
            return;
        case 1:
            ywin = value;
            return;
        case 2:
            cameratracking = (byte)value;
            return;
        case 3:
            vctimer = value;
            return;
        case 4:
            input.up = value;
            return;
        case 5:
            input.down = value;
            return;
        case 6:
            input.left = value;
            return;
        case 7:
            input.right = value;
            return;
        case 8:
            input.b1 = value;
            return;
        case 9:
            input.b2 = value;
            return;
        case 10:
            input.b3 = value;
            return;
        case 11:
            input.b4 = value;
            return;
        case 16:
            tracker = (byte)value;
            return;
        case 17:
            input.mousex = value;
            return; // Mouse_SetPosition(value, Mouse_Y()); return;
        case 18:
            input.mousey = value;
            return; // Mouse_SetPosition(Mouse_X(), value); return;
        case 19:
            input.mouseb = value;
            return; // Mouse_ButtonSetFlags(value); return;
        case 20:
            vctrack = (char)value;
            return;
        case 23:
            SetMusicVolume(value);
            return; // sound.volume=value; return;
        case 26:
            input.last_pressed = value;
            return; // key_lastpress=value; return;
        case 29:
            return; // vsync=value; return;
        }
    case op_HVAR1:
        switch (loc) {
        case 0:
            if (ofs < 0 || ofs >= gfx.scrx * gfx.scry) {
                if (v2_touchy)
                    Sys_Error(
                        "WriteInt: bad offset to screen[]: %d (dest: $%08X, "
                        "%dx%d)",
                        ofs, (int)gfx.screen, gfx.scrx, gfx.scry);
                return;
            }
            if (gfx.bpp > 1)
                ((word*)gfx.screen)[ofs] = (word)value;
            else
                gfx.screen[ofs] = (byte)value;
            return;
        case 1:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "WriteInt: bad offset to entity.x[]: %d (%d total)",
                        ofs, entities);
                return;
            }
            entity[ofs].x = value;
            return;
        case 2:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "WriteInt: bad offset to entity.y[]: %d (%d total)",
                        ofs, entities);
                return;
            }
            entity[ofs].y = value;
            return;
        case 3:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "WriteInt: bad offset to entity.tx[]: %d (%d total)",
                        ofs, entities);
                return;
            }
            entity[ofs].tx = (word)value;
            return;
        case 4:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "WriteInt: bad offset to entity.ty[]: %d (%d total)",
                        ofs, entities);
                return;
            }
            entity[ofs].ty = (word)value;
            return;
        case 5:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error("WriteInt: bad offset to entity.facing[]: %d (%d "
                              "total)",
                        ofs, entities);
                return;
            }
            EntitySetFace(ofs, value);
            /*
            entity[ofs].facing = (byte) value;
            switch (entity[ofs].facing)
            {
                    case 0:
                            entity[ofs].frame=(byte)chr[entity[ofs].chrindex].didle;
                            break;
                    case 1:
                            entity[ofs].frame=(byte)chr[entity[ofs].chrindex].uidle;
                            break;
                    case 2:
                            entity[ofs].frame=(byte)chr[entity[ofs].chrindex].lidle;
                            break;
                    case 3:
                            entity[ofs].frame=(byte)chr[entity[ofs].chrindex].ridle;
                            break;
            }
            */
            return;
        case 6:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error("WriteInt: bad offset to entity.moving[]: %d (%d "
                              "total)",
                        ofs, entities);
                return;
            }
            entity[ofs].moving = (byte)value;
            return;
        case 7:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "WriteInt: bad offset to entity.specframe[]: %d (%d "
                        "total)",
                        ofs, entities);
                return;
            }
            entity[ofs].specframe = (byte)value;
            return;
        case 8:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "WriteInt: bad offset to entity.speed[]: %d (%d total)",
                        ofs, entities);
                return;
            }
            entity[ofs].speed = (byte)value;
            return;
        case 9:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "WriteInt: bad offset to entity.movecode[]: %d (%d "
                        "total)",
                        ofs, entities);
                return;
            }
            entity[ofs].movecode = (byte)value;
            return;
        // case 10:
        case 11:
            if (ofs < 0 || ofs >= 128) {
                if (v2_touchy)
                    Sys_Error("WriteInt: bad offset to key[]: %d (%d total)",
                        ofs, entities);
                return;
            }
            input.key[ofs] = value;
            return;
        case 12:
            if (ofs < 0 || ofs >= numlayers) {
                if (v2_touchy)
                    Sys_Error(
                        "WriteInt: bad offset to layer.hline[]: %d (%d total)",
                        ofs, numlayers);
                return;
            }
            layer[ofs].hline = (unsigned char)value;
            return;

        case 13:
            (*(byte*)ofs) = (byte)value;
            return;
        case 14:
            (*(word*)ofs) = (word)value;
            return;
        case 15:
            (*(quad*)ofs) = (quad)value;
            return;

        case 16:
            if (ofs < 0 || ofs >= 3 * 256) {
                if (v2_touchy)
                    Sys_Error("WriteInt: bad offset to gfx.pal[]: %d", ofs);
                return;
            }
            gfx.pal[ofs] = (byte)value;
            return;

        case 17:
            (*(char*)ofs) = (byte)value;
            return;
        case 18:
            (*(short*)ofs) = (word)value;
            return;
        case 19:
            (*(int*)ofs) = (quad)value;
            return;

        case 20:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "WriteInt: bad offset to entity.isob[]: %d (%d total)",
                        ofs, entities);
                return;
            }
            entity[ofs].obsmode2 = (byte)value;
            return;

        case 21:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "WriteInt: bad offset to entity.canob[]: %d (%d total)",
                        ofs, entities);
                return;
            }
            entity[ofs].obsmode1 = (byte)value;
            return;

        case 22:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "WriteInt: bad offset to entity.autoface[]: %d (%d "
                        "total)",
                        ofs, entities);
                return;
            }
            entity[ofs].face = (byte)value;
            return;

        case 23:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "WriteInt: bad offset to entity.visible[]: %d (%d "
                        "total)",
                        ofs, entities);
                return;
            }
            entity[ofs].visible = (byte)value;
            ;
            return;

        case 24:
            if (ofs < 0 || ofs >= entities) {
                if (v2_touchy)
                    Sys_Error(
                        "WriteInt: bad offset to entity.on[]: %d (%d total)",
                        ofs, entities);
                return;
            }
            entity[ofs].on = (byte)value;
            return;

        case 29:
            if (ofs < 0 || ofs >= numzones) {
                if (v2_touchy)
                    Sys_Error(
                        "WriteInt: bad offset to zone.event: %d (%d total)",
                        ofs, numzones);
            }
            zones[ofs].script = value; // Zara made me add it!
            return;

        case 30:
            if (ofs < 0 || ofs >= numzones)
                if (v2_touchy)
                    Sys_Error(
                        "WriteInt: bad offset to zone.chance: %d (%d total)",
                        ofs, numzones);
            zones[ofs].percent = value;
            return;
        }
    case op_LVAR:
        if (loc < 0 || loc > 19) {
            Sys_Error("WriteInt: bad offset to local ints: %d", loc);
        }
#ifdef NEW_LOCALS
        int_stack[int_stack_base + loc] = value;
#else // OLD LOCALS
        lvar.nargs[loc] = value;
#endif
        return;

    default:
        Sys_Error(
            "VC Execution error: Invalid WriteInt category %d", (int)category);
    }
}

int ResolveOperand() {
    int cr = 0;
    int d = 0;
    byte c = 0;

    cr = ProcessOperand(); // Get base number
    while (1) {
        c = GrabC();
        switch (c) {
        case op_ADD:
            cr += ProcessOperand();
            continue;
        case op_SUB:
            cr -= ProcessOperand();
            continue;
        case op_DIV:
            d = ProcessOperand();
            if (!d)
                cr = 0;
            else
                cr /= d;
            continue;
        case op_MULT:
            cr = cr * ProcessOperand();
            continue;
        case op_MOD:
            d = ProcessOperand();
            if (!d)
                cr = 0;
            else
                cr %= d;
            continue;
        case op_SHL:
            cr = cr << ProcessOperand();
            continue;
        case op_SHR:
            cr = cr >> ProcessOperand();
            continue;
        case op_AND:
            cr = cr & ProcessOperand();
            continue;
        case op_OR:
            cr = cr | ProcessOperand();
            continue;
        case op_XOR:
            cr = cr ^ ProcessOperand();
            continue;
        case op_END:
            break;
        }
        break;
    }
    return cr;
}

int ProcessOperand() {
    byte op_desc = 0;
    byte c = 0;
    quad d = 0;
    quad ofs = 0;

    op_desc = GrabC();
    switch (op_desc) {
    case op_IMMEDIATE:
        return GrabD();
    case op_HVAR0:
        c = GrabC();
        return ReadInt(op_HVAR0, c, 0);
    case op_HVAR1:
        c = GrabC();
        ofs = ResolveOperand();
        return ReadInt(op_HVAR1, c, ofs);
    case op_UVAR:
        d = GrabD();
        return ReadInt(op_UVAR, d, 0);
    case op_UVARRAY:
        d = GrabD();
        d += ResolveOperand();
        return ReadInt(op_UVARRAY, d, 0);
    case op_LVAR:
        c = GrabC();
        if (c > 19) {
            Sys_Error("ProcessOperand: bad offset to local ints: %d", c);
        }
#ifdef NEW_LOCALS
        return int_stack[int_stack_base + c];
#else
        return lvar.nargs[c];
#endif
    case op_BFUNC:
        HandleStdLib();
        return vcreturn;
    case op_UFUNC:
        HandleExternFunc();
        return vcreturn;
    case op_GROUP:
        return ResolveOperand();
    default:
        Sys_Error("VC Execution error: Invalid operand %d.", op_desc);
        break;
    }
    return 0;
}

string_k HandleStringOperand() {
    string_k ret;
    int c;

    c = GrabC();
    switch (c) {
    case s_IMMEDIATE:
        ret = GrabString();
        break;

    case s_GLOBAL:
        c = GrabW();
        if (c >= 0 && c < stralloc) {
            ret = vc_strings[c];
        } else
            Sys_Error("HandleStringOperand: bad offset to vc_strings");
        break;

    case s_ARRAY:
        c = GrabW();
        c += ResolveOperand();
        if (c >= 0 || c < stralloc) {
            ret = vc_strings[c];
        } else
            Sys_Error("HandleStringOperand: bad offset to vc_strings");
        break;

    case s_NUMSTR:
        ret = va("%d", ResolveOperand());
        break;

    case s_LEFT:
        ret = ResolveString();
        ret = ret.left(ResolveOperand());
        break;

    case s_RIGHT:
        ret = ResolveString();
        ret = ret.right(ResolveOperand());
        break;

    case s_MID:
        ret = ResolveString();
        c = ResolveOperand();
        ret = ret.mid(c, ResolveOperand());
        break;

    case s_CHR:
        ret = (char)ResolveOperand();
        break;

    case s_LOCAL:
        c = GrabC();
#ifdef DEBUG_LOCALS
        Log(va("s_LOCAL: str_stack_base=%d, c=%d", str_stack_base, c));
#endif
        if (c >= 0 && c < 20) {
#ifdef NEW_LOCALS
            ret = str_stack[str_stack_base + c];
#else
            ret = lvar.s[c];
#endif
        } else
            Sys_Error(
                "HandleStringOperand: bad offset to local strings: %d", c);
        break;

    // sweet
    case s_UFUNC:
        HandleExternFunc();
        ret = vcretstr;
        break;

    default:
        Sys_Error("Invalid VC string operand %d", c);
    }

    return ret;
}

string_k ResolveString() {
    string_k ret;
    int c;

    ret = HandleStringOperand();
    do {
        c = GrabC();
        if (s_ADD == c)
            ret += HandleStringOperand();
        else if (s_END != c)
            Sys_Error("VC execution error: Unknown string operator %d", c);
    } while (c != s_END);

    return ret;
}

void vcpush(quad info) {
    if (vcsp >= vcstack + 1500)
        Sys_Error("VC stack overflow.");

    *vcsp++ = info;
}

quad vcpop() {
    if (vcsp <= vcstack)
        Sys_Error("VC stack underflow.");

    return *--vcsp;
}

// This might be better place in conlib or something, I dunno. --tSB
extern string_k Con_GetArg(int x);

void ReadVCVar() {
    int i = 0;
    int j = 0;
    int ofs;

    string_k arg1 = Con_GetArg(1).lower();

    // Search the int list
    for (i = 0; i <= numvars; i++)
        if (!strcmp(vars[i].vname, arg1.c_str()))
            break;

    if (i < numvars) {
        j = vars[i].varstartofs;

        if (vars[i].arraylen > 1) // if it's an array
        {
            ofs = V_atoi(Con_GetArg(2).c_str()); // get the next argument, and
                                                 // use it as an offset
            j = globalint[j + ofs];
            sprintf(strbuf, "%s[%d]=%d", vars[i].vname, ofs, j);
        } else {
            j = globalint[j];
            sprintf(strbuf, "%s=%d", vars[i].vname, j);
        }
        Console_Printf(strbuf);
        return;
    }

    /*  // not an int.  Check the string variables.
      for (i=0; i<=numstr; i++)
       if (!strcmp(str[i].vname, (const char*)arg1))
        break;

      if (i<numstr)
      {
        if (str[i].arraylen>1) // array?
         {
          ofs=V_atoi((const char*)Con_GetArg(2)); // get the offset
          sprintf(strbuf,"%s[%d]=%s",str[i].vname,ofs,(const
      char*)vc_strings[i+ofs]);
         }
        else
         {
          sprintf(strbuf,"%s=%s",str[i].vname, (const char*)vc_strings[i]);
         }
        Console_Printf(strbuf);
        return;
      }*/
    Console_Printf("No such VC variable.");
}

void WriteVCVar() {
    int i = 0;
    int j = 0;
    int ofs;

    string_k arg1 = Con_GetArg(1).lower();

    for (i = 0; i <= numvars; i++)
        if (!strcmp(vars[i].vname, arg1.c_str()))
            break;

    if (i < numvars) {
        j = vars[i].varstartofs;
        if (vars[i].arraylen > 1) {
            ofs = V_atoi(Con_GetArg(2).c_str());
            globalint[j + ofs] = V_atoi(Con_GetArg(3).c_str());
            sprintf(strbuf, "%s[%d]=%d", vars[i].vname, ofs,
                V_atoi(Con_GetArg(3).c_str()));
        } else {
            globalint[j] = V_atoi(Con_GetArg(2).c_str());
            sprintf(
                strbuf, "%s=%d", vars[i].vname, V_atoi(Con_GetArg(2).c_str()));
        }
        Console_Printf(strbuf);
        return;
    }
    /*  for (i=0; i<=numstr; i++)
        if (!strcmp(str[i].vname, (const char*)arg1))
                    break;

      if (i<numstr)
      {
        j=(int) stringbuf + (i*256);
            V_strncpy((char *)j, (const char*)Con_GetArg(2), 255);
        sprintf(strbuf,"%s:%s", str[i].vname, (const char*)Con_GetArg(2));
        Console_Printf(strbuf);
        return;
      }*/
    Console_Printf("No such VC variable.");
}

// ===================== New file stuff --tSB ==========================

int OpenVCFile(const char* fname)
// Opens a VC file for reading, and returns the index to vcfiles[], or 0 on fail
// note that vcfiles[0] is a dummy, and isn't actually used anywhere
{
    int i; // counter :P

    for (i = 1; i < MAXVCFILES; i++) {
        if (vcfiles[i].mode == 0) {
            vcfiles[i].readp = vopen(fname);

            if (!vcfiles[i].readp)
                return 0; // file ain't there?

            vcfiles[i].mode = 1;
            return i;
        }
    }
    return 0;
}

int OpenWriteVCFile(const char* fname)
// Opens a VC file for writing, and returns the index to vcfiles[], or -1 if
// fail
{
    for (int i = 1; i < MAXVCFILES; i++) {
        if (vcfiles[i].mode == 0) {
            vcfiles[i].writep = _fopen(fname, "wb");

            if (!vcfiles[i].writep)
                return 0; // dunno, sharing violation?

            vcfiles[i].mode = 2;
            return i;
        }
    }
    return 0;
}

void CloseVCFile(int index)
// closes the specified file, if it's open
{
    if (vcfiles[index].mode == 1) {
        vclose(vcfiles[index].readp);
        wasm_syncFileSystem();
    } else if (vcfiles[index].mode == 2) {
        fclose(vcfiles[index].writep);
    }
    vcfiles[index].mode = 0;
}

VFILE* GetReadFilePtr(int idx)
// returns the VFILE, if idx is open for reading, returns an error otherwise
{
    if (idx < 0 || idx >= MAXVCFILES)
        Sys_Error(va("GetReadFilePtr: Invalid file handle %i", idx));
    switch (vcfiles[idx].mode) {
    case 0:
        Sys_Error("GetReadFilePtr: Attempt to read from a closed file.");
    case 2:
        Sys_Error(
            "GetReadFilePtr: Attempt to read from a file opened for writing.");
    }
    return vcfiles[idx].readp;
}

FILE* GetWriteFilePtr(int idx)
// returns the FILE, if idx is open for writing, returns an error otherwise
{
    if (idx < 0 || idx >= MAXVCFILES)
        Sys_Error(va("GetWriteFilePtr: Invalid file handle %i", idx));
    switch (vcfiles[idx].mode) {
    case 0:
        Sys_Error("GetWriteFilePtr: Attempt to write to a closed file.");
    case 1:
        Sys_Error(
            "GetWriteFilePtr: Attempt to write to a file opened for reading.");
    }
    return vcfiles[idx].writep;
}

// ======================= VC Standard Function Library =======================

#include "vcstand.h"

// ===================== End VC Standard Function Library =====================

void HandleStdLib() {
    int x = 0;
    byte c = 0;

    c = GrabC();
    switch (c) {
    case 1:
        vc_Exit_();
        break;

    case 2:
        vc_Message();
        break;

    case 3:
        vc_Malloc();
        break;

    case 4:
        vc_Free();
        break;

    case 5:
        vc_pow();
        break;

    case 6:
        vc_loadimage();
        break;

    case 7:
        vc_copysprite();
        break;

    case 8:
        vc_tcopysprite();
        break;

    case 9:
        Render();
        break;

    case 10:
        gfx.ShowPage();
        break;

    case 11:
        vc_EntitySpawn();
        break;

    case 12:
        vc_SetPlayer();
        break;

    case 13:
        vc_Map();
        break;

    case 14:
        vc_LoadFont();
        break;

    case 15:
        vc_PlayFLI();
        break;

    case 16: {
        int x, y;
        x = ResolveOperand();
        y = ResolveOperand();
        Font_GotoXY(x, y);
    } break;

    case 17:
        vc_PrintString();
        break;

    case 18:
        vc_LoadRaw();
        break;

    case 19:
        vc_SetTile();
        break;

    case 20:
        Con_AllowConsole(ResolveOperand());
        break; // allowconsole=ResolveOperand(); break;

    case 21:
        vc_ScaleSprite();
        break;

    case 22:
        ProcessEntities();
        break;

    case 23:
        input.Update(); // Key_SendKeys();
        if (input.key[DIK_LMENU] && input.key[DIK_X])
            Sys_Error("");
        CheckHookTimer();
        CheckMessages();
        break; // UpdateControls(); break;

    case 24:
        vc_UnPress();
        break;

    case 25:
        vc_EntityMove();
        break;

    case 26:
        vc_HLine();
        break;

    case 27:
        vc_VLine();
        break;

    case 28:
        vc_Line();
        break;

    case 29:
        vc_Circle();
        break;

    case 30:
        vc_CircleFill();
        break;

    case 31:
        vc_Rect();
        break;

    case 32:
        vc_RectFill();
        break;

    case 33:
        vc_strlen();
        break;

    case 34:
        vc_strcmp();
        break;

    case 35:
        break;
    // CD_Stop(); break;

    case 36:
        ResolveOperand();
        break; // CD_Play(ResolveOperand()); break;

    case 37:
        vc_FontWidth();
        break;

    case 38:
        vc_FontHeight();
        break;

    case 39:
        vc_SetPixel();
        break;

    case 40:
        vc_GetPixel();
        break;

    case 41:
        vc_EntityOnScreen();
        break;

    case 42:
        vcreturn = 0;
        x = ResolveOperand();
        if (x)
            vcreturn = rand() % x;
        break;

    case 43:
        vc_GetTile();
        break;

    case 44:
        vc_HookRetrace();
        break;

    case 45:
        vc_HookTimer();
        break;

    case 46:
        vc_SetResolution();
        break;

    case 47:
        vc_SetRString();
        break;

    case 48:
        vc_SetClipRect();
        break;

    case 49:
        vc_SetRenderDest();
        break;

    case 50:
        vc_RestoreRenderSettings();
        break;

    case 51:
        vc_PartyMove();
        break;

    case 52: {
        int n = ResolveOperand();
        if (n < 0 || n >= 360)
            Sys_Error("HandleStdLib: bad offset to sintbl");
        vcreturn = sintbl[n];
    } break;

    case 53: {
        int n = ResolveOperand();
        if (n < 0 || n >= 360)
            Sys_Error("HandleStdLib: bad offset to costbl");
        vcreturn = costbl[n];
    } break;

    case 54: {
        int n = ResolveOperand();
        if (n < 0 || n >= 360)
            Sys_Error("HandleStdLib: bad offset to tantbl");
        vcreturn = tantbl[n];
    } break;

    case 55:
        //		Mouse_Read();
        input.Update();
        CheckMessages();
        break;

    case 56:
        ResolveOperand();
        // ClipOn=ResolveOperand();
        break;

    case 57:
        lucentmode = ResolveOperand();
        break;

    case 58:
        vc_WrapBlit();
        break;

    case 59:
        vc_TWrapBlit();
        break;

    case 60:
        vc_SetMousePos();
        break;

    case 61:
        vc_HookKey();
        break;

    case 62:
        vc_PlayMusic();
        break;

    case 63:
        StopMusic();
        break;

    case 64:
        vc_PaletteMorph();
        break;

    case 65:
        vc_OpenFile();
        break;

    case 66:
        vc_CloseFile();
        break;

    case 67:
        vc_QuickRead();
        break;

    case 68:
        vc_AddFollower();
        break;

    // case 69: vc_KillFollower(); break;
    // case 70: vc_KillAllFollowers(); break;
    // case 71: ResetFollowers();

    case 72:
        vc_FlatPoly();
        break;

    case 73:
        vc_TMapPoly();
        break;

    case 74:
        vc_CacheSound();
        break;

    case 75:
        FreeAllSounds();
        break;

    case 76:
        vc_PlaySound();
        break;

    case 77:
        vc_RotScale();
        break;

    case 78:
        vc_MapLine();
        break;

    case 79:
        vc_TMapLine();
        break;

    case 80:
        vc_val();
        break;

    case 81:
        vc_TScaleSprite();
        break;

    case 82:
        vc_GrabRegion();
        break;

    case 83:
        vc_Log();
        break;

    case 84:
        vc_fseekline();
        break;

    case 85:
        vc_fseekpos();
        break;

    case 86:
        vc_fread();
        break;

    case 87:
        vc_fgetbyte();
        break;

    case 88:
        vc_fgetword();
        break;

    case 89:
        vc_fgetquad();
        break;

    case 90:
        vc_fgetline();
        break;

    case 91:
        vc_fgettoken();
        break;

    case 92:
        vc_fwritestring();
        break;

    case 93:
        vc_fwrite();
        break;

    case 94:
        vc_frename();
        break;

    case 95:
        vc_fdelete();
        break;

    case 96:
        vc_fwopen();
        break;

    case 97:
        vc_fwclose();
        break;

    case 98:
        vc_memcpy();
        break;

    case 99:
        vc_memset();
        break;

    case 100:
        vc_Silhouette();
        break;

    case 101:
        // vcreturn=(int) InitMosaicTable();
        vcreturn = 0;
        break;

    case 102:
        vc_Mosaic();
        break;

    case 103:
        vc_WriteVars();
        break;

    case 104:
        vc_ReadVars();
        break;

    case 105:
        ExecuteEvent(ResolveOperand());
        break;

    case 106:
        vc_Asc();
        break;

    case 107:
        ExecuteUserFunc(ResolveOperand());
        break;

    case 108:
        vc_NumForScript();
        break;

    case 109:
        vc_Filesize();
        break;

    case 110:
        vc_FTell();
        break;

    case 111:
        vc_ChangeCHR();
        break;

    case 112:
        vc_RGB();
        break;

    case 113:
        vc_GetR();
        break;

    case 114:
        vc_GetG();
        break;

    case 115:
        vc_GetB();
        break;

    case 116:
        vc_Mask();
        break;

    case 117:
        vc_ChangeAll();
        break;

    case 118:
        vcreturn = (int)sqrt(ResolveOperand());
        break;
    case 119:
        vc_fwritebyte();
        break;
    case 120:
        vc_fwriteword();
        break;
    case 121:
        vc_fwritequad();
        break;
    case 122: // ResolveOperand();
        gfx.CalcLucentLUT(ResolveOperand());
        break;
    case 123:
        vc_ImageSize();
        break;
    default:
        Sys_Error("VC Execution error: Invalid STDLIB index. (%d)", (int)c);
    }
}

// ========================== VC Interpretation Core ==========================

int ProcessIf() {
    byte exec, c;

    exec = (byte)ProcessIfOperand(); // Get base value;
    while (1) {
        c = GrabC();
        switch (c) {
        case i_AND:
            exec = (byte)(exec & ProcessIfOperand());
            continue;
        case i_OR:
            exec = (byte)(exec | ProcessIfOperand());
            continue;
        case i_UNGROUP:
            break;
        }
        break;
    }
    return exec;
}

int ProcessIfOperand() {
    byte op_desc;
    int eval;

    eval = ResolveOperand();
    op_desc = GrabC();
    switch (op_desc) {
    case i_ZERO:
        if (!eval)
            return 1;
        else
            return 0;
    case i_NONZERO:
        if (eval)
            return 1;
        else
            return 0;
    case i_EQUALTO:
        if (eval == ResolveOperand())
            return 1;
        else
            return 0;
    case i_NOTEQUAL:
        if (eval != ResolveOperand())
            return 1;
        else
            return 0;
    case i_GREATERTHAN:
        if (eval > ResolveOperand())
            return 1;
        else
            return 0;
    case i_GREATERTHANOREQUAL:
        if (eval >= ResolveOperand())
            return 1;
        else
            return 0;
    case i_LESSTHAN:
        if (eval < ResolveOperand())
            return 1;
        else
            return 0;
    case i_LESSTHANOREQUAL:
        if (eval <= ResolveOperand())
            return 1;
        else
            return 0;
    case i_GROUP:
        if (ProcessIf())
            return 1;
        else
            return 0;
    }

    return 0;
}

void HandleIf() {
    char* d;

    if (ProcessIf()) {
        GrabD();
        return;
    }
    d = (char*)GrabD();
    code = (char*)(int)basevc + (int)d;
}

#if !defined(NEW_LOCALS) // *****

// assumes arguments are valid pointers
inline void CopyLocal(lvars* dest, lvars* source) {
    int n;

    V_memcpy(dest->nargs, source->nargs, MAX_ARGS * 4);

    for (n = MAX_LOCAL_STRINGS - 1; n >= 0; n--)
        dest->s[n] = source->s[n];
}

// assumes a valid pointer
inline void ClearLocal(lvars* dest) {
    int n;

    V_memset(dest->nargs, 0, MAX_ARGS * 4);

    for (n = MAX_LOCAL_STRINGS - 1; n >= 0; n--)
        dest->s[n] = "";
}

#endif // !def NEW_LOCALS

static int routine_depth = 0;

#ifdef DEBUG_LOCALS
std::string indent;
#endif

void HandleExternFunc() {
    funcdecl* pfunc;
    int n;
    int ilb = 0, slb = 0;

    n = GrabW();
    if (n < 0 || n >= numfuncs) {
        Sys_Error("HandleExternFunc: VC sys script out of bounds (%d/%d)", n,
            numfuncs);
    }
    pfunc = funcs + n;

#ifdef NEW_LOCALS // *****
    ilb = int_last_base;
    slb = str_last_base;
    PushBase(int_last_base, str_last_base);

#ifdef DEBUG_LOCALS
    Log(va("%s HandleExternFunc %s", indent.c_str(), pfunc->fname));
    indent += ' ';
#endif
    int isp, ssp;

    // we do not set the new base until we're done reading in the arguments--
    // this is because we might still need to read in local vars passed from the
    // previous function (lookup for locals works off current base values).
    // for now, just tag the to-be bases.
    isp = int_stack_ptr;
    ssp = str_stack_ptr;
    // allocate stack space
    if (pfunc->numlocals) {
        // read in arguments
        for (n = 0; n < pfunc->numargs; n++) {
            switch (pfunc->argtype[n]) {
            case 1:
                PushInt(ResolveOperand());
                break;
            case 3:
                PushStr(ResolveString());
                break;
            }
        }
        // finish off allocating locals
        while (n < pfunc->numlocals) {
            switch (pfunc->argtype[n]) {
            case 1:
                PushInt(0);
                break;
            case 3:
                PushStr("");
                break;
            }
            n++;
        }
    }
    // now we're safe to set the bases
    int_stack_base = int_last_base = isp;
    str_stack_base = str_last_base = ssp;
#else  // OLD LOCALS
    lvars temp, ob;

    // save lvar
    CopyLocal(&temp, &lvar);

    ClearLocal(&ob);
    int k = 0;
    for (n = 0; n < pfunc->numargs; n++) {
        switch (pfunc->argtype[n]) {
        case 1:
            ob.nargs[n] = ResolveOperand();
            break;
        case 3:
            if (k < 0 || k >= MAX_LOCAL_STRINGS)
                Sys_Error("HandleExternFunc: too many locals strings? @_@");
            ob.s[k++] = ResolveString();
            break;
        }
    }
    // copy in ob
    CopyLocal(&lvar, &ob);
#endif // OLD LOCALS

    vcpush((quad)basevc);
    vcpush((quad)code);

    basevc = sysvc;
    code = (char*)(basevc + pfunc->syscodeofs);

    if (vctrack) {
        std::string prefix(' ', routine_depth);
        Log(va("%s --> Entering user func %s, codeofs %d", prefix.c_str(), pfunc->fname,
            pfunc->syscodeofs));
        routine_depth++;
    }

    ExecuteBlock();

    basevc = (char*)vcpop();

#ifdef NEW_LOCALS // *****
                  // restore previous base
    PopBase();
    int_last_base = ilb;
    str_last_base = slb;
    // free stack space
    if (pfunc->numlocals) {
        // clear out all locals (args + 'true' locals)
        for (n = 0; n < pfunc->numlocals; n++) {
            switch (pfunc->argtype[n]) {
            case 1:
                PopInt();
                break;
            case 3:
                PopStr();
                break;
            }
        }
    }
#ifdef DEBUG_LOCALS
    indent.pop_back();
    Log(va("%s <<< HandleExternFunc", indent.c_str()));
#endif
#else  // OLD LOCALS
    // restore lvar
    CopyLocal(&lvar, &temp);
#endif // OLD LOCALS

    if (vctrack) {
        routine_depth--;
        std::string prefix(' ', routine_depth);
        Log(va("%s --> Returned from %s", prefix.c_str(), pfunc->fname));
    }
}

void HandleAssign() {
    int op, c, base, offset, value;

    c = GrabC();

    // string assignment
    if (c == op_STRING) {
        offset = GrabW();
        c = GrabC();
        if (c != a_SET) {
            Sys_Error("VC execution error: Corrupt string assignment");
        }
        if (offset >= 0 && offset < stralloc) {
            vc_strings[offset] = ResolveString();
        } else
            Sys_Error("HandleAssign: bad offset to vc_strings (var)");
        return;
    }
    // string array assignment
    if (c == op_SARRAY) {
        offset = GrabW();
        offset += ResolveOperand();
        c = GrabC();
        if (c != a_SET) {
            Sys_Error("VC execution error: Corrupt string assignment");
        }
        if (offset >= 0 && offset < stralloc) {
            vc_strings[offset] = ResolveString();
        } else
            Sys_Error("HandleAssign: bad offset to vc_strings (arr)");
        return;
    }
    // local string assignment
    if (c == op_SLOCAL) {
        offset = GrabW();
        c = GrabC();
        if (c != a_SET) {
            Sys_Error("VC execution error: Corrupt string assignment");
        }
        if (offset >= 0 && offset < 20) // MAX_LOCAL_STRINGS)
        {
#ifdef NEW_LOCALS
            str_stack[str_stack_base + offset] = ResolveString();
#else  // OLD LOCALS
            lvar.s[offset] = ResolveString();
#endif // OLD LOCALS
        } else
            Sys_Error("HandleAssign: bad offset to local strings: %d", c);
        return;
    }

    // integer assignment
    base = offset = 0;
    switch (c) {
    case op_UVAR:
        base = GrabD();
        break;
    case op_UVARRAY:
        base = GrabD();
        base += ResolveOperand();
        break;
    case op_HVAR0:
        base = GrabC();
        break;
    case op_HVAR1:
        base = GrabC();
        offset = ResolveOperand();
        break;
    case op_LVAR:
        base = GrabC();
        break;

    default:
        Sys_Error("VC Execution error: Unknown assignment category.");
    }
    value = ReadInt((char)c, base, offset);
    op = GrabC();
    switch (op) {
    case a_SET:
        value = ResolveOperand();
        break;
    case a_INC:
        value++;
        break;
    case a_DEC:
        value--;
        break;
    case a_INCSET:
        value += ResolveOperand();
        break;
    case a_DECSET:
        value -= ResolveOperand();
        break;

    default:
        Sys_Error("VC Execution error: Invalid assignment operator %d.", op);
    }
    WriteInt((char)c, base, offset, value);
}

void HandleSwitch() {
    int realvalue = 0;
    int compvalue = 0;
    byte c = 0;
    byte* next = 0;

    realvalue = ResolveOperand();
    c = GrabC();
    while (c != opRETURN) {
        compvalue = ResolveOperand();
        next = (byte*)GrabD();
        if (compvalue != realvalue) {
            code = (char*)(int)basevc + (int)next;
            c = GrabC();
            continue;
        }
        ExecuteSection();
        c = GrabC();
    }
}

void ExecuteVC() {
    byte c = 0;
    
    static const int BREAK_INTERVAL = 50;
    static int breakTime;
    breakTime = BREAK_INTERVAL;

    while (1) {
        if (kill)
            break;

        --breakTime;
        if (breakTime < 0) {
            wasm_nextFrame();
            breakTime = BREAK_INTERVAL;
        }

        CheckMessages();
        if (input.key[DIK_LMENU] && input.key['x'])
            Sys_Error("");

        c = GrabC();
        switch (c) {
        case opEXEC_STDLIB:
            HandleStdLib();
            break;
        case opEXEC_LOCALFUNC:
            break;
        case opEXEC_EXTERNFUNC:
            HandleExternFunc();
            break;
        case opIF:
            HandleIf();
            break;
        case opELSE:
            break;
        case opGOTO:
            code = basevc + GrabD();
            break;
        case opSWITCH:
            HandleSwitch();
            break;
        case opASSIGN:
            HandleAssign();
            break;
        case opRETURN:
            code = (char*)vcpop();
            break;
        case opSETRETVAL:
            vcreturn = ResolveOperand();
            break;
        case opSETRETSTRING:
            vcretstr = ResolveString();
            break;

        default:
            Sys_Error(
                "Internal VC execution error. (%d)", (int)code - (int)basevc);
        }

        if ((int)code != -1)
            continue;
        else
            break;
    }
}

void ExecuteBlock() {
    byte c = 0;

    static const int BREAK_INTERVAL = 50;
    static int breakTime;
    breakTime = BREAK_INTERVAL;

    while (1) {
        if (kill)
            break;

        --breakTime;
        if (breakTime < 0) {
            wasm_nextFrame();
            breakTime = BREAK_INTERVAL;
        }

        CheckMessages();
        if (input.key[DIK_LMENU] && input.key['x'])
            Sys_Error("");

        c = GrabC();
        switch (c) {
        case opEXEC_STDLIB:
            HandleStdLib();
            break;
        case opEXEC_LOCALFUNC:
            break;
        case opEXEC_EXTERNFUNC:
            HandleExternFunc();
            break;
        case opIF:
            HandleIf();
            break;
        case opELSE:
            break;
        case opGOTO:
            code = basevc + GrabD();
            break;
        case opSWITCH:
            HandleSwitch();
            break;
        case opASSIGN:
            HandleAssign();
            break;
        case opRETURN:
            code = (char*)vcpop();
            break;
        case opSETRETVAL:
            vcreturn = ResolveOperand();
            break;
        case opSETRETSTRING:
            vcretstr = ResolveString();
            break;

        default:
            Sys_Error(
                "Internal VC execution error. (%d)", (int)code - (int)basevc);
        }

        if (c == opRETURN)
            break;
    }
}

void ExecuteSection() {
    byte c = 0;

    static const int BREAK_INTERVAL = 50;
    static int breakTime;
    breakTime = BREAK_INTERVAL;

    while (1) {
        if (kill)
            break;

        --breakTime;
        if (breakTime < 0) {
            wasm_nextFrame();
            breakTime = BREAK_INTERVAL;
        }

        CheckMessages();
        if (input.key[DIK_LMENU] && input.key['x'])
            Sys_Error("");

        c = GrabC();
        switch (c) {
        case opEXEC_STDLIB:
            HandleStdLib();
            break;
        case opEXEC_LOCALFUNC:
            break;
        case opEXEC_EXTERNFUNC:
            HandleExternFunc();
            break;
        case opIF:
            HandleIf();
            break;
        case opELSE:
            break;
        case opGOTO:
            code = basevc + GrabD();
            break;
        case opSWITCH:
            HandleSwitch();
            break;
        case opASSIGN:
            HandleAssign();
            break;
        case opRETURN:
            break;
        case opSETRETVAL:
            vcreturn = ResolveOperand();
            break;
        case opSETRETSTRING:
            vcretstr = ResolveString();
            break;
        default:
            Sys_Error(
                "Internal VC execution error. (%d)", (int)code - (int)basevc);
        }

        if (c != opRETURN)
            continue;
        else
            break;
    }
}

void ExecuteEvent(int ev) {
    if (ev < 0 || ev >= mapevents) {
        Sys_Error("ExecuteEvent: VC event out of bounds (%d)", ev);
    }

    ++invc;

    vcpush((quad)code);
    vcpush((quad)basevc);

    basevc = mapvc;
    code = basevc + event_offsets[ev];

    vcpush((quad)-1);
    ExecuteVC();

    basevc = (char*)vcpop();
    code = (char*)vcpop();

    --invc;

    // timer_count=0;
}

void ExecuteUserFunc(int ufunc) {
    int ilb = 0, slb = 0;
    funcdecl* pfunc;

    if (ufunc < 0 || ufunc >= numfuncs) {
        Sys_Error("VC sys script out of bounds (%d)", ufunc);
    }
    pfunc = funcs + ufunc;

#ifdef NEW_LOCALS // *****
#ifdef DEBUG_LOCALS
    Log(">>> ExecuteUserFunc");
#endif

    // straight push of the current stack pointers
    ilb = int_last_base;
    slb = str_last_base;
    PushBase(int_last_base, str_last_base);
    int_stack_base = int_last_base = int_stack_ptr;
    str_stack_base = str_last_base = str_stack_ptr;
    int n;
    // allocate stack space
    if (pfunc->numlocals) {
        // only locals
        for (n = 0; n < pfunc->numlocals; n++) {
            switch (pfunc->argtype[n]) {
            case 1:
                PushInt(0);
                break;
            case 3:
                PushStr("");
                break;
            }
        }
    }
#else  // OLD LOCALS
    lvars temp;
    // save lvar
    CopyLocal(&temp, &lvar);
    // now wipe it
    ClearLocal(&lvar);
#endif // OLD LOCALS

    vcpush((quad)code);
    vcpush((quad)basevc);

    basevc = sysvc;
    code = (char*)(basevc + pfunc->syscodeofs);

    vcpush((quad)-1);

    ExecuteVC();

    basevc = (char*)vcpop();
    code = (char*)vcpop();

#ifdef NEW_LOCALS // *****
                  // restore previous base
    PopBase();
    int_last_base = ilb;
    str_last_base = slb;
    // free stack space
    if (pfunc->numlocals) {
        // clear out all locals (args + 'true' locals)
        for (n = 0; n < pfunc->numlocals; n++) {
            switch (pfunc->argtype[n]) {
            case 1:
                PopInt();
                break;
            case 3:
                PopStr();
                break;
            }
        }
    }
#ifdef DEBUG_LOCALS
    Log("<<< ExecuteUserFunc");
#endif
#else  // OLD LOCALS
    // restore lvar
    CopyLocal(&lvar, &temp);
#endif // OLD LOCALS
}

void HookRetrace() {
    if (!hookretrace)
        return;

    if (hookretrace < USERFUNC_MARKER)
        ExecuteEvent(hookretrace);
    if (hookretrace >= USERFUNC_MARKER)
        ExecuteUserFunc(hookretrace - USERFUNC_MARKER);
}

void CheckHookTimer() {
    while (hktimer) {
        HookTimer();
        hktimer--;
    }
}

void HookTimer() {
    if (!hooktimer)
        return;

    if (hooktimer < USERFUNC_MARKER)
        ExecuteEvent(hooktimer);
    if (hooktimer >= USERFUNC_MARKER)
        ExecuteUserFunc(hooktimer - USERFUNC_MARKER);
}

void HookKey(int script) {
    if (!script)
        return;

    if (script < USERFUNC_MARKER)
        ExecuteEvent(script);
    if (script >= USERFUNC_MARKER)
        ExecuteUserFunc(script - USERFUNC_MARKER);
}
