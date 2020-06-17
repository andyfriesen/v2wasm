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
// �                        VFile (VRG) module                           �
// �����������������������������������������������������������������������

#include <string.h>

#include "verge.h"
#include "wasm.h"

// ================================= Data ====================================

mountstruct pack[3];   // packfile structs
byte filesmounted = 0; // Number of VRG files to check.
// char headertag[]={ 'V','R','G','P','A','C','K',0 };
char headertag[] = "VRGPACK\0";

// ================================= Code ====================================

FILE* _fopen(const char* filename, const char* mode) {
    // TODO try lowercase first
    std::string s = gameRoot + "/persist/" + filename;
    V_strlwr(const_cast<char*>(s.data()));
    FILE* f = fopen(s.c_str(), mode);
    if (f) {
        return f;
    }

    s = gameRoot + "/persist/" + filename;
    f = fopen(s.c_str(), mode);
    if (f) {
        return f;
    }

    s = gameRoot + filename;
    V_strlwr(const_cast<char*>(s.data()));
    f = fopen(s.c_str(), mode);
    if (f) {
        return f;
    }

    s = gameRoot + filename;
    f = fopen(s.c_str(), mode);
    if (f) {
        return f;
    }

    return 0;
}

int Exist(const char* filename) {
    FILE* f;

    f = _fopen(filename, "rb");
    if (f) {
        fclose(f);
        return 1;
    }

    return 0;
}

void DecryptHeader() {
    byte lastvalue, precodebyte;
    byte* ptr;

    ptr = (byte*)pack[filesmounted].files;
    lastvalue = *ptr;
    ptr++;

    while (ptr < (byte*)(int)pack[filesmounted].files +
                     (int)pack[filesmounted].numfiles * 100) {
        precodebyte = *ptr;
        (*ptr) -= lastvalue;
        lastvalue = precodebyte;
        ptr++;
    }
}

void MountVFile(const char* filename) {
    char buffer[10] = {0};

    pack[filesmounted].vhandle = _fopen(filename, "rb");
    if (!pack[filesmounted].vhandle) {
        Sys_Error("*error* Unable to mount %s; file not found.\n", filename);
    }

    // Read pack header
    V_memset(buffer, 0, 10);
    fread(buffer, 1, 7, pack[filesmounted].vhandle);
    if (strcmp(buffer, headertag)) {
        Sys_Error("*error* %s is not a valid packfile.\n", filename);
    }

    fread(buffer, 1, 1, pack[filesmounted].vhandle);
    if (buffer[0] != 1) {
        Sys_Error(
            "*error* %s is an incompatible packfile version. (ver reported: "
            "%d) \n",
            filename, buffer[0]);
    }

    fread(&pack[filesmounted].numfiles, 1, 4, pack[filesmounted].vhandle);
    V_memcpy(pack[filesmounted].mountname, filename, V_strlen(filename) + 1);

    // Allocate memory for headers and read them in.

    pack[filesmounted].files =
        (filestruct*)valloc(pack[filesmounted].numfiles * 100,
            "pack[filesmounted].files", OID_VFILE);
    fread(pack[filesmounted].files, pack[filesmounted].numfiles, 100,
        pack[filesmounted].vhandle);

    DecryptHeader();

    filesmounted++;
}

char V_tolower(char c) {
    if (c >= 'A' && c <= 'Z') {
        c -= 'A';
        c += 'a';
    }
    return c;
}

char* V_strlwr(char* str) {
    while (*str) {
        *str = V_tolower(*str);

        str++;
    }
    return str;
}

int V_strcasecmp(const char* s1, const char* s2) {
    while (1) {
        if (V_tolower(*s1) < V_tolower(*s2))
            return -1;
        if (V_tolower(*s1) > V_tolower(*s2))
            return +1;
        if (!*s1)
            return 0;
        s1++;
        s2++;
    }
    // return 666;
}

VFILE* vopen(const char* filename) {
    char rf, vf;
    int i, j;

    rf = vf = 0;
    i = j = 0;

    // All files using V* are read-only. To write a file, use regular i/o.
    // First we'll see if a real file exists, then we'll check for one in
    // VFiles,
    // if we don't find one in VFile or it's overridable then a real file will
    // be used. That's the general logic progression.

    if (Exist(filename))
        rf = 1;

    // Search the VFiles.
    for (i = filesmounted - 1; i >= 0; i--) {
        for (j = 0; j < pack[i].numfiles; j++) {
            if (!V_strcasecmp(filename, (char*)(pack[i].files[j].fname))) {
                vf = 1;
                break;
            }
        }

        if (vf)
            break;
    }

    if (!vf && !rf)
        return 0;

    VFILE* tmp = (VFILE*)valloc(sizeof(VFILE), "vopen:tmp", OID_VFILE);

    if (vf && rf) {
        if (pack[i].files[j].override)
            vf = 0;
        else
            rf = 0;
    }

    if (vf) {
        tmp->fp = pack[i].vhandle;
        tmp->s = 1;
        tmp->v = (byte)i;
        tmp->i = (byte)j;
        pack[i].files[j].curofs = 0;
        fseek(tmp->fp, pack[i].files[j].packofs, 0);
        pack[i].curofs = pack[i].files[j].packofs;
        return tmp;
    }

    tmp->fp = _fopen(filename, "rb");
    tmp->s = 0;
    tmp->v = 0;
    tmp->i = 0;

    return tmp;
}

void vread(void* dest, int len, VFILE* f) {
    // This is fairly simple.. Just make sure our filepointer is at the right
    // place, then do a straight fread.

    if (f->s) {
        if (pack[f->v].curofs !=
            (pack[f->v].files[f->i].packofs + pack[f->v].files[f->i].curofs))
            fseek(f->fp,
                pack[f->v].files[f->i].curofs + pack[f->v].files[f->i].packofs,
                0);

        pack[f->v].files[f->i].curofs += len;
        pack[f->v].curofs += len;
    }
    fread(dest, 1, len, f->fp);
}

void vclose(VFILE* f) {
    if (!f)
        return;
    if (!f->s)
        fclose(f->fp);
    f->fp = 0;
    vfree(f);
}

int filesize(VFILE* f) {
    int oldpos, tmp;

    // Filesize for Vfiles is real simple.
    if (f->s)
        return pack[f->v].files[f->i].size;

    // It's a bit more complex for external files.
    oldpos = ftell(f->fp);
    fseek(f->fp, 0, 2);
    tmp = ftell(f->fp);
    fseek(f->fp, oldpos, 0);

    return tmp;
}

int vtell(VFILE* f) {
    if (!f->s) {
        return ftell(f->fp);
    }

    return pack[f->v].files[f->i].curofs;
}

void vseek(VFILE* f, int offset, int origin) {
    if (!f->s) {
        fseek(f->fp, offset, origin);
        return;
    }

    switch (origin) {
    case 0:
        pack[f->v].files[f->i].curofs = offset;
        fseek(f->fp, offset + pack[f->v].files[f->i].packofs, 0);
        return;

    case 1:
        pack[f->v].files[f->i].curofs += offset;
        fseek(f->fp, offset, 1);
        return;

    case 2:
        pack[f->v].files[f->i].curofs = pack[f->v].files[f->i].size - offset;
        fseek(f->fp,
            pack[f->v].files[f->i].curofs + pack[f->v].files[f->i].packofs, 0);
        return;
    }
}

void _vscanf(VFILE* f, char* format, char* dest) {
    fscanf(f->fp, format, dest);
    if (f->s)
        pack[f->v].files[f->i].curofs =
            ftell(f->fp) - pack[f->v].files[f->i].packofs;
}

char vgetc(VFILE* f) {
    char c = 0;

    vread(&c, 1, f);
    return c;
}

word vgetw(VFILE* f) {
    word c = 0;

    vread((char*)&c, 2, f);
    return c;
}

void vgets(char* str, int len, VFILE* f) {
    if (f->s) {
        if (pack[f->v].curofs !=
            (pack[f->v].files[f->i].packofs + pack[f->v].files[f->i].curofs))
            fseek(f->fp,
                pack[f->v].files[f->i].curofs + pack[f->v].files[f->i].packofs,
                0);

        pack[f->v].files[f->i].curofs += len;
        pack[f->v].curofs += len;
    }
    fgets(str, len, f->fp);
}
