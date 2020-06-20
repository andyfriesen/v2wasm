/*
VERGE 2.5+j (AKA V2k+j) -  A video game creation engine
Copyright (C) 1998-2000  Benjamin Eirich (AKA vecna), et al
Please see authors.txt for a complete list of contributing authors.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// ���������������������������������������������������������������������Ŀ
// �                  The VergeC Compiler version 2.01                   �
// �              Copyright (C)1998 BJ Eirich (aka vecna)                �
// �                           Lexical Parser                            �
// �����������������������������������������������������������������������

#include "compile.h"
#include "str.h"
#include "vcc.h"
#include "vccode.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexical.h"

// *****
#define NEW_LOCALS
// *****

// ================================= Data ====================================

// Character types

#define LETTER 1
#define DIGIT 2
#define SPECIAL 3

// ---------------

token_t tok;
// char	token[2000]				="";	// Token buffer
// unsigned int	token_nvalue	=0;		// int value of token if it's
// type DIGIT
// char	token_type				=0;		// type of
// current
// token.
// char	token_subtype			=0;		// This is just crap.
unsigned char chr_table[256] = ""; // Character type table.

int lines = 0;
int tlines = 0;        // current line number being processed in
char* source_file = 0; // the current source file

// ----------------

const char* hardfuncs[] = {
    // A
    "exit", "message", "malloc", "free", "pow", "loadimage", "copysprite",
    "tcopysprite", "render", "showpage", "entityspawn", "setplayer", "map",
    "loadfont", "playfli",

    // B
    "gotoxy", "printstring", "loadraw", "settile", "allowconsole",
    "scalesprite", "processentities", "updatecontrols", "unpress", "entitymove",
    "hline", "vline", "line", "circle", "circlefill", // 30

    // C
    "rect", "rectfill", "strlen", "strcmp", "cd_stop", "cd_play", "fontwidth",
    "fontheight", "setpixel", "getpixel", "entityonscreen", "random", "gettile",
    "hookretrace", "hooktimer",

    // D
    "setresolution", "setrstring", "setcliprect", "setrenderdest",
    "restorerendersettings", "partymove", "sin", "cos", "tan", "readmouse",
    "setclip", "setlucent", "wrapblit", "twrapblit", "setmousepos", // 60

    // E
    "hookkey", "playmusic", "stopmusic", "palettemorph", "fopen", "fclose",
    "quickread", "addfollower", "killfollower", "killallfollowers",
    "resetfollowers", "flatpoly", "tmappoly", "cachesound", "freeallsounds",

    // F
    "playsound", "rotscale", "mapline", "tmapline", "val", "tscalesprite",
    "grabregion", "log", "fseekline", "fseekpos", "fread", "fgetbyte",
    "fgetword", "fgetquad", "fgetline", // 90

    // G
    "fgettoken", "fwritestring", "fwrite", "frename", "fdelete", "fwopen",
    "fwclose", "memcpy", "memset", "silhouette", "initmosaictable", "mosaic",
    "writevars", "readvars", "callevent", // 105

    // H
    "asc", "callscript", "numforscript", "filesize", "ftell", "changechr",
    "rgb", "getr", "getg", "getb", "mask", "changeall", "sqrt", "fwritebyte",
    "fwriteword", // 120

    // I
    "fwritequad", "calclucent", "imagesize"};

char returntypes[] = {
    // A
    0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, // 15

    // B
    0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 30

    // C
    0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, // 45

    // D
    0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, // 60

    // E
    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, // 75

    // F
    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, // 90

    // G
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, // 105

    // H
    1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, // 120

    // I
    0, 0, 1};

const char* hardvar0[] = {
    "xwin", "ywin", "cameratracking", "timer", "up", "down", "left", "right",
    "b1", "b2", "b3", "b4", "screenx", "screeny",
    "player", // 15

    "numentsonscreen", "tracker", "mx", "my", "mb", "vctrace", "image_width",
    "image_height", "music_volume", "vsp", "lastent", "last_pressed",
    "map_width", "map_height", "vsync", "numents", "mask_color",
    "bitdepth" // 30
};

const char* hardvar1[] = {
    "screen", "entity.x", "entity.y", "entity.tx", "entity.ty", "entity.facing",
    "entity.moving", "entity.specframe", "entity.speed", "entity.movecode",
    "entsonscreen", "key", "layer.hline", "byte",
    "word", // 15

    "quad", "pal", "sbyte", "sword", "squad", "entity.isob", "entity.canob",
    "entity.autoface", "entity.visible", "entity.on", "chr_data",
    "entity.width", "entity.height", "entity.chrindex",
    "zone.event", // 30

    "zone.chance",
};

// int		funcidx			= 0;
// int		varcategory		= 0;

int numhardfuncs = 123;
int numhardvar0 = 33;
int numhardvar1 = 31;

// ================================= Code ====================================

int streq(const char* a, const char* b) {
    return 0 == strcmp(a, b);
}

char TokenIs(const char* str) {
    return (char)streq(str, tok.ident); // token);
}

void ParseWhitespace(void) {
skipwhite:
    while (*src && *src <= ' ' && *src > 2) {
        if (*src == '\n') {
            ++tlines;
        }

        src++;
    }

    /*
    if (src[0]=='/' && src[1]=='/')
    {
            while (*src && (*src != '\n'))
            {
                    src++;
            }
            continue;
    }

    if (src[0]=='/' && src[1]=='*')
    {
            while (!(src[0]=='*' && src[1]=='/'))
            {
                    src++;
                    if (!*src)
                    {
                            return;
                    }
                    else if (1 == *src)
                    {
                            src++;
                            source_file = (char *)src;
                            while (*src++)
                                    ; // null
                            continue;
                    }
                    else if (2 == *src)
                    {
                            src++;
                            lines = (int) *(int *)src;
                            tlines+=lines;
                            src += 4;
                            continue;
                    }
            }
            src+=2;
            continue;
    }
    */

    if (1 == *src) {
        src++;
        source_file = (char*)src;
        while (*src++)
            ; // null
        goto skipwhite;
    }
    if (2 == *src) {
        src++;
        lines = (int)*(int*)src;
        src += 4;
        goto skipwhite;
    }
}

int GetVarIdx(int found_at) {
#ifdef NEW_LOCALS
    int n, relative;

    relative = 0;
    for (n = 0; n < found_at; n++) {
        if (INT == current_func->argtype[n]) {
            relative++;
        }
    }
    return relative;
#else
    return found_at;
#endif
}

int GetStringIdx(int found_at) {
    int n, relative;

    relative = 0;
    for (n = 0; n < found_at; n++) {
        if (STRING == current_func->argtype[n]) {
            relative++;
        }
    }
    return relative;
}

void CheckLibFunc() {
    int n;

    tok.value = 0;
    tok.type = 0;
    // token_nvalue=0;
    // token_type=0;

    if (TokenIs("if") || TokenIs("else") || TokenIs("for") ||
        TokenIs("while") || TokenIs("switch") || TokenIs("case") ||
        TokenIs("goto")) {
        tok.type = RESERVED;
        // token_type=RESERVED;
        return;
    }
    if (TokenIs("and")) {
        tok.type = CONTROL;
        tok.ident[0] = '&';
        tok.ident[1] = '&';
        tok.ident[2] = '\0';
        // token_type=CONTROL;
        // token[0]='&'; token[1]='&'; token[2]=0;
        return;
    }
    if (TokenIs("or")) {
        tok.type = CONTROL;
        tok.ident[0] = '|';
        tok.ident[1] = '|';
        tok.ident[2] = '\0';
        // token_type=CONTROL;
        // token[0]='|'; token[1]='|'; token[2]=0;
        return;
    }

    for (n = 0; n < numhardfuncs; n++) {
        if (!strcmp(hardfuncs[n], tok.ident)) // token))
        {
            // token_type=FUNCTION;
            // token_subtype=op_BFUNC;
            // funcidx=n;
            tok.type = FUNCTION;
            tok.subtype = op_BFUNC;
            tok.index = n;
            return;
        }
    }

    for (n = 0; n < numhardvar0; n++) {
        if (!strcmp(hardvar0[n], tok.ident)) // token))
        {
            // token_type=IDENTIFIER;
            // varcategory=op_HVAR0;
            // varidx=n;
            tok.type = IDENTIFIER;
            tok.subtype = op_HVAR0;
            tok.index = n;
            return;
        }
    }

    for (n = 0; n < numhardvar1; n++) {
        if (!strcmp(hardvar1[n], tok.ident)) // token))
        {
            // token_type=IDENTIFIER;
            // varcategory=op_HVAR1;
            // varidx=n;
            tok.type = IDENTIFIER;
            tok.subtype = op_HVAR1;
            tok.index = n;
            return;
        }
    }

    if (functionlist.number_nodes()) {
        n = 0;
        functionlist.go_head();
        find_fun = 0;
        do {
            function_t* f = (function_t*)functionlist.current();
            if (!strcmp(f->fname, tok.ident)) // token))
            {
                tok.type = FUNCTION;
                tok.subtype = op_UFUNC;
                tok.index = n;
                // token_type=FUNCTION;
                // token_subtype=op_UFUNC;
                // funcidx=n;
                find_fun = f;
                return;
            }

            functionlist.go_next();
            n++;
        } while (functionlist.current() != functionlist.head());
    }

    if (current_func) {
        for (n = 0; n < current_func->numlocals; n++) {
            if (!strcmp(larg[n], tok.ident)) // token))
            {
                // token_type=IDENTIFIER;
                // varidx=0;
                tok.type = IDENTIFIER;
                tok.index = 0;

                switch (current_func->argtype[n]) {
                case INT:
                    tok.subtype = op_LVAR;
                    tok.index = GetVarIdx(n);
                    // varcategory=op_LVAR;
                    // varidx=GetVarIdx(n);
                    break;
                case STRING:
                    tok.subtype = op_SLOCAL;
                    tok.index = GetStringIdx(n);
                    // varcategory=op_SLOCAL;
                    // varidx=GetStringIdx(n);
                    break;

                default:
                    vcerr("CheckLibFunc: invalid local type (weird): %d",
                        current_func->argtype[n]);
                }
                return;
            }
        }
    }

    if (varlist.number_nodes()) {
        n = 0;
        varlist.go_head();
        find_var = 0;
        do {
            variable_t* v = (variable_t*)varlist.current();
            if (!strcmp(v->vname, tok.ident)) // token))
            {
                tok.type = IDENTIFIER;
                tok.subtype = (v->arraylen > 1) ? op_UVARRAY : op_UVAR;
                tok.index = n;
                // token_type=IDENTIFIER;
                // varcategory=(v->arraylen>1) ? op_UVARRAY : op_UVAR;
                // varidx=n;
                find_var = v;
                return;
            }
            varlist.go_next();
            n++;
        } while (varlist.current() != varlist.head());
    }

    if (strlist.number_nodes()) {
        n = 0;
        strlist.go_head();
        find_str = 0;
        do {
            string_t* s = (string_t*)strlist.current();
            if (!strcmp(s->vname, tok.ident)) // token))
            {
                tok.type = IDENTIFIER;
                tok.subtype = (s->arraylen > 1) ? op_SARRAY : op_STRING;
                tok.index = n;
                // token_type=IDENTIFIER;
                // varcategory=(s->arraylen>1) ? op_SARRAY : op_STRING;
                // varidx=n;
                find_str = s;
                return;
            }
            strlist.go_next();
            n++;
        } while (strlist.current() != strlist.head());
    }
}

void GetIdentifier(void) {
    char* ptok;

    ptok = tok.ident; // token;
    while (*src && (LETTER == chr_table[*src] || DIGIT == chr_table[*src])) {
        *ptok++ = *src++;
    }
    *ptok = '\0';

    strlwr(tok.ident); // token);
    CheckLibFunc();
}

// expects head of actual number, with no leading $ or 0x notation.
int hextoi(char* num) {
    int v, i, n, l;

    l = strlen(num);
    if (l > 8) {
        vcerr("Hex number exceeds 8 digit max.");
    }

    strlwr(num);
    for (n = i = 0; i < l; i++) {
        v = num[i] - 48;
        if (v > 9) {
            v -= 39; // v-=7 for uppercase
            if (v > 15) {
                vcerr("Invalid hex number.");
            }
        }
        n <<= 4;
        n += v;
    }
    return n;
}

void DoTickMarks() {
    // token_nvalue=token[1];
    tok.value = tok.ident[1];
}

void GetNumber() {
    int i = 0;

    if ('\'' == *src) {
        tok.ident[0] = *src++;
        tok.ident[1] = *src++;
        tok.ident[2] = *src++;
        tok.ident[3] = '\0';
        // token[0]=*src++;
        // token[1]=*src++;
        // token[2]=*src++;
        // token[3]=0;
    } else {
        i = 0;
        while (chr_table[*src] != SPECIAL) {
            // token[i++]=*src++;
            tok.ident[i++] = *src++;
        }
        // token[i]=0;
        tok.ident[i] = '\0';
    }

    // if (token[0]=='$')
    if ('$' == tok.ident[0]) {
        // token_nvalue=hextoi(token+1);
        tok.value = hextoi(tok.ident + 1);
    }
    // else if (token[0]=='\'')
    else if ('\'' == tok.ident[0]) {
        DoTickMarks();
    } else {
        // token_nvalue=atoi(token);
        tok.value = atoi(tok.ident);
    }
}

void GetPunctuation() {
    char* ptok = 0;

    ptok = tok.ident; // token;

    switch (*src) {
    case '(':
        ptok[0] = '(';
        ptok[1] = 0;
        src++;
        break;
    case ')':
        ptok[0] = ')';
        ptok[1] = 0;
        src++;
        break;
    case '{':
        ptok[0] = '{';
        ptok[1] = 0;
        src++;
        break;
    case '}':
        ptok[0] = '}';
        ptok[1] = 0;
        src++;
        break;
    case '[':
        ptok[0] = '[';
        ptok[1] = 0;
        src++;
        break;
    case ']':
        ptok[0] = ']';
        ptok[1] = 0;
        src++;
        break;
    case ',':
        ptok[0] = ',';
        ptok[1] = 0;
        src++;
        break;
    case ':':
        ptok[0] = ':';
        ptok[1] = 0;
        src++;
        break;
    case ';':
        ptok[0] = ';';
        ptok[1] = 0;
        src++;
        break;
    case '/':
        ptok[0] = '/';
        ptok[1] = 0;
        src++;
        break;
    case '*':
        ptok[0] = '*';
        ptok[1] = 0;
        src++;
        break;
    case '^':
        ptok[0] = '^';
        ptok[1] = 0;
        src++;
        break;
    case '%':
        ptok[0] = '%';
        ptok[1] = 0;
        src++;
        break;
    case '\"':
        ptok[0] = '\"';
        ptok[1] = 0;
        src++;
        break;
    case '+':
        ptok[0] = '+';
        src++;
        if (*src == '+') {
            ptok[1] = '+';
            src++;
        } else if (*src == '=') {
            ptok[1] = '=';
            src++;
        } else {
            ptok[1] = 0;
        }
        ptok[2] = 0;
        break;
    case '-':
        ptok[0] = '-';
        src++;
        if (*src == '-') {
            ptok[1] = '-';
            src++;
        } else if (*src == '=') {
            ptok[1] = '=';
            src++;
        } else {
            ptok[1] = 0;
        }
        ptok[2] = 0;
        break;
    case '>':
        ptok[0] = '>';
        src++;
        if (*src == '=') {
            ptok[1] = '=';
            ptok[2] = 0;
            src++;
            break;
        }
        if (*src == '>') {
            ptok[1] = '>';
            ptok[2] = 0;
            src++;
            break;
        }
        ptok[1] = 0;
        break;
    case '<':
        ptok[0] = '<';
        src++;
        if (*src == '=') {
            ptok[1] = '=';
            ptok[2] = 0;
            src++;
            break;
        }
        if (*src == '<') {
            ptok[1] = '<';
            ptok[2] = 0;
            src++;
            break;
        }
        ptok[1] = 0;
        break;
    case '!':
        ptok[0] = '!';
        src++;
        if (*src == '=') {
            ptok[1] = '=';
            ptok[2] = 0;
            src++;
            break;
        }
        ptok[1] = 0;
        break;
    case '=':
        ptok[0] = '=';
        src++;
        if (*src == '=') {
            ptok[1] = 0;
            src++;
        } else {
            ptok[1] = 0;
        }
        break;
    case '&':
        ptok[0] = '&';
        src++;
        if (*src == '&') {
            ptok[1] = '&';
            ptok[2] = 0;
            src++;
            break;
        }
        ptok[1] = 0;
        break;
    case '|':
        ptok[0] = '|';
        src++;
        if (*src == '|') {
            ptok[1] = '|';
            ptok[2] = 0;
            src++;
            break;
        }
        ptok[1] = 0;
        break;

    default:
        src++; // This should be an error.
    }
}

// aen <4jan2000> whee!
void GetEmitStringLiteral() {
    Expect("\"");

    while ('\"' != *src) {
        if ('\n' == *src) {
            vcerr("Unterminated string found.");
        }
        *code++ = *src++;
    }
    src++; // whoops
    *code++ = '\0';
}

/*
void	GetString(void) {
        int i=0;

        // Expects a "quoted" string. Places the contents of the string in
        // token[] but does not include the quotes.

        Expect("\"");

        i=0;
        while (*src!='\"') {
                if ('\n'==*src) {
                        vcerr("Unterminated string found.");
                }
                token[i++]=*src++;
                if (i>250) {
                        vcerr("String exceeds 250 character maximum.");
                }
        }
        src++;
        token[i]=0;
}
*/

void GetToken(void) {
    // int		n	=0;
    // Simply reads in the next statement and places it in the
    // token buffer.

    ParseWhitespace();

    switch (chr_table[*src]) {
    case LETTER:
        // token_type = IDENTIFIER;
        tok.type = IDENTIFIER;
        GetIdentifier();
        break;
    case DIGIT:
        // token_type = DIGIT;
        tok.type = DIGIT;
        GetNumber();
        break;
    case SPECIAL:
        // token_type = CONTROL;
        tok.type = CONTROL;
        GetPunctuation();
        break;
    }

    if (!*src && inevent) {
        err("Unexpected end of file");
    }
}

void Expect(const char* str) {
    GetToken();
    if (TokenIs(str)) {
        return;
    }
    vcerr("error: %s expected, %s got", str, tok.ident); // token);
}

int ExpectNumber() {
    GetToken();
    if (tok.type != DIGIT) // token_type!=DIGIT)
    {
        err("error: Numerical value expected");
    }
    return tok.value; // token_nvalue;
}

void InitCompileSystem() {
    int i = 0;

    vprint("Building chr_table[].");

    for (i = 0; i < 256; i++)
        chr_table[i] = SPECIAL;
    for (i = '0'; i <= '9'; i++)
        chr_table[i] = DIGIT;
    for (i = 'A'; i <= 'Z'; i++)
        chr_table[i] = LETTER;
    for (i = 'a'; i <= 'z'; i++)
        chr_table[i] = LETTER;

    chr_table[10] = 0;
    chr_table[13] = 0;
    chr_table[' '] = 0;
    chr_table['_'] = LETTER;
    chr_table['.'] = LETTER;
    chr_table['$'] = DIGIT;
    chr_table[39] = DIGIT;
}

char lasttoken[40 + 1] = "";

// TODO: get rid of this. keep a list of last tokens read, maybe 5, and just
// jump around in
// that instead of doing this NextIs tapdancing. also, store token length for
// quicker rejection.
int NextIs(const char* str) {
    int match;
    token_t save;

    save = tok;
    save.ptr = (char*)src;

    GetToken();
    match = streq(str, tok.ident);

    tok = save;
    src = (unsigned char*)save.ptr;

    return match;

    /*
    char *ptr,tt,tst;
    int i,nv;

    ptr=src;
    tt=token_type;
    tst=token_subtype;
    nv=token_nvalue;
    memcpy(lasttoken, token, 2048);
    GetToken();
    src=ptr;
    token_nvalue=nv;
    tst=token_subtype;
    tt=token_type;
    //if (!strcmp(str,token)) i=1; else i=0;
    i=streq(str,token);
    memcpy(token, lasttoken, 2048);
    return i;
    */
}
