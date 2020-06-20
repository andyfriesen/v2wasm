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

#if !defined(__COMPILE_INC)
#define __COMPILE_INC

#include "linked.h"

#define VOID 1
#define INT 1
#define CHARPTR 2
#define STRING 3

extern char xvc_sig[9];

// event offset marker
struct funcmark : public linked_node {
    int mark;
    funcmark(int x) : mark(x) {}
    // void mark_the_spot(int x) { mark=x; }
    int compare(void* c);
};

struct function_t : public linked_node {
    char fname[40];
    char argtype[20];
    int numargs, numlocals;
    int returntype;
    int syscodeofs;

    function_t() : numargs(0), numlocals(0), returntype(0), syscodeofs(0) {}

    int compare(void* o);
    void write(FILE* fp);
    void read(FILE* fp);
};

struct variable_t : public linked_node {
    char vname[40];
    int varstartofs;
    int arraylen;

    variable_t() : varstartofs(0), arraylen(0) {}

    int compare(void* o);
    void write(FILE* fp);
    void read(FILE* fp);
};

struct string_t : public linked_node {
    char vname[40];
    int vsofs;
    int arraylen;

    string_t() : vsofs(0), arraylen(0) {}

    int compare(void* o);
    void write(FILE* fp);
    void read(FILE* fp);
};

extern function_t* find_fun;
extern variable_t* find_var;
extern string_t* find_str;

extern unsigned char *source, *src;
extern unsigned char *outbuf, *code;
extern unsigned char inevent;

extern function_t* current_func;

extern int curstartofs;
extern int sstartofs;

extern linked_list functionlist;
extern linked_list varlist;
extern linked_list strlist;

// extern int varidx;
// extern int funcidx;

extern int startsyscript;

#define MAX_LOCALS 20
extern char larg[MAX_LOCALS][40]; // was [12][40]. blah
// extern int c;

// extern char *functbl[512];
extern linked_list functbl;
// extern int mfuncs;

extern void CompileMAP(char* fname);
extern void CompileSystem();
extern void Expect(char* str);

extern void EmitC(char c);
extern void EmitW(short int w);
extern void EmitD(int w);
extern void EmitOperand();
extern void EmitString(char* str);
extern void HandleString();

#endif // __COMPILE_INC
