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
// �                       Code Generation module                        �
// �����������������������������������������������������������������������

#include <cassert>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compile.h"

#include "funclib.h"
#include "lexical.h"
#include "linked.h"
#include "preproc.h"
#include "vcc.h"
#include "vccode.h"

void LoadSource();
void AllocateWriteBuffer();

// DATA
// ///////////////////////////////////////////////////////////////////////////////////////////

#define LETTER 1
#define SPECIAL 3

// -- Function arguements type defs

#define VOID 1
#define INT 1
#define CHARPTR 2
#define STRING 3

unsigned int source_length = 0;

unsigned char *source, *src;
unsigned char *outbuf, *code;
unsigned char inevent = 0;

int curstartofs = 0;
int sstartofs = 0;

// int varidx	=0;
// int funcidx	=0;

int funcmark::compare(void* c) { return (((funcmark*)c)->mark > mark); }

int function_t::compare(void* o) {
    function_t* f = (function_t*)o;
    return strcmp(f->fname, fname);
}
void function_t::write(FILE* fp) {
    if (!fp)
        return;

    fwrite(fname, 1, 40, fp);
    fwrite(argtype, 1, 20, fp);
    fwrite(&numargs, 1, 4, fp);
    fwrite(&numlocals, 1, 4, fp);
    fwrite(&returntype, 1, 4, fp);
    fwrite(&syscodeofs, 1, 4, fp);
}
void function_t::read(FILE* fp) {
    if (!fp)
        return;

    fread(fname, 1, 40, fp);
    fread(argtype, 1, 20, fp);
    fread(&numargs, 1, 4, fp);
    fread(&numlocals, 1, 4, fp);
    fread(&returntype, 1, 4, fp);
    fread(&syscodeofs, 1, 4, fp);
}
linked_list functionlist;
function_t* current_func = 0;

int variable_t::compare(void* o) {
    variable_t* v = (variable_t*)o;
    return strcmp(v->vname, vname);
}
void variable_t::write(FILE* fp) {
    if (!fp)
        return;

    fwrite(vname, 1, 40, fp);
    fwrite(&varstartofs, 1, 4, fp);
    fwrite(&arraylen, 1, 4, fp);
}
void variable_t::read(FILE* fp) {
    if (!fp)
        return;

    fread(vname, 1, 40, fp);
    fread(&varstartofs, 1, 4, fp);
    fread(&arraylen, 1, 4, fp);
}
linked_list varlist;

int string_t::compare(void* o) {
    string_t* s = (string_t*)o;
    return strcmp(s->vname, vname);
}
void string_t::write(FILE* fp) {
    if (!fp)
        return;

    fwrite(vname, 1, 40, fp);
    fwrite(&vsofs, 1, 4, fp);
    fwrite(&arraylen, 1, 4, fp);
}
void string_t::read(FILE* fp) {
    if (!fp)
        return;

    fread(vname, 1, 40, fp);
    fread(&vsofs, 1, 4, fp);
    fread(&arraylen, 1, 4, fp);
}
linked_list strlist;

function_t* find_fun;
variable_t* find_var;
string_t* find_str;

int vctype; // 0 / 1 :: map / system

// -- local function parameters --

char larg[MAX_LOCALS][40];

// -- MAP vc stuff --

linked_list functbl;

// CODE
// ///////////////////////////////////////////////////////////////////////////////////////////

void HandleString();
void HandleFunction();
void HandleUserFunction();
void HandleFunctionType();
void EmitOperand();

void EmitC(char b) {
    if (locate && locate == (int)(code - outbuf))
        vcerr("Located.");

    if (code < 1024 * 1024 + outbuf) {
        *code++ = b;

    } else {
        err("EmitC: out of bounds");
    }
}

void EmitW(short w) {
    if (locate && locate == (int)(code - outbuf))
        vcerr("Located.");

    if (code + 2 - 1 < 1024 * 1024 + outbuf) {
        code += 2;
        *(short*)(code - 2) = w;
    } else {
        err("EmitW: out of bounds");
    }
}

void EmitD(int d) {
    if (locate && locate == (int)(code - outbuf))
        vcerr("Located.");

    if (code + 4 - 1 < 1024 * 1024 + outbuf) {
        code += 4;
        *(long*)(code - 4) = d;
    } else {
        err("EmitD: out of bounds");
    }
}

void EmitD(int64_t i) {
    assert(i <= 0xFFFFFFFF);
    EmitD((int)i);
}

void EmitString(char* str) {
    while (*str) {
        *code++ = *str++;
    }
    *code++ = '\0';
}

void HandleStringOperand() {
    if (NextIs("\"")) {
        EmitC(s_IMMEDIATE);
        GetEmitStringLiteral(); // aen
        return;
    }

    GetToken();

    // if (token_type==IDENTIFIER && varcategory==op_STRING)
    if (IDENTIFIER == tok.type && op_STRING == tok.subtype) {
        EmitC(s_GLOBAL);
        EmitW((short)find_str->vsofs);
        return;
    }
    // if (token_type==IDENTIFIER && varcategory==op_SARRAY)
    if (IDENTIFIER == tok.type && op_SARRAY == tok.subtype) {
        EmitC(s_ARRAY);
        EmitW((short)find_str->vsofs);
        Expect("[");
        EmitOperand();
        Expect("]");
        return;
    }
    // if (token_type==IDENTIFIER && varcategory==op_SLOCAL)
    if (IDENTIFIER == tok.type && op_SLOCAL == tok.subtype) {
        EmitC(s_LOCAL);
        // EmitC((char) varidx);
        EmitC((char)tok.index);
        return;
    }
    /* ----- */
    if (FUNCTION == tok.type && op_UFUNC == tok.subtype) {
        if (find_fun->returntype != 2) {
            vcerr("%s() does not return a string.", tok.ident);
        }
        EmitC(s_UFUNC);
        HandleUserFunction();
        return;
    }
    /* ----- */
    if (TokenIs("str")) {
        EmitC(s_NUMSTR);
        Expect("(");
        EmitOperand();
        Expect(")");
        return;
    }
    if (TokenIs("left")) {
        EmitC(s_LEFT);
        Expect("(");
        HandleString();
        Expect(",");
        EmitOperand();
        Expect(")");
        return;
    }
    if (TokenIs("right")) {
        EmitC(s_RIGHT);
        Expect("(");
        HandleString();
        Expect(",");
        EmitOperand();
        Expect(")");
        return;
    }
    if (TokenIs("mid")) {
        EmitC(s_MID);
        Expect("(");
        HandleString();
        Expect(",");
        EmitOperand();
        Expect(",");
        EmitOperand();
        Expect(")");
        return;
    }
    if (TokenIs("chr")) {
        EmitC(s_CHR);
        Expect("(");
        EmitOperand();
        Expect(")");
        return;
    }

    vcerr("Unknown string operand. ");
}

void HandleString() {
    while (1) {
        HandleStringOperand();

        if (NextIs("+")) {
            EmitC(s_ADD);
            GetToken();
            continue;
        } else {
            EmitC(s_END);
            break;
        }
    }
}

void EmitOperand();
void DoSomething();

void HandleOperand() {
    GetToken();

    /*
    if (TokenIs("-") || TokenIs("+"))
    {
            EmitC(op_IMMEDIATE);
            EmitD(0);
            if (TokenIs("-"))
                    EmitC(op_SUB);
            else
                    EmitC(op_ADD);
            GetToken();
            return;
    }
    */
    // if (token_type==DIGIT)
    if (DIGIT == tok.type) {
        EmitC(op_IMMEDIATE);
        // EmitD(token_nvalue);
        EmitD(tok.value);
        return;
    }
    // if (token_type==IDENTIFIER)
    if (IDENTIFIER == tok.type) {
        // if (varcategory==op_UVAR)
        if (op_UVAR == tok.subtype) {
            EmitC(op_UVAR);
            EmitD(find_var->varstartofs);
            return;
        }
        // if (varcategory==op_UVARRAY)
        if (op_UVARRAY == tok.subtype) {
            EmitC(op_UVARRAY);
            EmitD(find_var->varstartofs);
            Expect("[");
            EmitOperand();
            Expect("]");
            return;
        }
        // if (varcategory==op_LVAR)
        if (op_LVAR == tok.subtype) {
            EmitC(op_LVAR);
            // EmitC((char) varidx);
            EmitC((char)tok.index);
            return;
        }
        // if (varcategory==op_HVAR0)
        if (op_HVAR0 == tok.subtype) {
            EmitC(op_HVAR0);
            // EmitC((char) varidx);
            EmitC((char)tok.index);
            return;
        }
        // if (varcategory==op_HVAR1)
        if (op_HVAR1 == tok.subtype) {
            EmitC(op_HVAR1);
            // EmitC((char) varidx);
            EmitC((char)tok.index);
            Expect("[");
            EmitOperand();
            Expect("]");
            return;
        }
    }
    // if (token_type==FUNCTION && token_subtype==op_BFUNC)
    if (FUNCTION == tok.type && op_BFUNC == tok.subtype) {
        // if (!returntypes[funcidx])
        if (returntypes[tok.index] != 1) {
            vcerr("%s() does not return an integer.", tok.ident); // token);
        }
        EmitC(op_BFUNC);
        HandleFunction();
        return;
    }
    // if (token_type==FUNCTION && token_subtype==op_UFUNC)
    if (FUNCTION == tok.type && op_UFUNC == tok.subtype) {
        if (find_fun->returntype != 1) {
            vcerr("%s() does not return an integer.", tok.ident); // token);
        }
        EmitC(op_UFUNC);
        HandleUserFunction();
        return;
    }
    vcerr(va("Unknown token <%s>.", tok.ident)); // token));
}

void EmitOperand() {
    // Modifier-process loop.
    while (1) {
        if (NextIs("(")) {
            EmitC(op_GROUP);
            GetToken();
            EmitOperand();
            Expect(")");
        } else if (NextIs("+") || NextIs("-")) {
            EmitC(op_GROUP);

            EmitC(op_IMMEDIATE);
            EmitD(0);
            if (NextIs("+"))
                EmitC(op_ADD);
            else
                EmitC(op_SUB);
            GetToken();
            EmitOperand();

            // EmitC(op_END);
            // continue;
        } else {
            HandleOperand();
        }

        if (NextIs("+")) {
            EmitC(op_ADD);
            GetToken();
            continue;
        } else if (NextIs("-")) {
            EmitC(op_SUB);
            GetToken();
            continue;
        } else if (NextIs("/")) {
            EmitC(op_DIV);
            GetToken();
            continue;
        } else if (NextIs("*")) {
            EmitC(op_MULT);
            GetToken();
            continue;
        } else if (NextIs("%")) {
            EmitC(op_MOD);
            GetToken();
            continue;
        } else if (NextIs(">>")) {
            EmitC(op_SHR);
            GetToken();
            continue;
        } else if (NextIs("<<")) {
            EmitC(op_SHL);
            GetToken();
            continue;
        } else if (NextIs("&")) {
            EmitC(op_AND);
            GetToken();
            continue;
        } else if (NextIs("|")) {
            EmitC(op_OR);
            GetToken();
            continue;
        } else if (NextIs("^")) {
            EmitC(op_XOR);
            GetToken();
            continue;
        } else {
            // PutbackToken();

            EmitC(op_END);
            break;
        }
    }
}

void HandleFunction() {
    // switch (funcidx)
    switch (tok.index) {
    case 0:
        vcfunc_Exit();
        break;
    case 1:
        Message();
        break;
    case 2:
        GenericFunc(3, 1);
        break;
    case 3:
        GenericFunc(4, 1);
        break;
    case 4:
        GenericFunc(5, 2);
        break;
    case 5:
        vc_loadimage();
        break;
    case 6:
        GenericFunc(7, 5);
        break;
    case 7:
        GenericFunc(8, 5);
        break;
    case 8:
        GenericFunc(9, 0);
        break;
    case 9:
        GenericFunc(10, 0);
        break;
    case 10:
        vc_AllocateEntity();
        break;
    case 11:
        GenericFunc(12, 1);
        break;
    case 12:
        vc_Map();
        break;
    case 13:
        vc_LoadFont();
        break;
    case 14:
        vc_PlayFLI();
        break;
    case 15:
        GenericFunc(16, 2);
        break;
    case 16:
        vc_PrintString();
        break;
    case 17:
        vc_LoadRaw();
        break;
    case 18:
        GenericFunc(19, 4);
        break;
    case 19:
        GenericFunc(20, 1);
        break;
    case 20:
        GenericFunc(21, 7);
        break;
    case 21:
        GenericFunc(22, 0);
        break;
    case 22:
        GenericFunc(23, 0);
        break;
    case 23:
        GenericFunc(24, 1);
        break;
    case 24:
        vc_EntityMove();
        break;
    case 25:
        GenericFunc(26, 4);
        break;
    case 26:
        GenericFunc(27, 4);
        break;
    case 27:
        GenericFunc(28, 5);
        break;
    case 28:
        GenericFunc(29, 4);
        break;
    case 29:
        GenericFunc(30, 4);
        break;
    case 30:
        GenericFunc(31, 5);
        break;
    case 31:
        GenericFunc(32, 5);
        break;
    case 32:
        vc_strlen();
        break;
    case 33:
        vc_strcmp();
        break;
    case 34:
        GenericFunc(35, 0);
        break;
    case 35:
        GenericFunc(36, 1);
        break;
    case 36:
        GenericFunc(37, 1);
        break;
    case 37:
        GenericFunc(38, 1);
        break;
    case 38:
        GenericFunc(39, 3);
        break;
    case 39:
        GenericFunc(40, 2);
        break;
    case 40:
        GenericFunc(41, 1);
        break;
    case 41:
        GenericFunc(42, 1);
        break;
    case 42:
        GenericFunc(43, 3);
        break;
    case 43:
        vc_HookRetrace();
        break;
    case 44:
        vc_HookTimer();
        break;
    case 45:
        GenericFunc(46, 2);
        break;
    case 46:
        vc_SetRString();
        break;
    case 47:
        GenericFunc(48, 4);
        break;
    case 48:
        GenericFunc(49, 3);
        break;
    case 49:
        GenericFunc(50, 0);
        break;
    case 50:
        vc_PartyMove();
        break;
    case 51:
        GenericFunc(52, 1);
        break;
    case 52:
        GenericFunc(53, 1);
        break;
    case 53:
        GenericFunc(54, 1);
        break;
    case 54:
        GenericFunc(55, 0);
        break;
    case 55:
        GenericFunc(56, 1);
        break;
    case 56:
        GenericFunc(57, 1);
        break;
    case 57:
        GenericFunc(58, 5);
        break;
    case 58:
        GenericFunc(59, 5);
        break;
    case 59:
        GenericFunc(60, 2);
        break;
    case 60:
        vc_HookKey();
        break;
    case 61:
        vc_PlayMusic();
        break;
    case 62:
        GenericFunc(63, 0);
        break;
    case 63:
        GenericFunc(64, 5);
        break;
    case 64:
        vc_OpenFile();
        break;
    case 65:
        GenericFunc(66, 1);
        break;
    case 66:
        vc_QuickRead();
        break;
    case 67:
        GenericFunc(68, 1);
        break;
    case 68:
        GenericFunc(69, 1);
        break;
    case 69:
        GenericFunc(70, 0);
        break;
    case 70:
        GenericFunc(71, 0);
        break;
    case 71:
        GenericFunc(72, 7);
        break;
    case 72:
        GenericFunc(73, 15);
        break;
    case 73:
        vc_CacheSound();
        break;
    case 74:
        GenericFunc(75, 0);
        break;
    case 75:
        GenericFunc(76, 3);
        break;
    case 76:
        GenericFunc(77, 7);
        break;
    case 77:
        GenericFunc(78, 4);
        break;
    case 78:
        GenericFunc(79, 4);
        break;
    case 79:
        vc_val();
        break;
    case 80:
        GenericFunc(81, 7);
        break;
    case 81:
        GenericFunc(82, 5);
        break;
    case 82:
        vc_Log();
        break;
    case 83:
        GenericFunc(84, 2);
        break;
    case 84:
        GenericFunc(85, 2);
        break;
    case 85:
        GenericFunc(86, 3);
        break;
    case 86:
        GenericFunc(87, 1);
        break;
    case 87:
        GenericFunc(88, 1);
        break;
    case 88:
        GenericFunc(89, 1);
        break;
    case 89:
        vc_fgetline();
        break;
    case 90:
        vc_fgettoken();
        break;
    case 91:
        vc_fwritestring();
        break;
    case 92:
        GenericFunc(93, 3);
        break;
    case 93:
        vc_frename();
        break;
    case 94:
        vc_fdelete();
        break;
    case 95:
        vc_fwopen();
        break;
    case 96:
        GenericFunc(97, 1);
        break; // fwclose
    case 97:
        GenericFunc(98, 3);
        break; // memcpy
    case 98:
        GenericFunc(99, 3);
        break; // memset
    case 99:
        GenericFunc(100, 5);
        break; // silhouette
    case 100:
        GenericFunc(101, 0);
        break; // initmosaictable
    case 101:
        GenericFunc(102, 7);
        break;
    case 102:
        GenericFunc(103, 1);
        break;
    case 103:
        GenericFunc(104, 1);
        break;
    case 104:
        GenericFunc(105, 1);
        break;
    case 105:
        vc_asc();
        break;
    case 106:
        GenericFunc(107, 1);
        break;
    case 107:
        vc_NumForScript();
        break;
    case 108:
        vc_FileSize();
        break;
    case 109:
        GenericFunc(110, 1);
        break;
    case 110:
        vc_ChangeCHR();
        break;
    case 111:
        GenericFunc(112, 3);
        break; // RGB
    case 112:
        GenericFunc(113, 1);
        break; // GetR
    case 113:
        GenericFunc(114, 1);
        break; // GetG
    case 114:
        GenericFunc(115, 1);
        break; // GetB
    case 115:
        GenericFunc(116, 5);
        break; // Mask
    case 116:
        GenericFunc(117, 5);
        break; // ChangeAll
    case 117:
        GenericFunc(118, 1);
        break; // sqrt
    case 118:
        GenericFunc(119, 2);
        break; // fwritebyte
    case 119:
        GenericFunc(120, 2);
        break; // fwriteword
    case 120:
        GenericFunc(121, 2);
        break; // fwritequad
    case 121:
        GenericFunc(122, 1);
        break; // CalcLucent
    case 122:
        GenericFunc(123, 4);
        break; // ImageSize
    default:
        vcerr("Internal error. Unknown standard function.");
    }
}

void HandleUserFunction() {
    int i, idx;
    function_t* pfunc;

    idx = tok.index; // funcidx;
    // gotta do this because of nesting, and find_fun
    // could change while parsing args
    pfunc = find_fun;

    EmitW((short)idx);
    Expect("(");

    for (i = 0; i < pfunc->numargs; i++) {
        if (i) {
            Expect(",");
        }

        if (INT == pfunc->argtype[i]) {
            EmitOperand();
        }
        if (STRING == pfunc->argtype[i]) {
            HandleString();
        }
    }
    Expect(")");
}

void HandleFunctionType() {
    // if (token_subtype==op_BFUNC)
    if (op_BFUNC == tok.subtype) {
        EmitC(opEXEC_STDLIB);
        HandleFunction();
    }
    // else if (token_subtype==op_UFUNC)
    else if (op_UFUNC == tok.subtype) {
        EmitC(opEXEC_EXTERNFUNC);
        HandleUserFunction();
    }
}

void HandleIfComponent() {
    char ot = 0;

    if (NextIs("!")) {
        ot = i_ZERO;
        GetToken();
    }

    EmitOperand();

    if (NextIs("=")) {
        ot = i_EQUALTO;
        GetToken();
    } else if (NextIs("!=")) {
        ot = i_NOTEQUAL;
        GetToken();
    } else if (NextIs(">")) {
        ot = i_GREATERTHAN;
        GetToken();
    } else if (NextIs(">=")) {
        ot = i_GREATERTHANOREQUAL;
        GetToken();
    } else if (NextIs("<")) {
        ot = i_LESSTHAN;
        GetToken();
    } else if (NextIs("<=")) {
        ot = i_LESSTHANOREQUAL;
        GetToken();
    }

    if (!ot)
        EmitC(i_NONZERO);
    else if (ot == i_ZERO)
        EmitC(i_ZERO);
    else {
        EmitC(ot);
        EmitOperand();
    }
}

void HandleIfGroup() {
    while (1) {
        HandleIfComponent();

        if (NextIs("&&")) {
            EmitC(i_AND);
            GetToken();
            continue;
        } else if (NextIs("||")) {
            EmitC(i_OR);
            GetToken();
            continue;
        } else {
            GetToken();
            if (!TokenIs(")") && !TokenIs(";"))
                vcerr("Syntax error.");
            EmitC(i_UNGROUP);
            break;
        }
    }
}

void Block() {
    if (NextIs("{")) {
        Expect("{");
        do {
            // EEP @_@
            if (NextIs("}") && !TokenIs("\""))
                break;
            DoSomething();
        } while (1);
        Expect("}");
    } else
        DoSomething();
}

void ProcessIf() {
    unsigned char* falseofs = 0;
    unsigned char* elseofs = 0;
    unsigned char* b = 0;

    EmitC(opIF);
    Expect("(");
    HandleIfGroup();

    falseofs = code;
    EmitD(0); // We'll come back to this and fix it up.

    Block();

    int e = NextIs("else");
    if (e) {
        EmitC(opGOTO);
        elseofs = code;
        EmitD(0);
    }
    /*
    if (!NextIs("{") && !TokenIs("\""))
    {
            DoSomething();

            if (NextIs("else"))
            {
                    EmitC(opGOTO);
                    elseofs=code;
                    EmitD(0);
            }
    }
    else
    {
            Expect("{");
            while (!NextIs("}") && !TokenIs("\"")) DoSomething();

            Expect("}");
            if (NextIs("else"))
            {
                    EmitC(opGOTO);
                    elseofs=code;
                    EmitD(0);
            }
    }
    */

    b = code; // Put correct false-execution offset thingy.
    code = falseofs;
    EmitD(b - outbuf);
    code = b;

    if (e) {
        GetToken();
        Block();
        /*
        if (!NextIs("{") && !TokenIs("\""))
        {
                DoSomething();
        }
        else
        {
                Expect("{");
                while (!NextIs("}")) DoSomething();
                Expect("}");
        }
        */

        b = code; // Put correct else-execution offset thingy.
        code = elseofs;
        EmitD(b - outbuf);
        code = b;
    }
}

void ProcessWhile() {
    unsigned char *falseofs, *top, *b;

    top = code;
    EmitC(opIF);
    Expect("(");
    HandleIfGroup();

    falseofs = code;
    EmitD(0); // We'll come back to this and fix it up.

    Block();

    EmitC(opGOTO);
    EmitD(top - outbuf);
    /*
    if (!NextIs("{") && !TokenIs("\""))
    {
            DoSomething();

            EmitC(opGOTO);
            EmitD((int) top - (int) outbuf);
    }
    else
    {
            Expect("{");
            while (!NextIs("}")) DoSomething();
            Expect("}");
            EmitC(opGOTO);
            EmitD((int) top - (int) outbuf);
    }
    */

    b = code; // Put correct false-execution offset thingy.
    code = falseofs;
    EmitD(b - outbuf);
    code = b;
}

void ProcessFor() {
    unsigned char *src1, *src2, *srctmp;
    int loopstartpos;

    Expect("(");
    while (!TokenIs(";"))
        DoSomething(); // Emit initialization code.

    src1 = src; // Store position of loop conditional
    while (!NextIs(";"))
        GetToken();
    GetToken();
    src2 = src; // Store position of end-of-loop code

    while (!NextIs(")"))
        GetToken();
    GetToken();

    loopstartpos = code - outbuf;

    Block();
    /*
    if (!NextIs("{"))
    {
            DoSomething();
    }
    else
    {
            Expect("{");
            while (!NextIs("}")) DoSomething();
            Expect("}");
    }
    */
    srctmp = src;
    src = src2;
    while (!TokenIs(")"))
        DoSomething();
    src = src1;
    EmitC(opIF);
    HandleIfGroup();
    EmitD(code - outbuf + 9);
    EmitC(opGOTO);
    EmitD(loopstartpos);

    src = srctmp;
}

void HandleAssign() {
    int vc;

    vc = tok.subtype; // varcategory;
    EmitC(opASSIGN);
    if (vc == op_UVAR) {
        EmitC(op_UVAR);
        EmitD(find_var->varstartofs);
    } else if (vc == op_UVARRAY) {
        EmitC(op_UVARRAY);
        EmitD(find_var->varstartofs);
        Expect("[");
        EmitOperand();
        Expect("]");
    } else if (vc == op_LVAR) {
        EmitC(op_LVAR);
        // EmitC((char) varidx);
        EmitC((char)tok.index);
    } else if (vc == op_HVAR0) {
        EmitC(op_HVAR0);
        // EmitC((char) varidx);
        EmitC((char)tok.index);
    } else if (vc == op_HVAR1) {
        EmitC(op_HVAR1);
        // EmitC((char) varidx);
        EmitC((char)tok.index);
        Expect("[");
        EmitOperand();
        Expect("]");
    } else if (vc == op_STRING) {
        EmitC(op_STRING);
        EmitW((short)find_str->vsofs);
    } else if (vc == op_SARRAY) {
        EmitC(op_SARRAY);
        EmitW((short)find_str->vsofs);
        Expect("[");
        EmitOperand();
        Expect("]");
    } else if (vc == op_SLOCAL) {
        EmitC(op_SLOCAL);
        // EmitW((short) varidx);
        EmitW((short)tok.index);
    }
    GetToken();
    if (TokenIs("++")) {
        EmitC(a_INC);
        GetToken();
        return;
    } else if (TokenIs("--")) {
        EmitC(a_DEC);
        GetToken();
        return;
    } else if (TokenIs("+=")) {
        EmitC(a_INCSET);
    } else if (TokenIs("-=")) {
        EmitC(a_DECSET);
    } else if (TokenIs("=")) {
        EmitC(a_SET);
    } else
        vcerr("Invalid assignment operator.");
    if (vc == op_STRING)
        HandleString();
    else if (vc == op_SARRAY)
        HandleString();
    else if (vc == op_SLOCAL)
        HandleString();
    else
        EmitOperand();
    GetToken();
}

// int c=0; // good grief

void HandleReturn() {
    if (!vctype) {
        Expect(";");
        EmitC(opRETURN);
        return;
    }
    if (!current_func->returntype) {
        Expect(";");
        EmitC(opRETURN);
        return;
    }
    if (current_func->returntype == 1) {
        EmitC(opSETRETVAL);
        EmitOperand();
        Expect(";");
        EmitC(opRETURN);
        return;
    }
    if (current_func->returntype == 2) {
        EmitC(opSETRETSTRING);
        HandleString();
        Expect(";");
        EmitC(opRETURN);
        return;
    }
}

void ProcessSwitch() {
    unsigned char *buf, *retrptr;

    EmitC(opSWITCH);
    Expect("(");
    EmitOperand();
    Expect(")");
    Expect("{");

    // case .. option loop

    while (1) //! NextIs("}"))
    {
        if (NextIs("}") && !TokenIs("\""))
            break;
        Expect("case");
        EmitC(opCASE);
        EmitOperand();
        Expect(":");
        retrptr = code;
        EmitD(0);
        while (1) {
            if ((NextIs("case") || NextIs("}")) && !TokenIs("\""))
                break;
            DoSomething();
        }
        // while (!NextIs("case") && !NextIs("}")) DoSomething();
        EmitC(opRETURN);
        buf = code;
        code = retrptr;
        EmitD(buf - outbuf);
        code = buf;
    }
    Expect("}");
    EmitC(opRETURN);
}

void DoSomething(void) {
    GetToken();
    if (TokenIs("return")) {
        HandleReturn();
        return;
    }

    if (FUNCTION == tok.type) {
        HandleFunctionType();
        Expect(";");
        return;
    }
    if (IDENTIFIER == tok.type) {
        HandleAssign();
        return;
    }
    /*
    if (token_type==FUNCTION)   { HandleFunctionType(); Expect(";"); return; }
    if (token_type==IDENTIFIER) { HandleAssign(); return; }
    */

    if (TokenIs("if")) {
        ProcessIf();
        return;
    }
    if (TokenIs("while")) {
        ProcessWhile();
        return;
    }
    if (TokenIs("for")) {
        ProcessFor();
        return;
    }
    if (TokenIs("switch")) {
        ProcessSwitch();
        return;
    }

    vcerr("Unknown token.");
}

char xvc_sig[9] = "VERGE2X\0";

void DumpSystemIdx() {
    FILE* f;
    int n, x;
    char* code;
    int code_length;
    char buf[8];

    f = 0;
    n = 0;
    x = 0;
    code = 0;
    code_length = 0;
    buf[0] = '\0';

    vprint("Dumping index portion of system.xvc.");

    // does SYSTEM.XVC already exist?
    f = fopen("system.xvc", "rb");
    if (f) {
        // check signature
        fread(buf, 1, 8, f);
        if (strncmp(buf, xvc_sig, 7))
            err("DumpSystemIDX: system.xvc contains invalid signature");

        // find position of code
        fread(&n, 1, 4, f);    // get code start
        fseek(f, 0, SEEK_END); // go to end
        x = ftell(f) - n + 1;  // get difference
        fseek(f, n, SEEK_SET); // go back to code start
                               // *is* there any code?
        if (x > 0) {
            // yep, save it
            code = new char[x];
            if (!code)
                err("DumpSystemIDX: memory exhausted on code");
            fread(code, 1, x, f);
        }
        // through with pre-read
        fclose(f);
    }

    f = fopen("system.xvc", "wb");
    if (!f) {
        err("DumpSystemIDX: error opening system.xvc for writing index");
    }

    fwrite(xvc_sig, 1, 8, f);

    n = 0;
    fwrite(&n, 1, 4, f);

    n = varlist.number_nodes();
    fwrite(&n, 1, 4, f);
    if (n) {
        varlist.go_head();
        do {
            ((variable_t*)varlist.current())->write(f);
            varlist.go_next();
        } while (varlist.current() != varlist.head());
    }

    n = functionlist.number_nodes();
    fwrite(&n, 1, 4, f);
    if (n) {
        functionlist.go_head();
        do {
            ((function_t*)functionlist.current())->write(f);
            functionlist.go_next();
        } while (functionlist.current() != functionlist.head());
    }

    n = strlist.number_nodes();
    fwrite(&n, 1, 4, f);
    if (n) {
        strlist.go_head();
        do {
            ((string_t*)strlist.current())->write(f);
            strlist.go_next();
        } while (strlist.current() != strlist.head());
    }

    n = ftell(f);
    fseek(f, 8, SEEK_SET);
    fwrite(&n, 1, 4, f);
    fseek(f, n, SEEK_SET);

    // did we save code in the pre-read? if so, rewrite it
    if (x > 0) {
        fwrite(code, 1, x, f);
        delete[] code;
    }

    fclose(f);
}

void DestroyLists() {
    while (functionlist.head()) {
        function_t* pfunc = (function_t*)functionlist.head();
        functionlist.unlink((linked_node*)pfunc);
        delete pfunc;
    }
    while (varlist.head()) {
        variable_t* pvar = (variable_t*)varlist.head();
        varlist.unlink((linked_node*)pvar);
        delete pvar;
    }
    while (strlist.head()) {
        string_t* pstr = (string_t*)strlist.head();
        strlist.unlink((linked_node*)pstr);
        delete pstr;
    }
}

void ReadSystemIdx() {
    FILE* f;
    int n;
    char buf[8];

    DestroyLists();

    f = fopen("system.xvc", "rb");
    if (!f) {
        err("ReadSystemIdx: could not open system.xvc to access index");
    }

    // check signature
    fread(buf, 1, 8, f);
    if (strncmp(buf, xvc_sig, 8)) {
        err("ReadSystemIdx: system.xvc contains invalid signature");
    }

    // skip code location
    fseek(f, 4, SEEK_CUR);

    fread(&n, 1, 4, f);
    while (n--) {
        variable_t* newvar = new variable_t;
        if (!newvar) {
            err("ReadSystemIdx: memory exhausted on newvar");
        }
        newvar->read(f);
        varlist.insert_tail((linked_node*)newvar);
    }

    fread(&n, 1, 4, f);
    while (n--) {
        function_t* newfunc = new function_t;
        if (!newfunc) {
            err("ReadSystemIdx: memory exhausted on newfunc");
        }
        newfunc->read(f);
        functionlist.insert_tail((linked_node*)newfunc);
    }

    fread(&n, 1, 4, f);
    while (n--) {
        string_t* newstr = new string_t;
        if (!newstr) {
            err("ReadSystemIdx: memory exhausted on newstr");
        }
        newstr->read(f);
        strlist.insert_tail((linked_node*)newstr);
    }

    fclose(f);
}

void DoLocalVariables() {
    // int		n;
    int na;

    na = current_func->numargs;

    /*
    dprint("> locals before");
    for (n=0; n<na; n++)
    {
            char* ty="!UNKNOWN!";
            if (current_func->argtype[n]==INT)
                    ty="INT";
            if (current_func->argtype[n]==STRING)
                    ty="STRING";
            dprint("\t%-29s %-6s OFS(%d)", larg[n], ty,
    current_func->argtype[n]==INT?GetVarIdx(n):GetStringIdx(n));
    }
    */

    // while (NextIs("local"))
    while (NextIs("int") || NextIs("string") || NextIs("local")) {
        GetToken();
        // ignore prefix
        if (TokenIs("local"))
            GetToken();
        // GetToken();	// type

        if (TokenIs("int")) {
            current_func->argtype[na] = INT;
            GetToken();
            memcpy(larg[na++], tok.ident, 40); // token, 40);
            while (!NextIs(";")) {
                Expect(",");
                current_func->argtype[na] = INT;
                GetToken();
                memcpy(larg[na++], tok.ident, 40); // token, 40);
            }
            Expect(";");
        } else if (TokenIs("string")) {
            current_func->argtype[na] = STRING;
            GetToken();
            memcpy(larg[na++], tok.ident, 40); // token, 40);
            while (!NextIs(";")) {
                Expect(",");
                current_func->argtype[na] = STRING;
                GetToken();
                memcpy(larg[na++], tok.ident, 40); // token, 40);
            }
            Expect(";");
        }
    }

    current_func->numlocals = na;

    /*
    dprint("routine: %s", current_func->fname);
    dprint("> locals after");
    for (n=0; n<na; n++)
    {
            char* ty="!UNKNOWN!";
            if (current_func->argtype[n]==INT)
                    ty="INT";
            if (current_func->argtype[n]==STRING)
                    ty="STRING";
            dprint("\t%-29s %-6s OFS(%d)", larg[n], ty,
    current_func->argtype[n]==INT?GetVarIdx(n):GetStringIdx(n));
    }
    */
}

void CompileMAP(char* fname) {
    PreProcess(va("%s.vc", fname));

    LoadSource();
    AllocateWriteBuffer();

    ReadSystemIdx();

    src = source;
    code = outbuf;
    vctype = 0;

    while (*src) {
        functbl.insert_tail((linked_node*)new funcmark((int)(code - outbuf)));

        Expect("event");
        Expect("{");

        while (1) //! NextIs("}"))
        {
            if (NextIs("}") && !TokenIs("\""))
                break;
            DoSomething();
        }

        Expect("}");
        EmitC(opRETURN);

        ParseWhitespace();
    }

    dprint("%s.vc (%i lines)", fname, lines);
}

// TODO: make this skip local vars, then run until it hits either its matching
// }, or error out
// by hitting another variable dec (fell into global space), or hitting another
// function.
void SkipBrackets() {
    while (1) //! NextIs("}"))
    {
        if (NextIs("}") && !TokenIs("\""))
            break;
        if (!*src)
            err("No matching bracket.");

        GetToken();

        if (TokenIs("{"))
            SkipBrackets();
    }
    GetToken();
}

void CheckDup() {
    int n;

    for (n = 0; n < numhardfuncs; n++) {
        if (!strcmp(hardfuncs[n], tok.ident)) // token))
        {
            vcerr("CheckDup: %s: duplicate identifier (built-in function)");
        }
    }

    for (n = 0; n < numhardvar0; n++) {
        if (!strcmp(hardvar0[n], tok.ident)) // token))
        {
            vcerr("CheckDup: %s: duplicate identifier (built-in variable)");
        }
    }

    for (n = 0; n < numhardvar1; n++) {
        if (!strcmp(hardvar1[n], tok.ident)) // token))
        {
            vcerr(
                "CheckDup: %s: duplicate identifier (built-in variable array)");
        }
    }

    if (functionlist.number_nodes()) {
        functionlist.go_head();
        do {
            function_t* pfunc = (function_t*)functionlist.current();
            if (!strcmp(pfunc->fname, tok.ident)) // token))
            {
                vcerr("CheckDup: %s: duplicate identifier (already a function "
                      "name)",
                    tok.ident); // token);
            }
            functionlist.go_next();
        } while (functionlist.current() != functionlist.head());
    }

    if (varlist.number_nodes()) {
        varlist.go_head();
        do {
            variable_t* pvar = (variable_t*)varlist.current();
            if (!strcmp(pvar->vname, tok.ident)) // token))
            {
                vcerr("CheckDup: %s: duplicate identifier (already a variable "
                      "name)",
                    tok.ident); // token);
            }
            varlist.go_next();
        } while (varlist.current() != varlist.head());
    }

    if (strlist.number_nodes()) {
        strlist.go_head();
        do {
            string_t* pstr = (string_t*)strlist.current();
            if (!strcmp(pstr->vname, tok.ident)) // token))
            {
                vcerr("CheckDup: %s: duplicate identifier (already a string "
                      "name)",
                    tok.ident); // token);
            }
            strlist.go_next();
        } while (strlist.current() != strlist.head());
    }
}

void ParseStringDecs() {
    string_t* newstr = 0;

    while (1) {
        GetToken();
        CheckDup();

        newstr = new string_t;
        if (!newstr) {
            err("FirstPass: memory exhausted on newstr");
        }
        strncpy(newstr->vname, tok.ident, 39); // token, 39);
        newstr->vname[39] = '\0';
        newstr->vsofs = sstartofs;
        if (NextIs("[")) {
            GetToken();
            GetToken();
            newstr->arraylen = tok.value; // token_nvalue;
            Expect("]");
            /*
            if (NextIs("["))
            {
                    GetToken();
                    GetToken();
                    newstr->arraylen*=tok.value;
                    Expect("]");
            }
            */
        } else {
            newstr->arraylen = 1;
        }
        sstartofs += newstr->arraylen;

        vprint("Decl '%s' of type string, size %i. [%i]", newstr->vname,
            newstr->arraylen, newstr->vsofs);

        strlist.insert_tail((linked_node*)newstr);

        if (!NextIs(",")) {
            break;
        }
        GetToken();
    }
    Expect(";");
}

void ParseFunctionDec() {
    int na = 0;
    function_t* newfunc = 0;

    newfunc = new function_t;
    if (!newfunc) {
        err("FirstPass: memory exhausted on newfunc");
    }

    if (TokenIs("void")) {
        newfunc->returntype = 0;
    } else if (TokenIs("int")) {
        newfunc->returntype = 1;
    } else if (TokenIs("string")) {
        newfunc->returntype = 2;
    }

    GetToken();
    CheckDup();

    strncpy(newfunc->fname, tok.ident, 39); // token, 39);
    newfunc->fname[39] = '\0';
    newfunc->numargs = 0;
    Expect("(");
    while (!NextIs(")")) {
        GetToken();
        na = newfunc->numargs;
        if (TokenIs("int")) {
            newfunc->argtype[na] = INT;
        } else if (TokenIs("string")) {
            newfunc->argtype[na] = STRING;
        } else {
            vcerr("Invalid arguement declaration.");
        }
        GetToken();
        if (NextIs(",")) {
            GetToken();
        }
        newfunc->numargs++;
    }

    Expect(")");
    Expect("{");

    SkipBrackets();

    const char* ret_type = "void";
    if (1 == newfunc->returntype)
        ret_type = "int";
    else if (2 == newfunc->returntype)
        ret_type = "string";
    vprint("Found '%s' declaration for '%s', %i parameters.", ret_type,
        newfunc->fname, newfunc->numargs);

    functionlist.insert_tail((linked_node*)newfunc);
}

void ParseVarDecs() {
    variable_t* newvar = 0;

    while (1) {
        GetToken();
        CheckDup();

        newvar = new variable_t;
        if (!newvar) {
            err("FirstPass: memory exhausted on newvar");
        }
        strncpy(newvar->vname, tok.ident, 39); // token, 39);
        newvar->vname[39] = '\0';
        newvar->varstartofs = curstartofs;
        if (NextIs("[")) {
            GetToken();
            GetToken();
            newvar->arraylen = tok.value; // token_nvalue;
            Expect("]");
            /*
            if (NextIs("["))
            {
                    GetToken();
                    GetToken();
                    newvar->arraylen*=tok.value;
                    Expect("]");
            */
        } else {
            newvar->arraylen = 1;
        }

        curstartofs += newvar->arraylen;

        vprint("Decl '%s' of type int, size %i. [%i]", newvar->vname,
            newvar->arraylen, newvar->varstartofs);

        varlist.insert_tail((linked_node*)newvar);

        if (!NextIs(",")) {
            break;
        }
        GetToken();
    }
    Expect(";");
}

void ParseGlobalDecs() {
    GetToken();

    if (TokenIs("int")) {
        ParseVarDecs();
    } else if (TokenIs("string")) {
        ParseStringDecs();
    }
}

void SecondPass() {
    unsigned char* savespot = 0;
    int is_func = 0;

    src = source;

    vprint("Second pass (find variables)...");

    vctype = 1;
    tlines = 0;

    while (*src) {
        savespot = src;

        GetToken();
        // ignore prefixes
        if (TokenIs("global") || TokenIs("function")) {
            savespot = src;
            GetToken();
        }

        // printf("token = %s\n", tok.ident);
        if (TokenIs("int") || TokenIs("string")) {
            GetToken();

            is_func = NextIs("(");
            src = savespot; // figure out how to get rid of this

            GetToken();
        }
        if (TokenIs("void") ||
            ((TokenIs("int") || TokenIs("string")) && is_func)) {
            ParseFunctionDec();
            continue;
        }
        if (TokenIs("int") && !is_func) {
            ParseVarDecs();
            continue;
        }
        if (TokenIs("string") && !is_func) {
            ParseStringDecs();
            continue;
        }
        /*
        if (TokenIs("global"))
        {
                ParseGlobalDecs();
                continue;
        }
        else if (TokenIs("function"))
        {
                GetToken();	// return type
                ParseFunctionDec();
                continue;
        }
        */

        // hack; we need to call ParseWhitespace here just in case we are
        // actually
        // at the end of
        // system.vc, included files and all. but, assuming we aren't, it is
        // possible that a call to
        // ParseWhitespace would stumble onto another file marker & line info
        // (from
        // everything being
        // joined by the preprocessor), so we save the source filename and line
        // count here so that
        // we have correct info if, in fact, we found something other than a
        // script
        // or variable dec
        // at the global level.
        char* f = source_file;
        int x = lines;
        ParseWhitespace();
        if (*src) {
            err("%s(%i): syntax error; expecting global variable or script, "
                "got: %s",
                f, x, tok.ident); // token);
        }
    }

    dprint("system.vc: %i ints, %i strings, %i functions",
        varlist.number_nodes(), strlist.number_nodes(),
        functionlist.number_nodes());
}

void SkipStringDecs() {
    while (1) {
        GetToken();
        if (NextIs("[")) {
            GetToken();
            GetToken();
            Expect("]");

            /*
            if (NextIs("["))
            {
                    GetToken();
                    GetToken();
                    Expect("]");
            }
            */
        }
        if (!NextIs(",")) {
            break;
        }
        GetToken();
    }
    Expect(";");
}

void SkipVarDecs() {
    while (1) {
        GetToken();
        if (NextIs("[")) {
            GetToken();
            GetToken();
            Expect("]");
            /*
            if (NextIs("["))
            {
                    GetToken();
                    GetToken();
                    Expect("]");
            }
            */
        }
        if (!NextIs(",")) {
            break;
        }
        GetToken();
    }
    Expect(";");
}

void GenerateUserFunc() {
    int n;

    if (!current_func) {
        err("GenerateUserFunc: current_func is NULL!");
    }

    current_func->syscodeofs = (int)(code - outbuf);

    GetToken();

    Expect("(");

    // get argument names
    for (n = 0; n < MAX_LOCALS; n++) {
        memset(larg[n], 0, 40);
    }
    n = 0;
    while (!NextIs(")")) {
        GetToken();
        GetToken();
        strncpy(larg[n], tok.ident, 39); // token, 39);
        larg[n][39] = '\0';
        if (NextIs(",")) {
            GetToken();
        }
        n++;
    }

    Expect(")");
    Expect("{");

    DoLocalVariables(); // works off of current_func

    while (1) //! NextIs("}"))
    {
        if (NextIs("}") && !TokenIs("\""))
            break;
        DoSomething();
    }

    EmitC(opRETURN);
    Expect("}");

    current_func = (function_t*)(current_func->next());
}

void SkipGlobalDecs() {
    GetToken();

    if (TokenIs("int")) {
        SkipVarDecs();
    } else if (TokenIs("string")) {
        SkipStringDecs();
    }
}

void ThirdPass() {
    int is_func = 0;
    unsigned char* savespot = 0;

    vprint("Third pass (write function code)...");

    code = outbuf;
    src = source;
    tlines = 0;

    current_func = (function_t*)(functionlist.head());

    while (*src) {
        // Everything in system.vc will either be a global var decl or
        // a function definition.

        savespot = src;

        GetToken();
        // ignore prefixes
        if (TokenIs("global") || TokenIs("function")) {
            savespot = src;
            GetToken();
        }

        if (TokenIs("int") || TokenIs("string")) {
            GetToken();

            is_func = NextIs("(");
            src = savespot; // figure out how to get rid of this

            GetToken();
        }
        if (TokenIs("void") ||
            ((TokenIs("int") || TokenIs("string")) && is_func)) {
            GenerateUserFunc();
        }
        if (TokenIs("int") && !is_func)
            SkipVarDecs();
        if (TokenIs("string") && !is_func)
            SkipStringDecs();
    }

    dprint("system.vc (%i lines, %i total)", lines, tlines);
}

void LoadSource() {
    FILE* f;
    printf("LoadSource\n");

    f = fopen("vcctemp.$$$", "rb");
    if (!f) {
        err("Could not open source file.");
    }
    fseek(f, 0, SEEK_END);
    source_length = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (source)
        delete[] source;
    source = new unsigned char[source_length + 2];
    if (!source) {
        err("LoadSource: memory exhausted");
    }
    fread(source, 1, source_length, f);
    source[source_length + 0] = '\0';
    source[source_length + 1] = '\0';

    fclose(f);

    printf("ok\n");
}

void AllocateWriteBuffer() {
    // allocate a meg first time around
    if (!outbuf) {
        // TODO: use a linked list to hold blocks of generated code as it comes
        outbuf = new unsigned char[1024 * 1024];
        if (!outbuf) {
            err("AllocateWriteBuffer: memory exhausted");
        }
        memset(outbuf, 0, 1024 * 1024);
    }
    // it ptr already exists, just wipe the mem; might not even need to do this
}

void CompileSystem() {
    // system.vc is compiled in a three-pass system. the first pass
    // pre-processes
    // system.vc, which involves reading in all #included files and writing them
    // out
    // as one file, performing any #define replacements as needed. the
    // pre-processing
    // stage also removes all unecessary whitespace from the source. the second
    // pass
    // goes through the system.vc file and sets up declarations for all global
    // variables
    // and function declarations, so system.vc won't have to worry about forward
    // declarations or anything. the final pass generates the codes for all user
    // functions.

    PreProcess("system.vc"); // first pass

    LoadSource();
    AllocateWriteBuffer();

    SecondPass();

    ThirdPass();

    DumpSystemIdx();
}
