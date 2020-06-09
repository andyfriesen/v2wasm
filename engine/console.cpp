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
// ³                      Command Console module                         ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

/*
        mod log:

    <tSB>
    12  Nov         2000    Rehashed the keyboard handler.  The console now
   actively pulls keys from the handler, instead
                            of the handler "pushing" them around.
        <aen>
        28	Decemeber	1999	Major revamp.
*/

#include "verge.h"

#define CONSOLE_TEXT_LINES 100
#define CONSOLE_LAST_LINES 25

byte cpu_watch = 0;
byte cpubyte = 0;

class console_command_t {
    string_k m_name;
    void (*m_execute)();

   public:
    console_command_t() : m_execute(0) {}
    console_command_t(string_k s, void (*e)()) : m_name(s), m_execute(e) {}

    string_k name() const { return m_name; }
    void execute() {
        if (m_execute) m_execute();
    }
};

static vector_t<console_command_t> concmds;

///////////////////////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
// /////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////

static int console_processing = 0;

// console background image
static byte *consolebg = 0;

static vector_t<string_k> consoletext;

// TODO: make this static; ramifications w/ VERGE.CC
static string_k glob_cmd;

// last commands list
static vector_t<string_k> lastcmds;
// traverses last commands list; -1 = not traversing last commands
static int cmdpos = -1;

static vector_t<string_k> args;

static char cursor = 1;           // flag on/off cursor visible
static unsigned int cswtime = 0;  // cursor switch time.

// aen <24dec99> Changed to fixed point.
static int conlines = 0;  // Number of visible lines

static int backtrack = 0;

static int allowconsole = 1;
static int consoleoverride = 0;

int Con_Width() { return gfx.XRes(); }
int Con_Length()
//	{ return (int)((120.0/200.0)*gfx.YRes() + 2); }
{
    return (gfx.YRes() * 120) / 200 + 2;
}
int Con_Lines() { return Con_Length() / Font_GetLength(0) - 1; }
int Con_CharsPerLine() { return Con_Width() / Font_GetWidth(0) - 2; }

int Con_NumArgs() { return args.size(); }

string_k Con_GetArg(int x) {
    return (x >= 0 && x < args.size()) ? args[x] : "";
}

int Con_BottomEdge() { return conlines; }

void Con_SetViewablePixelRows(int x) {
    if (x < 0) x = 0;
    if (x > Con_Length()) x = Con_Length();

    conlines = x;
}

int Con_ViewLines() { return Con_Length() / Font_GetLength(0) + 2; }

int Con_IsConsoleAllowed() { return allowconsole; }
void Con_AllowConsole(int allow) { allowconsole = allow; }

int Con_IsOverriden() { return consoleoverride; }
void Con_SetOverride(int override) { consoleoverride = override; }

/*
static byte key_ascii_tbl[128] =
{
  0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,   9,
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13,  0,   'a',
's',
  'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39,  0,   0,   92,  'z', 'x', 'c',
'v',
  'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   3,   3,   3,   3,   8,
  3,   3,   3,   3,   3,   0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,
  0,   0,   0,   127, 0,   0,   92,  3,   3,   0,   0,   0,   0,   0,   0,   0,
  13,  0,   '/', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 127,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '/', 0,   0,   0,   0,   0
};

static byte key_shift_tbl[128] =
{
   0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 126,
126,
   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 126, 0,   'A',
'S',
   'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', 34,  0,   0,   '|', 'Z', 'X', 'C',
'V',
   'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   1,   0,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,
   0,   0,   1,   127, 0,   0,   0,   1,   1,   0,   0,   0,   0,   0,   0,   0,
   13,  0,   '/', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 127,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '/', 0,   0,   0,   0,   0
};
*/

static void Con_AddCommand(string_k name, void (*execute)()) {
    // alphabetize, heh! cool.
    int n;
    for (n = 0; n < concmds.size(); ++n)
        if (concmds[n].name() > name) break;
    concmds.insert(n, console_command_t(name, execute));
}

void Console_Draw(void) {
    Render();
    if (consolebg) {
        gfx.ScaleSprite(0, -Con_Length() + Con_BottomEdge(), 320, 120,
            Con_Width(), Con_Length(), consolebg);
    } else {
        int y = -Con_Length() + Con_BottomEdge();
        gfx.RectFill(0, y, Con_Width(), y + Con_Length() - 1, 0, 1);
    }

    // write console text
    if (consoletext.size() > 0) {
        // aen <june 8 2000> rewrote
        int pixelrow =
            Con_BottomEdge() - 1 - Font_GetLength(0) * (2 + (backtrack > 0));
        int index = backtrack;
        do {
            Font_GotoXY(1, pixelrow);
            Font_Print(0, consoletext[index++].c_str());
            pixelrow -= Font_GetLength(0);

        } while (pixelrow > -Font_GetLength(0));
    }

    if (backtrack > 0) {
        Font_GotoXY(1, Con_BottomEdge() - 1 - Font_GetLength(0) * 2);
        while (Font_GetX() + (Font_GetWidth(0) * 3) < gfx.XRes()) {
            Font_Print(0, "^   ");
        }
    }

    // paint command prompt & cursor
    Font_GotoXY(1, Con_BottomEdge() - 1 - Font_GetLength(0));
    int startx = 0;
    if (glob_cmd.length() > Con_CharsPerLine()) {
        Font_Print(0, cursor ? "<" : " ");
        startx = glob_cmd.length() - Con_CharsPerLine();
    } else {
        Font_Print(0, "]");
    }
    Font_Print(0, glob_cmd.mid(startx, Con_CharsPerLine()).c_str());
    if (systemtime >= cswtime) {
        cursor ^= 1;
        cswtime = systemtime + 40;
    }
    if (cursor) Font_Print(0, "-");
}

void ParseCommand(string_k text)
// aen <june 8 2000> rewrote
{
    args.clear();

    const char *s = text.c_str();

    while (*s) {
        // skip whitespace
        while (*s && ' ' >= *s) ++s;

        string_k token;
        // clump non-whitespace
        while (*s && ' ' < *s) token += *s++;

        // add it to the argument list!
        if (token.length() > 0) args.push(token);
    }
}

void ConKey_Tab() {
    int len = glob_cmd.length();
    if (len < 1) return;

    // command completion
    for (int n = 0; n < concmds.size(); ++n) {
        if (glob_cmd.lower().mid(0, len) ==
            concmds[n].name().lower().mid(0, len)) {
            glob_cmd = concmds[n].name().lower();
            return;
        }
    }
}

void ConKey_Type(int k) {
    // printable characters
    if (k > 31 && k < 128) {
        if (glob_cmd.length() < 256) glob_cmd += (char)k;
    }
}

void Con_Key(int key) {
    switch (key) {
    // done w/ console
    case DIK_GRAVE:
        console_processing = 0;
        break;

    case DIK_HOME:
        backtrack = consoletext.size() - 1;
        break;

    case DIK_END:
        backtrack = 0;
        break;

    case DIK_PGUP:
        backtrack += 2;
        if (backtrack > consoletext.size() - 1)
            backtrack = consoletext.size() - 1;
        break;

    case DIK_PGDN:
        backtrack -= 2;

        // scrolling down could become odd; keep even
        if (backtrack & 1) ++backtrack;
        //        backtrack&=~1;                          // I'm so evil --tSB

        if (backtrack < 0) backtrack = 0;
        break;

    case DIK_UP:
        ++cmdpos;
        if (cmdpos > lastcmds.size() - 1) cmdpos = lastcmds.size() - 1;
        glob_cmd = lastcmds[cmdpos];
        break;

    case DIK_DOWN:
        --cmdpos;
        if (cmdpos < -1) cmdpos = -1;
        glob_cmd = cmdpos > -1 ? lastcmds[cmdpos] : "";
        break;

    case DIK_LEFT:
    case DIK_BACK:
        if (glob_cmd.length()) {
            glob_cmd = glob_cmd.left(glob_cmd.length() - 1);
        }
        break;

    case DIK_TAB:
        ConKey_Tab();
        break;

    case DIK_ENTER:
        Console_SendCommand(glob_cmd);
        break;

    default:
        ConKey_Type(input.Scan2ASCII(key));
        break;
    }
}

#include "conlib.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////
// INTERFACE
// //////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////

void Console_Init(void) {
    int slot = 0;

    Logp("Initialize console.");

    // returns -1 if it does not exist; otherwise slot #
    slot = Font_Load("system.fnt");
    // must always sit in first slot
    if (slot != 0) {
        // that is, if it exists
        if (slot > -1)
            Sys_Error("Console_Init: system.fnt in wrong slot (reported %d).\n",
                slot);
    }

    Con_AddCommand("CONSOLEBG", Conlib_ConsoleBG);
    Con_AddCommand("LISTMOUNTS", Conlib_ListMounts);
    Con_AddCommand("PACKINFO", Conlib_PackInfo);
    Con_AddCommand("LISTCMDS", Conlib_ListCmds);
    Con_AddCommand("CD_PLAY", Conlib_CD_Play);
    Con_AddCommand("CD_STOP", Conlib_CD_Stop);
    Con_AddCommand("CD_OPEN", Conlib_CD_Open);
    Con_AddCommand("CD_CLOSE", Conlib_CD_Close);
    Con_AddCommand("EXIT", Conlib_Exit);
    Con_AddCommand("VID_MODE", Conlib_Vid_Mode);
    Con_AddCommand("CPU_USAGE", Conlib_Cpu_Usage);
    Con_AddCommand("MOUNT", Conlib_Mount);
    Con_AddCommand("MAP", Conlib_Map);
    Con_AddCommand("VER", Conlib_Ver);
    Con_AddCommand("BROWSETILES", Conlib_BrowseTiles);
    Con_AddCommand("WARP", Conlib_Warp);
    Con_AddCommand("CAMERATRACKING", Conlib_CameraTracking);
    Con_AddCommand("RSTRING", Conlib_RString);
    Con_AddCommand("SHOWOBS", Conlib_ShowObs);
    Con_AddCommand("PHANTOM", Conlib_Phantom);
    Con_AddCommand("ENTITYSTAT", Conlib_EntityStat);
    Con_AddCommand("ACTIVEENTS", Conlib_ActiveEnts);
    Con_AddCommand("ENTITY", Conlib_Entity);
    Con_AddCommand("CURPOS", Conlib_CurPos);
    Con_AddCommand("PLAYERSPEED", Conlib_PlayerSpeed);
    Con_AddCommand("SPEEDDEMON", Conlib_SpeedDemon);
    Con_AddCommand("RV", Conlib_RV);
    Con_AddCommand("SV", Conlib_SV);
    Con_AddCommand("PLAYER", Conlib_Player);
    Con_AddCommand("SPAWNENTITY", Conlib_SpawnEntity);
    Con_AddCommand("SHOWZONES", Conlib_ShowZones);

    Con_AddCommand("EDIT", Conlib_EditScript);
    // we can send this command... *after* we've added the commands. d'oh.
    Console_SendCommand("consolebg console.gif");

    LogDone();
}

void Console_Printf(string_k s) {
    consoletext.insert(0, s);
    if (consoletext.size() >= CONSOLE_TEXT_LINES) consoletext.pop();

    backtrack = 0;
}

void Console_SendCommand(string_k cmd) {
    // there must be commands available to execute!
    if (concmds.size() < 1) return;

    ParseCommand(cmd.upper());
    if (Con_NumArgs() < 1) {
        glob_cmd = "";
        return;
    }

    // always show what was typed
    Console_Printf("]" + cmd);

    // starting link
    int n;
    for (n = 0; n < concmds.size(); n++)
        if (Con_GetArg(0) == concmds[n].name()) {
            concmds[n].execute();
            break;
        }

    // if we couldn't find anything, let user know
    if (n >= concmds.size()) Console_Printf("*** unrecognized command ***");

    lastcmds.insert(0, cmd);
    if (lastcmds.size() >= CONSOLE_LAST_LINES) lastcmds.pop();

    // globals
    cmdpos = -1;
    glob_cmd = "";
}

void Console_Activate() {
    if (!allowconsole && !consoleoverride) return;

    int tag, r;
    tag = r = 0;

    //	Log("Whap.");                       // o_O

    Con_SetViewablePixelRows(r);

    console_processing = 1;
    while (console_processing) {
        tag = systemtime;

        Console_Draw();
        gfx.ShowPage();

        CheckMessages();  // handles any keypresses, and stores them in the
                          // keyboard
                          // buffer.

        int key;
        // Clear the buffer, handle all the keys. (so none are lost when the
        // framerate goes down)
        while (key = input.GetKey()) Con_Key(key);  // --tSB

        tag = systemtime - tag;
        while (tag > 0) {
            tag--;

            Con_SetViewablePixelRows(r * r);
            if (r * r < Con_Length()) r++;
        }
    }

    // blech
    // TODO: get all main render loops for vc editor, map renders, this console,
    // etc. into a single
    // routine, just like input all goes thru Key_SendKeys.
    while (conlines > 1) {
        tag = systemtime;

        Console_Draw();
        gfx.ShowPage();

        CheckMessages();

        tag = systemtime - tag;
        while (tag) {
            tag--;

            Con_SetViewablePixelRows(r * r);
            if (r > 0) r--;
        }
    }

    timer_count = 0;
}
