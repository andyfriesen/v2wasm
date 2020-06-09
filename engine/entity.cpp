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
// ³                 Entity and Player Movement module                   ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <tSB, whenever>
// changed lidle, ridle, etc.. into array idle[4].  Makes things simpler in more
// than one place. :P
// <zero, 5.6.98>
// + corrected oversight in movement script management by sticking a hack in
//   MoveScript().  Bug caused Fx commands to not work sometimes.
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "verge.h"

// ================================= Data ====================================

chrlist_r chrlist[100];
byte nmchr, playernum;
entity_r *player = 0;
entity_r entity[256];
byte entities = 0;

chrdata chr[100];

byte numchrs = 0;
byte entidx[256];
byte cc;
byte movesuccess;

// ================================= Code ====================================

void EntitySetFace(int ent, int face) {
    entity_r *ep;

    if (ent < 0 || ent >= entities) return;

    ep = entity + ent;
    ep->facing = (byte)face;
    //        ep->moving = (byte)(1 + ep->facing);

    /*        switch (ep->facing)
            {
                    case 0: ep->frame = (byte) chr[ep->chrindex].didle; break;
                    case 1: ep->frame = (byte) chr[ep->chrindex].uidle; break;
                    case 2: ep->frame = (byte) chr[ep->chrindex].lidle; break;
                    case 3: ep->frame = (byte) chr[ep->chrindex].ridle; break;
            }*/
    ep->frame =
        (byte)chr[ep->chrindex].idle[ep->facing];  // way more elegant (yes
                                                   // I learned Pascal
                                                   // before C ^_^) - tSB
}

int ObstructionAt(int tx, int ty) {
    if (tx < 0 || tx >= layer[0].sizex || ty < 0 || ty >= layer[0].sizey)
        return 1;

    return obstruct[(ty * layer[0].sizex) + tx];
}

int Zone(int tx, int ty) {
    if (tx < 0 || tx >= layer[0].sizex || ty < 0 || ty >= layer[0].sizey)
        return 0;

    return zone[(ty * layer[0].sizex) + tx];
}

void LoadCHR(const char *fname, chrdata *c) {
    VFILE *f;
    char ver;
    char b;
    int n;
    char *ptr;

    V_strlwr((char *)fname);

    f = vopen(fname);
    if (!f) Sys_Error("Could not open CHR file %s.", fname);

    strncpy(c->fname, fname, 59);
    c->fname[59] = '\0';

    vread(&ver, 1, f);
    if (ver != 2 && ver != 4)
        Sys_Error("CHR %s incorrect CHR format version.", fname);
    if (gfx.bpp == 1 && ver == 4)
        Sys_Error("Hicolor CHRs can't be loaded when in 8bit mode");

    vread(&c->fxsize, 2, f);
    vread(&c->fysize, 2, f);
    vread(&c->hx, 2, f);
    vread(&c->hy, 2, f);

    // Modified by Pyro
    vread(&c->hw, 2, f);
    vread(&c->hh, 2, f);

    if (ver == 2) {
        vread(&c->totalframes, 2, f);
        vread(&n, 4, f);
        ptr = (char *)valloc(n, "LoadCHR:ptr", OID_TEMP);
        vread(ptr, n, f);
        c->imagedata = new byte[c->fxsize * c->fysize *
                                c->totalframes];  //(byte *) valloc(c->fxsize *
                                                  // c->fysize * c->totalframes,
        //"LoadCHR:c->imagedata",
        // OID_IMAGE);

        n = ReadCompressedLayer1(
            c->imagedata, c->fxsize * c->fysize * c->totalframes, ptr);
        if (n) Sys_Error("LoadCHR: %s: bogus compressed image data", fname);

        vfree(ptr);

        if (gfx.bpp > 1) {
            unsigned short *_16 =
                new unsigned short[c->fxsize * c->fysize * c->totalframes];
            for (n = 0; n < c->fxsize * c->fysize * c->totalframes; n++) {
                if (!c->imagedata[n])
                    _16[n] = gfx.trans_mask;
                else
                    _16[n] =
                        gfx.Conv8(c->imagedata[n]);  // LoToHi(c->imagedata[n]);
            }
            delete[] c->imagedata;
            c->imagedata = (byte *)_16;
        }

        vread(&c->idle[2], 4, f);
        vread(&c->idle[3], 4, f);
        vread(&c->idle[1], 4, f);
        vread(&c->idle[0], 4, f);

        for (b = 0; b < 4; b++) {
            switch (b) {
            case 0:
                ptr = c->lanim;
                break;
            case 1:
                ptr = c->ranim;
                break;
            case 2:
                ptr = c->uanim;
                break;
            case 3:
                ptr = c->danim;
                break;
            }
            vread(&n, 4, f);
            if (n > 99) Sys_Error("Animation strand too long. %d", n);
            vread(ptr, n, f);
        }
    } else  // if (ver==4)
    {
        vread(&c->idle[2], 2, f);
        vread(&c->idle[3], 2, f);
        vread(&c->idle[1], 2, f);
        vread(&c->idle[0], 2, f);
        vread(&c->totalframes, 2, f);
        for (b = 0; b < 4; b++) {
            switch (b) {
            case 0:
                ptr = c->lanim;
                break;
            case 1:
                ptr = c->ranim;
                break;
            case 2:
                ptr = c->uanim;
                break;
            case 3:
                ptr = c->danim;
                break;
            }
            vread(&n, 4, f);

            if (n > 99)
                Sys_Error(
                    "!Animation strand too long. %d  %d", n, c->totalframes);
            vread(ptr, n, f);
            // ptr[n]='\0';
        }

        c->imagedata =
            new byte[c->fxsize * c->fysize * c->totalframes * 2];  //(byte
        //*)valloc(c->fxsize*c->fysize*c->totalframes*2,"LoadCHR
        //(4):imagedata",OID_IMAGE);
        vread(&n, 4, f);
        ptr = (char *)valloc(n, "LoadCHR (hicolour): n", OID_TEMP);
        vread(ptr, n, f);
        n = ReadCompressedLayer2(((word *)c->imagedata),
            c->fxsize * c->fysize * c->totalframes, (word *)ptr);
        if (n) Sys_Error("Stuff has happened whilst loading up a hicolour CHR");
        vfree(ptr);
        // adjust data to suit pixel format
        if (gfx.bpp > 1) {
            for (n = 0; n < (c->fxsize * c->fysize * c->totalframes); n++)
                if (!((word *)c->imagedata)[n])
                    ((word *)c->imagedata)[n] = gfx.trans_mask;
        }
    }
    vclose(f);
}

int CacheCHR(const char *fname) {
    if (numchrs >= 100)
        Sys_Error("CacheCHR: too many chrs loaded: %d", numchrs);

    LoadCHR(fname, &chr[numchrs]);
    return numchrs++;
}

void FreeCHRList() {
    int n = 0;

    for (n = 0; n < numchrs; n++) {
        // vfree(chr[i].imagedata);
        delete[] chr[n].imagedata;
    }
    V_memset(chr, 0, sizeof(chr));
}

void LoadCHRList() {
    int n;

    for (n = 0; n < nmchr; n++) {
        if (V_strlen(chrlist[n].t)) CacheCHR(chrlist[n].t);
    }
}

void DrawEntity(int i) {
    int a, b, dx, dy;
    unsigned char *ptr;

    if (!entity[i].visible || !entity[i].on) return;

    dx = entity[i].x - xwin;
    dy = entity[i].y - ywin;

    a = entity[i].chrindex;
    if (a < 0 || a >= numchrs) return;

    b = entity[i].specframe
            ? entity[i].specframe
            : entity[i].frame;  // specframe was disabled! why?! -tSB

    if (b < 0 || b >= chr[a].totalframes)
        Sys_Error("DrawEntity: invalid frame request: %d (%d total)", b,
            chr[a].totalframes);

    ptr = (unsigned char *)(chr[a].imagedata +
                            ((b * chr[a].fxsize * chr[a].fysize) * gfx.bpp));

    gfx.TCopySprite(
        dx - chr[a].hx, dy - chr[a].hy, chr[a].fxsize, chr[a].fysize, ptr);
}

static int cmpent(const void *a, const void *b) {
    return entity[*(byte *)a].y - entity[*(byte *)b].y;
}

void RenderEntities() {
    int n;

    qsort(entidx, cc, 1, cmpent);
    for (n = 0; n < cc; n++) DrawEntity(entidx[n]);
}

int GetArg(entity_r *p) {
    int n;
    static char token[10];

    while (' ' == *p->animofs) p->animofs++;

    n = 0;
    while (*p->animofs >= '0' && *p->animofs <= '9') {
        token[n++] = *p->animofs++;
    }
    token[n] = 0;

    return V_atoi(token);
}

static void AnimateSetFrames(entity_r *p) {
    if (!p) return;

    if (!p->animofs) {
        p->delayct = 0;

        if (p->moving) {
            switch (p->facing) {
            case 0:
                p->animofs = chr[p->chrindex].danim;
                break;
            case 1:
                p->animofs = chr[p->chrindex].uanim;
                break;
            case 2:
                p->animofs = chr[p->chrindex].lanim;
                break;
            case 3:
                p->animofs = chr[p->chrindex].ranim;
                break;
            }
        }
        // !p->moving
        else {
            p->animofs = 0;  // tSB
            p->frame = (byte)chr[p->chrindex].idle[p->facing];
            p->delayct = 0;  // tSB
        }
    }
}

void GetNextCommand(entity_r *p) {
    if (p->animofs == NULL)  // gah! --tSB
    {
        AnimateSetFrames(p);
        return;
    }

    while (' ' == *p->animofs) p->animofs++;
    switch (*p->animofs++) {
    case 'f':
    case 'F':
        p->frame = (byte)GetArg(p);
        break;
    case 'w':
    case 'W':
        p->delayct = (byte)GetArg(p);
        break;

    case 0:
        p->animofs = 0;
        AnimateSetFrames(p);
        break;
    }
}

void AnimateEntity(entity_r *p) {
    if (!p) return;

    AnimateSetFrames(p);

    if (p->delayct)
        p->delayct--;
    else
        GetNextCommand(p);
}

int EntityAt(int ex, int ey) {
    int n;

    for (n = 0; n < cc; n++) {
        if (&entity[entidx[n]] == player) continue;
        if (ex == entity[entidx[n]].tx && ey == entity[entidx[n]].ty) {
            if (entity[entidx[n]].on) return entidx[n] + 1;
        }
    }
    return 0;
}

int EntityObsAt(int ex, int ey) {
    int n;

    for (n = 0; n < cc; n++) {
        if (&entity[entidx[n]] == player) continue;
        if (ex == entity[entidx[n]].tx && ey == entity[entidx[n]].ty &&
            entity[entidx[n]].obsmode2) {
            if (entity[entidx[n]].on) return entidx[n] + 1;
        }
    }
    return 0;
}

int AEntityObsAt(int ex, int ey) {
    int n;

    for (n = 0; n < cc; n++) {
        if (ex == entity[entidx[n]].tx && ey == entity[entidx[n]].ty &&
            entity[entidx[n]].obsmode2) {
            if (entity[entidx[n]].on) return entidx[n] + 1;
        }
    }
    return 0;
}

void SiftEntities() {
    int n, dx, dy;

    V_memset(entidx, 0, 256);
    cc = 0;
    for (n = 0; n < entities; n++) {
        dx = entity[n].x - xwin + 16;
        dy = entity[n].y - ywin + 16;

        if (dx < 0 || dx > gfx.scrx + chr[entity[n].chrindex].fxsize) continue;
        if (dy < 0 || dy > gfx.scry + chr[entity[n].chrindex].fysize) continue;

        entidx[cc++] = (byte)n;
    }
}

void MoveRight(int i) {
    int tx, ty;

    tx = entity[i].tx + 1;
    ty = entity[i].ty;

    if (entity[i].obsmode1 && (ObstructionAt(tx, ty) || AEntityObsAt(tx, ty))) {
        // entity[i].reset = 1;
        entity[i].animofs = 0;
        movesuccess = 0;
        return;
    }

    if (entity[i].facing != 3) {
        entity[i].animofs = 0;  // trigger strand reset
    }
    entity[i].facing = 3;
    entity[i].moving = 4;

    entity[i].ctr = 15;
    entity[i].tx++, entity[i].x++;

    movesuccess = 1;
}

void MoveLeft(int i) {
    int tx, ty;

    tx = entity[i].tx - 1;
    ty = entity[i].ty;

    if (entity[i].obsmode1 && (ObstructionAt(tx, ty) || AEntityObsAt(tx, ty))) {
        // entity[i].reset = 1;
        entity[i].animofs = 0;
        movesuccess = 0;
        return;
    }

    if (entity[i].facing != 2) {
        entity[i].animofs = 0;  // trigger strand reset
    }
    entity[i].facing = 2;
    entity[i].moving = 3;

    entity[i].ctr = 15;
    entity[i].tx--, entity[i].x--;

    movesuccess = 1;
}

void MoveUp(int i) {
    int tx, ty;

    tx = entity[i].tx;
    ty = entity[i].ty - 1;

    if (entity[i].obsmode1 && (ObstructionAt(tx, ty) || AEntityObsAt(tx, ty))) {
        // entity[i].reset = 1;
        entity[i].animofs = 0;
        movesuccess = 0;
        return;
    }

    if (entity[i].facing != 1) {
        entity[i].animofs = 0;  // trigger strand reset
    }
    entity[i].facing = 1;
    entity[i].moving = 2;

    entity[i].ctr = 15;
    entity[i].ty--, entity[i].y--;

    movesuccess = 1;
}

void MoveDown(int i) {
    int tx, ty;

    tx = entity[i].tx;
    ty = entity[i].ty + 1;

    if (entity[i].obsmode1 && (ObstructionAt(tx, ty) || AEntityObsAt(tx, ty))) {
        // entity[i].reset = 1;
        entity[i].animofs = 0;
        movesuccess = 0;
        return;
    }

    if (entity[i].facing != 0) {
        entity[i].animofs = 0;  // trigger strand reset
    }
    entity[i].facing = 0;
    entity[i].moving = 1;

    entity[i].ctr = 15;
    entity[i].ty++, entity[i].y++;

    movesuccess = 1;
}

void Wander1(int i) {
    if (!entity[i].data1) {
        entity[i].data2 = (word)rnd(0, 3);     // pick a direction
        entity[i].data1 = entity[i].step + 1;  // and set the step count
    }

    if (entity[i].data1 == 1) {
        entity[i].delayctr++;
        if (entity[i].delayctr >= entity[i].delay) entity[i].data1 = 0;
        EntitySetFace(i, entity[i].facing);
        entity[i].animofs = 0;
        entity[i].moving = 0;
        return;
    }

    switch (entity[i].data2) {
    case 0:
        MoveUp(i);
        break;
    case 1:
        MoveDown(i);
        break;
    case 2:
        MoveLeft(i);
        break;
    case 3:
        MoveRight(i);
        break;
    }
    entity[i].data1--;
    if (entity[i].data1 == 1) entity[i].delayctr = 0;

    /*
    if (entity[i].data1==1)
    {
            //entity[i].delayctr=0;
            //entity[i].animofs=0;
            //entity[i].delayct=0;
    }
    */
}

void Wander2(int i) {
    if (!entity[i].data1) {
        entity[i].data3 = (word)rnd(
            0, 3);  // data3 is the direction the entity will try to move in
        entity[i].data1 =
            (word)(entity[i].step +
                   1);  // data1 is the number of steps in that direction
    }
    if (entity[i].data1 == 1)  // if there's one more step
    {
        entity[i].delayctr++;  // increment the wait counter
        if (entity[i].delayctr >=
            entity[i].delay)      // and check to see if we're done yet
            entity[i].data1 = 0;  // yes, set data1 to 0 so that the if above
                                  // can choose a direction
        EntitySetFace(i, entity[i].facing);
        entity[i].animofs = 0;
        entity[i].moving = 0;
        return;  // we're done
    }
    if (entity[i].data1 > 1) {
        switch (entity[i].data3) {
        case 0:
            if (Zone(entity[i].tx, entity[i].ty - 1) == entity[i].data2)
                MoveUp(i);
            break;
        case 1:
            if (Zone(entity[i].tx, entity[i].ty + 1) == entity[i].data2)
                MoveDown(i);
            break;
        case 2:
            if (Zone(entity[i].tx - 1, entity[i].ty) == entity[i].data2)
                MoveLeft(i);
            break;
        case 3:
            if (Zone(entity[i].tx + 1, entity[i].ty) == entity[i].data2)
                MoveRight(i);
            break;
        }
        entity[i].data1--;
        if (entity[i].data1 == 1) entity[i].delayctr = 0;
    }
}

void Wander3(int i) {
    if (!entity[i].data1) {
        entity[i].data2 = (word)rnd(0, 3);
        entity[i].data1 = (word)(entity[i].step + 1);
    }
    if (entity[i].data1 == 1) {
        entity[i].delayctr++;
        if (entity[i].delayctr >= entity[i].delay) entity[i].data1 = 0;
        EntitySetFace(i, entity[i].facing);
        entity[i].animofs = 0;
        entity[i].movecode = 0;
        return;
    }
    if (entity[i].data1 > 1) {
        switch (entity[i].data2) {
        case 0:
            if (entity[i].ty > entity[i].data3) MoveUp(i);
            break;
        case 1:
            if (entity[i].ty < entity[i].data6) MoveDown(i);
            break;
        case 2:
            if (entity[i].tx > entity[i].data2) MoveLeft(i);
            break;
        case 3:
            if (entity[i].tx < entity[i].data5) MoveRight(i);
            break;
        }
        entity[i].data1--;
        if (entity[i].data1 == 1) entity[i].delayct = 0;
    }
}

void Whitespace(int i) {
    while (' ' == *entity[i].scriptofs) entity[i].scriptofs++;
}

void GetArgMS(int i) {
    int j;
    char token[10];

    j = 0;
    Whitespace(i);
    while (*entity[i].scriptofs >= '0' && *entity[i].scriptofs <= '9') {
        token[j++] = *entity[i].scriptofs++;
    }
    token[j] = '\0';
    entity[i].data1 = (word)V_atoi(token);
}

void GetNextCommandMS(int i) {
    Whitespace(i);

    switch (*entity[i].scriptofs++) {
    case 'u':
    case 'U':
        entity[i].mode = 1;
        GetArgMS(i);
        break;
    case 'd':
    case 'D':
        entity[i].mode = 2;
        GetArgMS(i);
        break;
    case 'l':
    case 'L':
        entity[i].mode = 3;
        GetArgMS(i);
        break;
    case 'r':
    case 'R':
        entity[i].mode = 4;
        GetArgMS(i);
        break;
    case 's':
    case 'S':
        entity[i].mode = 5;
        GetArgMS(i);
        break;
    case 'w':
    case 'W':
        entity[i].mode = 6;
        GetArgMS(i);
        entity[i].animofs = 0;
        entity[i].delayct = 0;
        break;

    case 0:  // End of script, set entity movement to "stopped"
        entity[i].animofs = 0;
        entity[i].frame =
            (byte)chr[entity[i].chrindex]
                .idle[entity[i].facing];  // set the appropriate idle frame
        entity[i].movecode = 0;           // movecode=0 (stopped)
        entity[i].mode = 7;
        entity[i].data1 = 0;
        entity[i].scriptofs = 0;
        entity[i].delayct = 0;
        break;

    case 'c':
    case 'C':
        entity[i].mode = 8;
        GetArgMS(i);
        break;
    case 'b':
    case 'B':
        entity[i].mode = 9;
        break;
    case 'x':
    case 'X':
        entity[i].mode = 10;
        GetArgMS(i);
        break;
    case 'y':
    case 'Y':
        entity[i].mode = 11;
        GetArgMS(i);
        break;
    case 'f':
    case 'F':
        entity[i].mode = 12;
        GetArgMS(i);
        break;
    case 'z':
    case 'Z':
        entity[i].mode = 13;
        GetArgMS(i);
        break;

    default:
        Sys_Error("Invalid entity movement script.");
    }
}

void MoveScript(int i) {
    if (!entity[i].scriptofs)
        entity[i].scriptofs = ms + (int)msbuf[entity[i].movescript];

    if (!entity[i].mode) GetNextCommandMS(i);

    switch (entity[i].mode) {
    case 1:
        MoveUp(i);
        if (movesuccess) entity[i].data1--;
        break;
    case 2:
        MoveDown(i);
        if (movesuccess) entity[i].data1--;
        break;
    case 3:
        MoveLeft(i);
        if (movesuccess) entity[i].data1--;
        break;
    case 4:
        MoveRight(i);
        if (movesuccess) entity[i].data1--;
        break;
    case 5:
        entity[i].speed = (byte)entity[i].data1;
        entity[i].data1 = 0;
        break;
    case 6:
        entity[i].data1--;
        break;
    case 7:
        return;
    case 8:
        lastent = i;
        ExecuteEvent(entity[i].data1);
        entity[i].data1 = 0;
        break;
    case 9:
        entity[i].scriptofs = ms + (int)msbuf[entity[i].movescript];
        entity[i].data1 = 0;
        break;
    case 10:
        if (entity[i].tx < entity[i].data1) MoveRight(i);
        if (entity[i].tx > entity[i].data1) MoveLeft(i);
        if (entity[i].tx == entity[i].data1) entity[i].data1 = 0;
        break;
    case 11:
        if (entity[i].ty < entity[i].data1) MoveDown(i);
        if (entity[i].ty > entity[i].data1) MoveUp(i);
        if (entity[i].ty == entity[i].data1) entity[i].data1 = 0;
        break;
    case 12:
        // entity[i].facing=(byte)entity[i].data1;
        EntitySetFace(i, entity[i].data1);
        entity[i].data1 = 0;
        /*
        //--- zero 5.6.99
        switch (entity[i].facing)
        {
                case 0:
                        entity[i].frame=(byte)chr[entity[i].chrindex].didle;
                        break;
                case 1:
                        entity[i].frame=(byte)chr[entity[i].chrindex].uidle;
                        break;
                case 2:
                        entity[i].frame=(byte)chr[entity[i].chrindex].lidle;
                        break;
                case 3:
                        entity[i].frame=(byte)chr[entity[i].chrindex].ridle;
                        break;
        }
        */
        //---
        break;
    case 13:
        entity[i].specframe = (byte)entity[i].data1;
        entity[i].data1 = 0;
        break;
    }

    if (!entity[i].data1) {
        entity[i].mode = 0;
        //      entity[i].movecode=0; // tSB- 11.20.00
    }
}

void TestActive(int i) {
    int dx, dy;

    dx = abs(entity[i].x - player->x);
    dy = abs(entity[i].y - player->y);
    if ((dx <= 16 && dy <= 3) || (dx <= 3 && dy <= 16)) {
        if (!entity[i].expand4 && !invc) {
            entity[i].expand4 = 1;
            ExecuteEvent(entity[i].actscript);
        }
    } else
        entity[i].expand4 = 0;
}

static void ProcessMoving(int i) {
    switch (entity[i].moving) {
    case 1:
        entity[i].y++;
        break;
    case 2:
        entity[i].y--;
        break;
    case 3:
        entity[i].x--;
        break;
    case 4:
        entity[i].x++;
        break;
    }

    entity[i].ctr--;
    if (!entity[i].ctr) {
        entity[i].moving = 0;
    }
}

static void ProcessStopped(int i) {
    // entity[i].reset		= 1;
    // entity[i].animofs	= 0;
    // entity[i].delayct	= 0;

    switch (entity[i].movecode) {
    case 0:
        return;
    case 1:
        Wander1(i);
        break;
    case 2:
        Wander2(i);
        break;
    case 3:
        Wander3(i);
        break;
    case 4:
        MoveScript(i);
        break;

    default:
        Sys_Error("ProcessEntity1: unknown entity movement pattern: %d",
            entity[i].movecode);
    }
}

void ProcessEntity1(int i) {
    /*
    if (entity[i].reset)
    {
            entity[i].reset = 0;
            entity[i].animofs = 0;
            switch (entity[i].facing)
            {
                    case 0: entity[i].frame =
    (byte)chr[entity[i].chrindex].didle;
    break;
                    case 1: entity[i].frame =
    (byte)chr[entity[i].chrindex].uidle;
    break;
                    case 2: entity[i].frame =
    (byte)chr[entity[i].chrindex].lidle;
    break;
                    case 3: entity[i].frame =
    (byte)chr[entity[i].chrindex].ridle;
    break;
            }
            return;
    }
    */

    if (entity[i].moving) ProcessMoving(i);
    if (!entity[i].moving)  // else
        ProcessStopped(i);
}

void ProcessEntity(int i) {
    if (player == entity + i) return;

    if (!entity[i].on) return;

    if (!entity[i].speed) return;

    if (entity[i].speed < 4) {
        switch (entity[i].speed) {
        case 1:
            if (entity[i].speedct < 3) {
                entity[i].speedct++;
                return;
            }
        case 2:
            if (entity[i].speedct < 2) {
                entity[i].speedct++;
                return;
            }
        case 3:
            if (entity[i].speedct < 1) {
                entity[i].speedct++;
                return;
            }
        }
    }
    if (entity[i].speed < 5) {
        entity[i].speedct = 0;
        ProcessEntity1(i);
        AnimateEntity(entity + i);
    }
    if (entity[i].speed > 4) {
        for (int j = 0; j < entity[i].speed - 3; j++) {
            ProcessEntity1(i);
            AnimateEntity(entity + i);
        }
    }
    /*
    switch (entity[i].speed)
    {
            case 5:
                    for (j=0; j<2; j++)
                    {
                            ProcessEntity1(i);
            AnimateEntity(entity+i);
                    }
                    return;
            case 6:
                    for (j=0; j<3; j++)
                    {
                            ProcessEntity1(i);
            AnimateEntity(entity+i);
                    }
                    return;
            case 7:
                    for (j=0; j<4; j++)
                    {
                            ProcessEntity1(i);
            AnimateEntity(entity+i);
                    }
                    return;
    }
    */
}

void ProcessEntities() {
    int n;

    SiftEntities();  // in case people still want to affect only onscreen ents
    for (n = 0; n < entities; n++) {
        ProcessEntity(n);
    }
}

int FindCHR(const char *fname) {
    int n = 0;

    V_strlwr((char *)fname);
    for (n = 0; n < numchrs; n++) {
        if (!V_strcmp(chr->fname, fname)) return n;
    }

    return -1;
}

int AllocateEntity(int x1, int y1, const char *fname) {
    int n = 0;

    n = FindCHR(fname);
    entity[entities].chrindex = (byte)((n > -1) ? n : CacheCHR(fname));

    entity[entities].x = x1 * 16;
    entity[entities].y = y1 * 16;
    entity[entities].tx = (word)x1;
    entity[entities].ty = (word)y1;
    entity[entities].speed = 4;
    entity[entities].obsmode1 = 1;
    entity[entities].obsmode2 = 1;
    entity[entities].on = 1;
    entity[entities].visible = 1;

    return entities++;
}

void ChangeCHR(int who, const char *chrname) {
    int n = 0;

    if (who < 0 || who >= 256) Sys_Error("ChangeCHR: no such entity: %d", who);

    n = FindCHR(chrname);
    entity[who].chrindex = (byte)((n > -1) ? n : CacheCHR(chrname));
}
