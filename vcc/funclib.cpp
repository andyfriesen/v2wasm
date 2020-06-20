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

// ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
// ³                  The VergeC Compiler version 2.01                   ³
// ³              Copyright (C)1998 BJ Eirich (aka vecna)                ³
// ³                  Standard Function  Library module                  ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

#include "funclib.h"
#include "compile.h"
#include "lexical.h"
#include "vcc.h"
#include "vccode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ================================= Data ====================================

// ================================= Code ====================================

void vcfunc_Exit() {
    EmitC(1);
    Expect("(");
    HandleString();
    Expect(")");
}

void Message() {
    EmitC(2);
    Expect("(");
    HandleString();
    Expect(",");
    EmitOperand();
    Expect(")");
}

void vc_loadimage() {
    EmitC(6);
    Expect("(");
    HandleString();
    Expect(")");
}

void vc_AllocateEntity() {
    EmitC(11);
    Expect("(");
    EmitOperand();
    Expect(",");
    EmitOperand();
    Expect(",");
    HandleString();
    Expect(")");
}

void vc_Map() {
    EmitC(13);
    Expect("(");
    HandleString();
    Expect(")");
}

void vc_LoadFont() {
    EmitC(14);
    Expect("(");
    HandleString();
    Expect(")");
}

void vc_PlayFLI() {
    EmitC(15);
    Expect("(");
    HandleString();
    Expect(")");
}

void vc_PrintString() {
    EmitC(17);
    Expect("(");
    EmitOperand();
    Expect(",");
    HandleString();
    Expect(")");
}

void vc_LoadRaw() {
    EmitC(18);
    Expect("(");
    HandleString();
    Expect(")");
}

void vc_EntityMove() {
    EmitC(25);
    Expect("(");
    EmitOperand();
    Expect(",");
    HandleString();
    Expect(")");
}

void vc_strlen() {
    EmitC(33);
    Expect("(");
    HandleString();
    Expect(")");
}

void vc_strcmp() {
    EmitC(34);
    Expect("(");
    HandleString();
    Expect(",");
    HandleString();
    Expect(")");
}

void vc_SetRString() {
    EmitC(47);
    Expect("(");
    HandleString();
    Expect(")");
}

void vc_PartyMove() {
    EmitC(51);
    Expect("(");
    HandleString();
    Expect(")");
}

void vc_PlayMusic() {
    EmitC(62);
    Expect("(");
    HandleString();
    Expect(")");
}

void vc_OpenFile() {
    EmitC(65);
    Expect("(");
    HandleString();
    Expect(")");
}

void vc_HookRetrace() {
    unsigned char* na;

    EmitC(44);
    Expect("(");
    na = src;
    GetToken();
    // if (token_subtype == op_UFUNC)
    if (op_UFUNC == tok.subtype) {
        EmitC(2);
        // EmitD(funcidx);
        EmitD(tok.index);
    } else {
        src = na;
        EmitC(1);
        EmitOperand();
    }
    Expect(")");
}

void vc_HookTimer() {
    unsigned char* na;

    EmitC(45);
    Expect("(");
    na = src;
    GetToken();
    // if (token_subtype == op_UFUNC)
    if (op_UFUNC == tok.subtype) {
        EmitC(2);
        // EmitD(funcidx);
        EmitD(tok.index);
    } else {
        src = na;
        EmitC(1);
        EmitOperand();
    }
    Expect(")");
}

void vc_HookKey() {
    unsigned char* na;

    EmitC(61);
    Expect("(");
    EmitOperand();
    Expect(",");
    na = src;
    GetToken();
    // if (token_subtype == op_UFUNC)
    if (op_UFUNC == tok.subtype) {
        EmitC(2);
        // EmitD(funcidx);
        EmitD(tok.index);
    } else {
        src = na;
        EmitC(1);
        EmitOperand();
    }
    Expect(")");
}

void vc_QuickRead() {
    EmitC(67);
    Expect("(");
    HandleString();
    Expect(",");
    GetToken();
    // if (varcategory == op_STRING)
    if (op_STRING == tok.subtype) {
        EmitC(op_STRING);
        // EmitW((short)str[varidx].vsofs);
        EmitW((short)find_str->vsofs);
        Expect(",");
        EmitOperand();
        Expect(")");
        return;
    }
    // if (varcategory == op_SARRAY)
    if (op_SARRAY == tok.subtype) {
        EmitC(op_SARRAY);
        // EmitW((short)str[varidx].vsofs);
        EmitW((short)find_str->vsofs);
        Expect("[");
        EmitOperand();
        Expect("]");
        Expect(",");
        EmitOperand();
        Expect(")");
        return;
    }
    vcerr("String variable expected.");
}

void vc_CacheSound() {
    EmitC(74);
    Expect("(");
    HandleString();
    Expect(")");
}

void vc_val() {
    EmitC(80);
    Expect("(");
    HandleString();
    Expect(")");
}

void vc_Log() {
    EmitC(83);
    Expect("(");
    HandleString();
    Expect(")");
}

void vc_fgetline() {
    EmitC(90);
    Expect("(");
    GetToken();
    // if (varcategory == op_STRING)
    if (op_STRING == tok.subtype) {
        EmitC(op_STRING);
        // EmitW((short)str[varidx].vsofs);
        EmitW((short)find_str->vsofs);
        Expect(",");
        EmitOperand();
        Expect(")");
        return;
    }
    // if (varcategory == op_SARRAY)
    if (op_SARRAY == tok.subtype) {
        EmitC(op_SARRAY);
        // EmitW((short)str[varidx].vsofs);
        EmitW((short)find_str->vsofs);
        Expect("[");
        EmitOperand();
        Expect("]");
        Expect(",");
        EmitOperand();
        Expect(")");
        return;
    }
    vcerr("String variable expected.");
}

void vc_fgettoken() {
    EmitC(91);
    Expect("(");
    GetToken();
    // if (varcategory == op_STRING)
    if (op_STRING == tok.subtype) {
        EmitC(op_STRING);
        // EmitW((short)str[varidx].vsofs);
        EmitW((short)find_str->vsofs);
        Expect(",");
        EmitOperand();
        Expect(")");
        return;
    }
    // if (varcategory == op_SARRAY)
    if (op_SARRAY == tok.subtype) {
        EmitC(op_SARRAY);
        // EmitW((short)str[varidx].vsofs);
        EmitW((short)find_str->vsofs);
        Expect("[");
        EmitOperand();
        Expect("]");
        Expect(",");
        EmitOperand();
        Expect(")");
        return;
    }
    /*        // --- tSB --- // Not working right, dunno why yet :P
            if (op_SLOCAL == tok.subtype)
            {
                    EmitC(op_SLOCAL);
                    EmitW((short)tok.index);
                    Expect(",");
                    EmitOperand();
                    Expect(")");
                    return;
            }
            // --- tSB ---*/
    vcerr("String variable expected.");
}

void vc_fwritestring() {
    EmitC(92);
    Expect("(");
    HandleString();
    Expect(",");
    EmitOperand();
    Expect(")");
}

void vc_frename() {
    EmitC(94);
    Expect("(");
    HandleString();
    Expect(",");
    HandleString();
    Expect(")");
}

void vc_fdelete() {
    EmitC(95);
    Expect("(");
    HandleString();
    Expect(")");
}

void vc_fwopen() {
    EmitC(96);
    Expect("(");
    HandleString();
    Expect(")");
}

void vc_asc() {
    EmitC(106);
    Expect("(");
    HandleString();
    Expect(")");
}

void vc_NumForScript() {
    EmitC(108);
    Expect("(");
    GetToken();
    // if (token_subtype == op_UFUNC)
    if (op_UFUNC == tok.subtype)
        // EmitD(funcidx);
        EmitD(tok.index);
    else
        vcerr("system script expected.");
    Expect(")");
}

void vc_FileSize() {
    EmitC(109);
    Expect("(");
    HandleString();
    Expect(")");
}

void vc_ChangeCHR() {
    EmitC(111);
    Expect("(");
    EmitOperand();
    Expect(",");
    HandleString();
    Expect(")");
}

// added by tSB

// end of stuff tSB added

void GenericFunc(unsigned char idcode, int numargs) {
    char i;

    EmitC(idcode);
    if (!numargs) {
        Expect("(");
        Expect(")");
        return;
    }
    if (numargs == 1) {
        Expect("(");
        EmitOperand();
        Expect(")");
        return;
    }
    Expect("(");
    for (i = 1; i < numargs; i++) {
        EmitOperand();
        Expect(",");
    }
    EmitOperand();
    Expect(")");
}
