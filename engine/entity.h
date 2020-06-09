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

#ifndef ENTITY_H
#define ENTITY_H

typedef struct {
    int x, y;                   // xwc, ywx position
    word tx, ty;                // xtc, ytc position
    byte facing;                // direction entity is facing
    byte moving, movecnt;       // direction entity is moving
    byte frame;                 // bottom-line frame to display
    byte specframe;             // special-frame set thingo
    byte chrindex, reset;       // CHR index | Reset animation
    byte obsmode1, obsmode2;    // can be obstructed | Is an obstruction
    byte speed, speedct;        // entity speed, speedcount :)
    byte delayct;               // animation frame-delay
    char *animofs, *scriptofs;  // anim script | move script
    byte face, actm;            // auto-face | activation mode
    byte movecode, movescript;  // movement type | movement script
    byte ctr, mode;             // sub-tile move ctr, mode flag (internal)
    word step, delay;           // step, delay
    word stepctr, delayctr;     // internal use counters
    word data1, data2, data3;   //
    word data4, data5, data6;   //
    int actscript;              // activation script
    byte on, visible;
    char expand1[2];
    // int  expand1;
    int expand2;           //
    int expand3, expand4;  //
    char desc[20];         // Entity description.
} entity_r;

typedef struct {
    byte *imagedata;     // CHR frame data
    int fxsize, fysize;  // frame x/y dimensions
    int hx, hy;          // x/y obstruction hotspot

    int hw, hh;

    int totalframes;  // total # of frames.
    int idle[4];

    char lanim[100];
    char ranim[100];
    char uanim[100];
    char danim[100];

    char fname[60];
} chrdata;

typedef struct { char t[60]; } chrlist_r;

extern chrlist_r chrlist[100];
extern byte nmchr, playernum;
extern entity_r *player;
extern entity_r entity[256];
extern chrdata chr[100];
extern byte entities, numchrs;
extern byte entidx[256];
extern byte cc;

extern void EntitySetFace(int ent, int face);

extern void RenderEntities();
extern void AnimateEntity(entity_r *p);
extern void ProcessEntities();
extern void SiftEntities();
extern void LoadCHRList();
extern void FreeCHRList();
// extern void EntityStat();
// extern void ListActiveEnts();
// extern void EntityS();
extern int EntityAt(int ex, int ey);
extern int EntityObsAt(int ex, int ey);
extern int ObstructionAt(int ex, int ey);
extern int CacheCHR(const char *fname);
extern int AllocateEntity(int x1, int y1, const char *fname);
extern void ChangeCHR(int who, const char *chrname);

#endif
