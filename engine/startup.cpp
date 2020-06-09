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

// startup.c
// Copyright (C) 1998 BJ Eirich
// This shouldn't really be platform-dependent, but it's put in the platform
// dependent files because of Windows. :P

// tSB: Most of the Win32 crap is here. (the message pump, CreateWindowEx,
// etc...)
//      Sometimes, the graphics driver plays with the window size and little
//      things like that,
//      And w_timer.cpp has a callback function.

//#define WIN32_LEAN_AND_MEAN

#include <stdarg.h>  // va_*()
#include <stdlib.h>
#include <time.h>
#include <windows.h>

//#define MAIN_H
#include "verge.h"

#define TITLE "VERGE v2.6"
#define NAME "MainWindow"

// in VERGE.CPP
extern int VMain();
extern void Log(const char *message);
extern void Logp(const char *message);
extern void InitLog();

extern char logoutput;

// Data

// Win32 stuff
HWND hWnd;
bool bActive;

static unsigned char vergepal[] = {0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x03,
    0x03, 0x03, 0x05, 0x05, 0x05, 0x07, 0x07, 0x07, 0x09, 0x09, 0x09, 0x0a,
    0x0a, 0x0a, 0x0c, 0x0c, 0x0c, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f, 0x11,
    0x11, 0x11, 0x13, 0x13, 0x13, 0x15, 0x15, 0x15, 0x16, 0x16, 0x16, 0x18,
    0x18, 0x18, 0x1a, 0x1a, 0x1a, 0x1c, 0x1c, 0x1c, 0x1d, 0x1d, 0x1d, 0x1f,
    0x1f, 0x1f, 0x21, 0x21, 0x21, 0x22, 0x22, 0x22, 0x24, 0x24, 0x24, 0x26,
    0x26, 0x26, 0x28, 0x28, 0x28, 0x29, 0x29, 0x29, 0x2b, 0x2b, 0x2b, 0x2e,
    0x2e, 0x2e, 0x31, 0x31, 0x31, 0x34, 0x34, 0x34, 0x36, 0x36, 0x36, 0x39,
    0x39, 0x39, 0x3c, 0x3c, 0x3c, 0x3f, 0x00, 0x00, 0x3b, 0x00, 0x00, 0x38,
    0x00, 0x00, 0x35, 0x00, 0x00, 0x32, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x2c,
    0x00, 0x00, 0x29, 0x00, 0x00, 0x26, 0x00, 0x00, 0x22, 0x00, 0x00, 0x1f,
    0x00, 0x00, 0x1c, 0x00, 0x00, 0x19, 0x00, 0x00, 0x16, 0x00, 0x00, 0x13,
    0x00, 0x00, 0x10, 0x00, 0x00, 0x3f, 0x36, 0x36, 0x3f, 0x2e, 0x2e, 0x3f,
    0x27, 0x27, 0x3f, 0x1f, 0x1f, 0x3f, 0x17, 0x17, 0x3f, 0x10, 0x10, 0x3f,
    0x08, 0x08, 0x3f, 0x00, 0x00, 0x11, 0x0b, 0x06, 0x13, 0x0d, 0x07, 0x15,
    0x0f, 0x09, 0x17, 0x11, 0x0a, 0x18, 0x13, 0x0b, 0x1a, 0x15, 0x0c, 0x1c,
    0x17, 0x0e, 0x1e, 0x19, 0x0f, 0x3f, 0x3f, 0x36, 0x3f, 0x3f, 0x2e, 0x3f,
    0x3f, 0x27, 0x3f, 0x3f, 0x1f, 0x3f, 0x3e, 0x17, 0x3f, 0x3d, 0x10, 0x3f,
    0x3d, 0x08, 0x3f, 0x3d, 0x00, 0x39, 0x36, 0x00, 0x33, 0x31, 0x00, 0x2d,
    0x2b, 0x00, 0x27, 0x27, 0x00, 0x21, 0x21, 0x00, 0x1c, 0x1b, 0x00, 0x16,
    0x15, 0x00, 0x10, 0x10, 0x00, 0x34, 0x3f, 0x17, 0x31, 0x3f, 0x10, 0x2d,
    0x3f, 0x08, 0x28, 0x3f, 0x00, 0x24, 0x39, 0x00, 0x20, 0x33, 0x00, 0x1d,
    0x2d, 0x00, 0x18, 0x27, 0x00, 0x36, 0x3f, 0x36, 0x2e, 0x3d, 0x2e, 0x27,
    0x3b, 0x27, 0x1f, 0x39, 0x1f, 0x17, 0x36, 0x17, 0x0f, 0x34, 0x0f, 0x08,
    0x32, 0x08, 0x00, 0x30, 0x00, 0x00, 0x2d, 0x00, 0x00, 0x2b, 0x00, 0x00,
    0x28, 0x00, 0x00, 0x26, 0x00, 0x00, 0x23, 0x00, 0x00, 0x21, 0x00, 0x00,
    0x1e, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x19, 0x00, 0x00, 0x17, 0x00, 0x00,
    0x14, 0x00, 0x00, 0x12, 0x00, 0x00, 0x0f, 0x00, 0x02, 0x0d, 0x02, 0x03,
    0x0a, 0x03, 0x05, 0x08, 0x05, 0x36, 0x3f, 0x3f, 0x2e, 0x3f, 0x3f, 0x27,
    0x3f, 0x3f, 0x1f, 0x3f, 0x3f, 0x17, 0x3f, 0x3f, 0x0f, 0x3f, 0x3f, 0x08,
    0x3f, 0x3f, 0x00, 0x3f, 0x3f, 0x00, 0x39, 0x39, 0x00, 0x33, 0x33, 0x00,
    0x2d, 0x2d, 0x00, 0x27, 0x27, 0x00, 0x22, 0x22, 0x00, 0x1c, 0x1c, 0x00,
    0x16, 0x16, 0x00, 0x10, 0x10, 0x17, 0x2f, 0x3f, 0x10, 0x2c, 0x3f, 0x08,
    0x2a, 0x3f, 0x00, 0x27, 0x3f, 0x00, 0x23, 0x39, 0x00, 0x1f, 0x33, 0x00,
    0x1b, 0x2d, 0x00, 0x17, 0x27, 0x36, 0x36, 0x3f, 0x2e, 0x2f, 0x3f, 0x27,
    0x27, 0x3f, 0x1f, 0x20, 0x3f, 0x17, 0x18, 0x3f, 0x10, 0x10, 0x3f, 0x08,
    0x09, 0x3f, 0x00, 0x01, 0x3f, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x3b, 0x00,
    0x00, 0x38, 0x00, 0x00, 0x35, 0x00, 0x00, 0x32, 0x00, 0x00, 0x2f, 0x00,
    0x00, 0x2c, 0x00, 0x00, 0x29, 0x00, 0x00, 0x26, 0x00, 0x00, 0x22, 0x00,
    0x00, 0x1f, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x19, 0x00, 0x00, 0x16, 0x00,
    0x00, 0x13, 0x00, 0x00, 0x10, 0x0d, 0x08, 0x00, 0x0f, 0x09, 0x00, 0x12,
    0x0a, 0x00, 0x14, 0x0b, 0x00, 0x16, 0x0c, 0x00, 0x19, 0x0d, 0x00, 0x1b,
    0x0e, 0x00, 0x1e, 0x0f, 0x00, 0x20, 0x10, 0x00, 0x22, 0x11, 0x00, 0x25,
    0x12, 0x00, 0x28, 0x15, 0x03, 0x2c, 0x18, 0x06, 0x2f, 0x1b, 0x09, 0x32,
    0x1e, 0x0c, 0x35, 0x21, 0x0e, 0x39, 0x24, 0x11, 0x3c, 0x27, 0x14, 0x3f,
    0x2a, 0x17, 0x3f, 0x2e, 0x1c, 0x3f, 0x31, 0x22, 0x3f, 0x35, 0x27, 0x3f,
    0x38, 0x2c, 0x22, 0x1c, 0x12, 0x25, 0x1f, 0x14, 0x29, 0x22, 0x17, 0x2c,
    0x25, 0x19, 0x2f, 0x28, 0x1c, 0x32, 0x2a, 0x1e, 0x36, 0x2d, 0x20, 0x39,
    0x30, 0x23, 0x3c, 0x33, 0x25, 0x3f, 0x3a, 0x37, 0x3f, 0x38, 0x34, 0x3f,
    0x36, 0x31, 0x3f, 0x35, 0x2f, 0x3f, 0x33, 0x2c, 0x3f, 0x31, 0x29, 0x3f,
    0x2f, 0x27, 0x3f, 0x2e, 0x24, 0x3f, 0x2c, 0x20, 0x3f, 0x29, 0x1c, 0x3f,
    0x27, 0x18, 0x3c, 0x25, 0x17, 0x3a, 0x23, 0x16, 0x37, 0x22, 0x15, 0x34,
    0x20, 0x14, 0x32, 0x1f, 0x13, 0x2f, 0x1e, 0x12, 0x2d, 0x1c, 0x11, 0x2a,
    0x1a, 0x10, 0x28, 0x19, 0x0f, 0x27, 0x18, 0x0e, 0x24, 0x17, 0x0d, 0x22,
    0x16, 0x0c, 0x20, 0x14, 0x0b, 0x1d, 0x13, 0x0a, 0x1b, 0x12, 0x09, 0x17,
    0x10, 0x08, 0x15, 0x0f, 0x07, 0x12, 0x0e, 0x06, 0x10, 0x0c, 0x06, 0x0e,
    0x0b, 0x05, 0x0a, 0x08, 0x03, 0x3f, 0x00, 0x00, 0x3f, 0x04, 0x00, 0x3f,
    0x08, 0x00, 0x3f, 0x0d, 0x00, 0x3f, 0x11, 0x00, 0x3f, 0x15, 0x00, 0x3f,
    0x19, 0x00, 0x3f, 0x1d, 0x00, 0x3f, 0x22, 0x00, 0x3f, 0x26, 0x00, 0x3f,
    0x2a, 0x00, 0x3f, 0x2e, 0x00, 0x3f, 0x32, 0x00, 0x3f, 0x37, 0x00, 0x3f,
    0x3b, 0x00, 0x3f, 0x3f, 0x00, 0x3f, 0x2f, 0x00, 0x36, 0x28, 0x00, 0x2d,
    0x22, 0x00, 0x24, 0x1b, 0x00, 0x1b, 0x14, 0x00, 0x12, 0x0d, 0x00, 0x09,
    0x07, 0x00, 0x00, 0x00, 0x00, 0x29, 0x00, 0x28, 0x23, 0x00, 0x2b, 0x1d,
    0x00, 0x2f, 0x17, 0x00, 0x32, 0x12, 0x00, 0x35, 0x0c, 0x00, 0x38, 0x06,
    0x00, 0x3c, 0x3f, 0x3f, 0x3f, 0x3f};  // gah!

int hicolour = 0;            // bleh --tSB
bool fullscreenmode = true;  // ditto

char *strbuf = 0;  // Universal temporary string buffer. :)

char joyflag = 0;

int vidxres = 0;
int vidyres = 0;

char nocdaudio = 0;  // do not use CD audio

string_k startmap;  // start map

// ================================= Code ====================================

void V_memset(void *dest, int fill, int count) {
    int i;
    for (i = 0; i < count; i++) ((byte *)dest)[i] = (byte)fill;
}

void V_memcpy(void *dest, const void *src, int count) {
    int i;

    if ((((long)dest | (long)src | count) & 3) == 0) {
        count >>= 2;
        for (i = 0; i < count; i++) ((int *)dest)[i] = ((int *)src)[i];
    } else
        for (i = 0; i < count; i++) ((byte *)dest)[i] = ((byte *)src)[i];
}

int V_memcmp(const void *m1, const void *m2, int count) {
    while (count) {
        count--;
        if (((byte *)m1)[count] != ((byte *)m2)[count]) return -1;
    }
    return 0;
}

void V_strcpy(char *dest, const char *src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest++ = 0;
}

void V_strncpy(char *dest, const char *src, int count) {
    while (*src && count--) {
        *dest++ = *src++;
    }
    if (count) *dest++ = 0;
}

int V_strlen(const char *str) {
    int count;

    count = 0;
    while (str[count]) count++;

    return count;
}

void V_strcat(char *dest, char *src) {
    dest += V_strlen(dest);
    V_strcpy(dest, src);
}

int V_strcmp(const char *s1, const char *s2) {
    while (1) {
        if (*s1 < *s2) return -1;  // strings not equal
        if (*s1 > *s2) return +1;
        if (!*s1) return 0;  // strings are equal
        s1++;
        s2++;
    }

    // return 666;
}

int V_atoi(const char *str) {
    int val;
    int sign;
    int c;

    if (*str == '-') {
        sign = -1;
        str++;
    } else
        sign = 1;

    val = 0;

    //
    // check for hex
    //
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
        while (1) {
            c = *str++;
            if (c >= '0' && c <= '9')
                val = (val << 4) + c - '0';
            else if (c >= 'a' && c <= 'f')
                val = (val << 4) + c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                val = (val << 4) + c - 'A' + 10;
            else
                return val * sign;
        }
    }

    //
    // check for character
    //
    if (str[0] == '\'') {
        return sign * str[1];
    }

    //
    // assume decimal
    //
    while (1) {
        c = *str++;
        if (c < '0' || c > '9') return val * sign;
        val = val * 10 + c - '0';
    }

    // return 0;
}

float V_atof(const char *str) {
    float val;
    int sign;
    int c;
    int decimal, total;

    if (*str == '-') {
        sign = -1;
        str++;
    } else
        sign = 1;

    val = 0;

    //
    // check for hex
    //
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
        while (1) {
            c = *str++;
            if (c >= '0' && c <= '9')
                val = (val * 16) + c - '0';
            else if (c >= 'a' && c <= 'f')
                val = (val * 16) + c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                val = (val * 16) + c - 'A' + 10;
            else
                return val * sign;
        }
    }

    //
    // check for character
    //
    if (str[0] == '\'') {
        return (float)(sign * str[1]);
    }

    //
    // assume decimal
    //
    decimal = -1;
    total = 0;
    while (1) {
        c = *str++;
        if (c == '.') {
            decimal = total;
            continue;
        }
        if (c < '0' || c > '9') break;
        val = val * 10 + c - '0';
        total++;
    }

    if (decimal == -1) return val * sign;
    while (total > decimal) {
        val /= 10;
        total--;
    }

    return val * sign;
}

char *va(char *format, ...) {
    va_list argptr;
    static char string[1024];

    va_start(argptr, format);
    vsprintf(string, format, argptr);
    va_end(argptr);

    return string;
}

void Sys_Error(const char *format, ...) {
    va_list argptr;
    static char string[1024];

    va_start(argptr, format);
    vsprintf(string, format, argptr);
    va_end(argptr);

    // <aen> why can't I nest another va() in here?
    Logp("Sys: Exiting with message: ");
    Log(string);

    ShutdownTimer();
    input.ShutDown();
    gfx.ShutDown();
    ShutdownMusicSystem();
    // CD_Deinit();
    /*MD_PlayStop();
    MD_Exit();*/
    // sound.init();

    /*if (ShutdownVideo)
		ShutdownVideo(1);*/  // grdriver's destructor handles this for us

    DestroyWindow(hWnd);  // is this necessary? --tSB

    if (strcmp("", string)) MessageBox(NULL, string, "", 0);
    fflush(stdout);

    FreeAllMemory();
    PostQuitMessage(0);

    exit(-1);
}

int sgn(int x) {
    if (x > 0)
        return 1;
    else if (x < 0)
        return -1;
    return 0;
}

void InitializeDefaults() {
    /*	kb1=K_ENTER;                         // default keyboard controls
	kb2=K_ALT;
	kb3=K_ESC;
	kb4=' ';

	jb1=1;                               // joystick aliases
	jb2=2;
	jb3=3;
	jb4=4;
	joyflag=0;       */  // joystick
    // defaults
    // to
    // disabled

    vidxres = 320;  // default res is 320x240
    vidyres = 240;

    logoutput = 0;  // Don't be annoyingly verbose

    V_memset(bindarray, 0, sizeof(bindarray));  // clear this here so we don't
                                                // trigger random events on the
                                                // first keypress

    strbuf = (char *)valloc(2000, "strbuf", OID_TEMP);  // globally used string;
                                                        // TODO: remove all
                                                        // dependencies on this,
                                                        // and get rid of it

    startmap = "";  // test.map";            // default startup map

    gfx.VSync(false);  // no vsync by default

    // default MikMod settings
    /*	md_mode		=DMODE_STEREO|DMODE_16BITS|DMODE_INTERP;
            md_mixfreq	=22050;*/

    // prep the joystick for use
    // Calibrate();
}

// <aen, apr 21>
// + added these few static routines and made ParseStartupFiles() use'em

static VFILE *user_cfg_file = 0;
static char parse_str[256];

static char *parse_cfg_token() {
    vscanf(user_cfg_file, "%s", parse_str);
    return parse_str;
}

// compares string against parse_str (grabbed by parse_cfg_token())
// 0=mismatch, 1=match
static int parse_match(char *str) { return !strcmp(parse_str, str); }

// zero error correcting or detection; fix <aen, apr 21>
void ParseStartupFiles() {
    user_cfg_file = vopen("user.cfg");
    if (!user_cfg_file) {
        return;
    }

    while (1) {
        parse_cfg_token();

        // mounts a pack file; up to 3? (perhaps gaurd against more?)
        if (parse_match("mount")) {
            MountVFile(parse_cfg_token());
            continue;
        }
        // set video resolution
        else if (parse_match("vidmode")) {
            vidxres = V_atoi(parse_cfg_token());
            vidyres = V_atoi(parse_cfg_token());

            continue;
        }
        // log to VERGE.LOG
        else if (parse_match("log")) {
            logoutput = 1;
            continue;
        }
        // disable CD playing
        /*    else if (parse_match("nocdaudio"))
              { nocdaudio=1; continue; }*/
        // map VERGE.EXE will run first when executed
        else if (parse_match("startmap")) {
            startmap = parse_cfg_token();
            continue;
        }  // --tSB why not?
        // 0=auto detect, 1=???, 2=???, 3=nosound
        else if (parse_match("sound_device")) {
            parse_cfg_token();
            //    	md_device = (UWORD)V_atoi(parse_cfg_token());
            continue;
        }
        // sound lib setting
        else if (parse_match("mixrate")) {
            sfx_mixrate = (word)V_atoi(parse_cfg_token());
            continue;
        }
        // sound lib setting
        else if (parse_match("safemode")) {
            sfx_safemode = 1;
            continue;
        } else if (parse_match("dmabufsize")) {
            parse_cfg_token();
            continue;
        }
        // sound lib setting
        else if (parse_match("force8bit")) {
            continue;
        }
        // sound lib setting
        else if (parse_match("forcemono")) {
            continue;
        } else if (parse_match("hicolor")) {
            hicolour = 1;
            continue;
        } else if (parse_match("window")) {
            fullscreenmode = 0;
            continue;
        } else if (parse_match("vsync")) {
            gfx.VSync(true);
            continue;
        }

        // unknown command, assume end
        break;
    }

    // done parsing config
    vclose(user_cfg_file);
}

void ParseAutoCFG() {
    VFILE *vf;
    int i;

    vf = vopen("auto.cfg");
    if (!vf) return;

    while (1) {
        char temp[256 + 1] = {0};
        vgets(temp, 256, vf);
        temp[256] = '\0';

        for (i = 0; i < V_strlen(temp); i++) {
            if (temp[i] == 10 || temp[i] == 13) temp[i] = 0;
        }
        if (V_strlen(temp) < 2) {
            break;
        }
        // cmd=temp;
        Console_SendCommand(temp);
    }
    vclose(vf);
}

// --tSB moved Conlib_Ver() to conlib.h (that's the only place it's needed
// anyway :P

int CheckMessages() {
    // Win95 can bite me.
    // mehehehe --tSB
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) || !bActive) {
        if (msg.message == WM_QUIT) return msg.wParam;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

void InitSystems(HINSTANCE hInst) {
    InitLog();

    Log("v2.6 startup. Logfile initialized.");

    Logp("Sys: Initializing keyboard handler.");
    if (!input.Init(hInst, hWnd))
        Sys_Error("Error initializing keyboard handler");
    memset(bindarray, 0, 256);  // no keys bound yet
    input.ClipMouse(0, 0, vidxres, vidyres);
    LogDone();

    Logp("Sys: Initializing timer. Set 100hz.");
    if (!InitTimer()) Sys_Error("Error initing timer");
    LogDone();

    /*
    Logp("Sys: Initializing CD Audio.");
    CD_Init();
    LogDone();
    */

    Logp("Sys: Initializing music system.");
    InitMusicSystem((unsigned int)hWnd);
    LogDone();

    Logp("Sys: Initializing graphics.");
    if (!gfx.Init(hWnd, vidxres, vidyres, hicolour ? 16 : 8, fullscreenmode))
        Sys_Error("Error initizlizing graphics");
    if (!gfx.SetPalette(vergepal)) Sys_Error("Error setting the palette");
    LogDone();
}

LRESULT CALLBACK MainWinProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    // Mouse management
    // General Win32 type crap
    case WM_KEYDOWN:
        input.Poll();
    case WM_KEYUP:
        input.Poll();
    case WM_ACTIVATE:
        bActive = LOWORD(wParam) !=
                  WA_INACTIVE;  // I know it's ugly.  Sue me, I'm lazy.
        if (bActive) {
            SoundResume();
        } else {
            SoundPause();
        }
        return 0;
    /*
       case WM_ACTIVATEAPP: bActive=wParam!=0?true:false;
                            if (bActive)
                             {
                              SoundResume();
                             }
                            else
                             {
                              SoundPause();
                             }
                            InvalidateRect(hWnd,NULL,true);
                            return 0;*/

    case WM_CLOSE:
        Sys_Error("");
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//#include "dpmi.h"
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    WNDCLASSEX wc;  // Win32. blarg
                    /*	if (1 == argc) {
                                    printf("VERGE v.%s build %s at %s.
                       \n",VERSION,__DATE__,__TIME__);
                                    printf("Copyright (C)1998 vecna \n");
                                    delay(500);
                            }*/
    InitLog();

    srand(timeGetTime());
    // ---Directly from the DOS version -- used to be in VERGE.CPP but it fits
    // here better. --tSB
    InitializeDefaults();
    ParseStartupFiles();

    // set up and register window class
    ZeroMemory(&wc, sizeof wc);
    wc.cbSize = sizeof wc;
    wc.style = 0;
    wc.lpfnWndProc = MainWinProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = LoadIcon(hInst, "AppIcon");
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = NAME;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    RegisterClassEx(&wc);

    // Create it!
    hWnd = CreateWindowEx(0, NAME, TITLE,
        0,  // WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        HWND_DESKTOP, NULL, hInst, NULL);
    ShowWindow(hWnd, nShow);
    SetFocus(hWnd);
    //    ShowCursor(false);

    InitSystems(hInst);

    bActive = true;

    return VMain();
}
