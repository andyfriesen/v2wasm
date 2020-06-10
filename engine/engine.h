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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <zero 5.7.99>
// + added ScreenShot() headers
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef ENGINE_H
#define ENGINE_H

typedef struct {
    word start;  // strand start
    word finish; // strand end
    word delay;  // tile-switch delay
    word mode;   // tile-animation mode
} vspanim_r;

typedef struct {
    char pmultx, pdivx;          // parallax multiplier/divisor for X
    char pmulty, pdivy;          // parallax multiplier/divisor for Y
    unsigned short sizex, sizey; // layer dimensions.
    unsigned char trans, hline;  // transparency flag | hline (raster fx)
} layer_r;

typedef struct {
    char name[40];               // zone name/desc
    unsigned short script;       // script to call thingy
    unsigned short percent;      // chance of executing
    unsigned short delay;        // step-delay
    unsigned short aaa;          // Accept Adjacent Activation
    unsigned short entityscript; // script to call for entities
} zoneinfo;

extern zoneinfo zones[256];         // zone data records
extern layer_r layer[6];            // Array of layer data
extern vspanim_r vspanim[100];      // tile animation data
extern unsigned short vadelay[100]; // Tile animation delay ctr

extern char mapname[61]; // MAP filename
extern char vspname[61]; // VSP filemap

// extern char rstring[20];                    // render-order string
extern string_k rstring;

extern char numlayers;        // number of layers in map
extern byte *obstruct, *zone; // obstruction and zone buffers
extern int bufsize;           // how many bytes need to be written
extern char layertoggle[8];   // layer visible toggles

extern word* layers[6]; // Raw layer data
extern int xwin, ywin;

// -- entity things --

extern char* msbuf[100]; // ptr-table to script offset
extern char* ms;         // script text buffer
extern byte nms;         // number of movescripts

extern char numfollowers;  // number of party followers
extern byte follower[10];  // maximum of 10 followers.
extern char laststeps[10]; // record of last movements
extern int lastent;

extern int numzones;

// -- vsp data --

extern byte *vsp, *vspmask;     // VSP data buffer.
extern unsigned short numtiles; // number of tiles in VSP.
extern unsigned short* tileidx; // tile index thingamajig
extern char* flipped;           // bi-direction looping flag
extern byte movegranularity, phantom, speeddemon, movectr;

// <aen>
// This *MUST* have 256 elements, because of the new input code. I generate my
// own
// codes for control keys and others, such as ENTER, ESC, ALT, etc. And their
// codes are
// up in the 200's.
// <tSB>
// DirectInput uses the high 128 elements for extended keys and stuff, so it
// still has to be 256 elements, just for a different reason. ;)

extern int bindarray[256];

// -- prototypes --

extern void LoadVSP(const char* fname);
extern void FreeVSP(void);
extern void LoadMAP(const char* fname);
extern void FreeMAP(void);
extern void MAPswitch(void);
extern void MAPstats(void);
extern void ProcessControls(void);
extern void GameTick(void);

extern int ReadCompressedLayer1(byte*, int, char*);
extern int ReadCompressedLayer2(word*, int, word*);

//--- zero 5.7.99
#include <stdio.h>
void WritePalette(FILE* f);
void WritePCXLine(unsigned char* p, int len, FILE* pcxf);
void ScreenShot();
//
#endif // ENGINE_H
