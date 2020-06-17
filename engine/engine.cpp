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
// �                      Main Game Engine module                        �
// �����������������������������������������������������������������������

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// CHANGELOG:
// <tSB Nov 8>
// + HookKey is now handled in key_game, instead of keyboard.cpp (which no
// longer exists anyway)
//   As an added benefit, hooked keys no longer activate VC when other VC is
//   already running.
// + removed keyboard.cpp and controls.cpp in favour of w_input.cpp. (class
// wrapper for DirectInput)
//   keyboard.h is just a list of keyboard codes now. :P
// <tSB Oct 31,00>
// + Too much to list! @_@
// <tSB Oct 30,00>
// + Minor tweaks here and there
// <tSB 5.19.00>
// + replaced WriteRaw24 with WriteBMP24
// + added support for version 4 VSPs (15bit uncompressed, from v2+i)
// <zero 5.7.99>
// + added ScreenShot() on F11
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#include "verge.h"
#include "wasm.h"

// ================================= Data ====================================

zoneinfo zones[256];         // zone data records
layer_r layer[6];            // Array of layer data
vspanim_r vspanim[100];      // tile animation data
unsigned short vadelay[100]; // Tile animation delay ctr

char mapname[60 + 1]; // MAP filename
char vspname[60 + 1]; // VSP filemap
char musname[60 + 1]; // MAP bkgrd music default filename
// char rstring[20];                    // render-order string
string_k rstring;
char numlayers; // number of layers in map
byte* obstruct = 0;
byte* zone = 0;      // obstruction and zone buffers
char layertoggle[8]; // layer visible toggles
word xstart, ystart; // MAP start x/y location
int bufsize;         // how many bytes need to be written
int numzones;        // number of active zones

word* layers[6];        // Raw layer data
int xwin = 0, ywin = 0; // camera offset

// -- vsp data --

byte *vsp = 0, *vspmask; // VSP data buffer.
unsigned short numtiles; // number of tiles in VSP.
unsigned short* tileidx; // tile index thingamajig
char* flipped;           // bi-direction looping flag

// entity stuffs

char* msbuf[100]; // ptr-table to script offset
char* ms = 0;     // script text buffer
byte nms;         // number of movescripts

char numfollowers = 0;    // number of party followers
byte follower[10];        // maximum of 10 followers.
char laststeps[10] = {0}; // record of last movements
int lastent;

// -- stuff --

byte movegranularity; // means nothing now, please remove
byte movectr = 0;

byte phantom = 0;    // walk-through-walls
byte speeddemon = 0; // doublespeed cheat

// <aen>
// This *MUST* have 256 elements, because of the new input code. I generate my
// own
// codes for control keys and others, such as ENTER, ESC, ALT, etc. And their
// codes are
// up in the 200's.
// <tSB>
// Still needs 256 elements, but because of DirectInput.

int bindarray[256]; // bind script offset

// ================================= Code ====================================

int ReadCompressedLayer1(byte* dest, int len, char* buf) {
    byte run, w;

    do {
        run = 1;
        w = *buf++;
        if (0xFF == w) {
            run = *buf++;
            w = *buf++;
        }
        len -= run;
        // totally bogus. shaa.
        if (len < 0)
            return 1;
        while (run--)
            *dest++ = w;
    } while (len);

    // good
    return 0;
}

int ReadCompressedLayer2(word* dest, int len, word* buf) {
    word run, w;

    do {
        run = 1;
        w = *buf++;
        if ((w & 0xFF00) == 0xFF00) {
            run = (word)(w & 0x00FF);
            w = *buf++;
        }
        len -= run;
        // totally bogus. shaa.
        if (len < 0)
            return 1;
        while (run--)
            *dest++ = w;
    } while (len);

    // good
    return 0;
}

void LoadVSP(const char* fname) {
    VFILE* f;
    int n, bufsize;
    word ver;
    char* cb;
    word* c; // --tSB
    word r, g, b;

    // Mwahaha! The Indefatigable Grue has snuck into the V2 source code! It is
    // forever corrupted by his evil touch! Cower in fear, oh yes, FEAR!
    // MwahahaHA..ha...hem...
    // FIXME: 8 bit palettes are getting mangled in here somewhere :P

    f = vopen(fname);
    if (!f)
        Sys_Error("*error* Could not open VSP file %s.", fname);

    vread(&ver, 2, f);
    if (ver > 5)
        Sys_Error("Invalid VSP %s", fname);
    if (gfx.bpp < 2 && ver > 3)
        Sys_Error("Unable to load hicolour VSPs in 8bit mode");

    // vsp's, and thus map's, palette
    if (ver < 4) // no palette in v4&5 VSPs - tSB
        vread(gfx.gamepal, 3 * 256, f);

    vread(&numtiles, 2, f);
    if (numtiles < 1)
        numtiles = 1;
    if (ver < 4)
        vsp = (byte*)valloc(256 * numtiles, "vsp", OID_IMAGE);
    else
        vsp = (byte*)valloc(512 * numtiles, "vsp (hicolor)", OID_IMAGE);

    switch (ver) {
    case 2:
        vread(vsp, 256 * numtiles, f); // old version; raw data
        break;
    case 3:
        vread(&n, 4, f); // compressed version
        cb = (char*)valloc(n, "LoadVSP:cb", OID_TEMP);
        vread(cb, n, f);
        n = ReadCompressedLayer1(vsp, 16 * 16 * numtiles, cb);
        if (n) {
            Sys_Error("LoadVSP: %s: bogus compressed image data", fname);
        }
        vfree(cb);
        break;
    case 4:
        vread(vsp, numtiles * 512, f); // hicolor uncompressed (from v2+i) - tSB

        for (n = 0; n < numtiles * 256; n++) {
            b = ((word*)vsp)[n];
            if (b) {
                r = (word)(b >> 10);
                g = (word)((b >> 5) & 31);
                b = (word)(b & 31);
                // Gah! @_@
                ((word*)vsp)[n] =
                    (r << 11) + (g << 6) +
                    b; // v4 vsps are 1:5:5:5, 16bit color is 5:6:5
            } else
                ((word*)vsp)[n] = gfx.trans_mask;
        }
        break;
    case 5:
        vread(&bufsize, 4, f);
        cb = (char*)valloc(bufsize, "LoadVSP: cb", OID_TEMP); // compressed
        vread(cb, bufsize, f);
        if (gfx.bpp > 1) {
            n = ReadCompressedLayer2((word*)vsp, numtiles * 256, (word*)cb);
            if (n)
                Sys_Error("LoadVSP: %s: bogus compressed image data", fname);
            vfree(cb);
        }
        break;

    default:
        Sys_Error("LoadVSP: %s: bogus version: %d", fname, ver);
    }

    gfx.SetPalette(gfx.gamepal);
    // adjust data for currene pixel depth
    if (ver < 4) {
        // nothing needs to happen to an 8bit VSP if we're in 8bit mode, so
        // leave it
        // alone.
        if (gfx.bpp == 2) {
            c = (word*)valloc(16 * 16 * numtiles * 2,
                "LoadVSP: Tile Data (hicolour)", OID_IMAGE);
            for (n = 0; n < 16 * 16 * numtiles; n++) {
                c[n] = gfx.Conv8(vsp[n]); // Do it!
            }
            vfree(vsp);     // dump the 8bit data
            vsp = (byte*)c; // and keep the hicolour data
        }
    } else {
        // we'll assume we're in hicolour mode by this point
        // TODO: allow hicolour VSPs in 8bit mode? (do some
        // closest-matching-colour
        // type stuff)
        for (n = 0; n < 16 * 16 * numtiles; n++)
            ((word*)vsp)[n] = gfx.Conv16(((word*)vsp)[n]);
    }

    // animation strand data
    vread(vspanim, sizeof(vspanim), f);

    vclose(f);

    // now calculate VSP transparency mask.
    vspmask = 0;
    /*
    vspmask=(byte *) valloc(16*16*numtiles, "vspmask", OID_MISC);
    for (n=0; n<16*16*numtiles; n++)
            vspmask[n]=vsp[n]?0:255;
    */

    // allocate and build tileidx.
    tileidx = (unsigned short*)valloc(2 * numtiles, "tileidx", OID_MISC);
    for (n = 0; n < numtiles; n++)
        tileidx[n] = (unsigned short)n;

    // for ping-pong mode
    flipped = (char*)valloc(numtiles, "flipped", OID_MISC);

    animate = TRUE;
}

// Yeah, I'm paranoid, so WHAT?! --tSB
void FreeVSP() {
    animate = FALSE;

    if (flipped != NULL) {
        vfree(flipped);
        flipped = NULL;
    }
    if (tileidx != NULL) {
        vfree(tileidx);
        tileidx = NULL;
    }
    if (vspmask != NULL) {
        vfree(vspmask);
        vspmask = NULL;
    }
    if (vsp != NULL) {
        vfree(vsp);
        vsp = NULL;
    }
    if (mapvc != NULL) {
        vfree(mapvc);
        mapvc = NULL;
    }
}

void LoadMAP(const char* fname) {
    VFILE* f;
    char* cb;
    int i;
    int bogus; // for checking ReadCompressedLayer*
    char sig[6 + 1];
    char rstring_temp[20 + 1];

    // No matter where you go, you're there.

    Log(va("Loading MAP %s.", fname));
    V_strncpy(mapname, fname, 60);
    mapname[60] = '\0';

    f = vopen(fname);
    if (!f)
        Sys_Error("Could not find %s.", fname);

    vread(sig, 6, f);
    sig[6] = '\0';
    static const char header[] = { 'M', 'A', 'P', char(0xF9), '5', 0 };
    if (V_strcmp(sig, header)) {
        Sys_Error("%s is not a recognized MAP file.", fname);
    }

    // Lalala! Can you find Waldo hiding in the code? Here's a hint, he likes to
    // dress like a candy-cane.

    vread(&i, 4, f);
    vread(vspname, 60, f);
    vread(musname, 60, f);

    vread(rstring_temp, 20, f);
    rstring_temp[20] = '\0';
    rstring = rstring_temp;

    vread(&xstart, 2, f);
    vread(&ystart, 2, f);
    vseek(f, 51, SEEK_CUR);
    // vread(strbuf, 51, f);

    vread(&numlayers, 1, f);
    for (i = 0; i < numlayers; i++)
        vread(&layer[i], 12, f);

    V_memset(layertoggle, 0, 8);
    // read actual layer data.
    for (i = 0; i < numlayers; i++) {
        vread(&bufsize, 4, f);
        layers[i] =
            (unsigned short*)valloc(layer[i].sizex * (layer[i].sizey + 2) * 2,
                "LoadMAP:layers[i]", OID_MAP);
        cb = (char*)valloc(bufsize, "LoadMAP:cb", OID_TEMP);
        vread(cb, bufsize, f);
        bogus = ReadCompressedLayer2(
            layers[i], (layer[i].sizex * layer[i].sizey), (word*)cb);
        if (bogus) {
            Sys_Error("LoadMAP: %s: bogus compressed layer data (layer %d)",
                fname, i);
        }
        vfree(cb);
        layertoggle[i] = 1;
    }

    obstruct = (byte*)valloc(
        layer[0].sizex * (layer[0].sizey + 2), "obstruct", OID_MAP);
    zone =
        (byte*)valloc(layer[0].sizex * (layer[0].sizey + 2), "zone", OID_MAP);

    // read obstruction grid
    vread(&bufsize, 4, f);
    cb = (char*)valloc(bufsize, "LoadMAP:cb (2)", OID_TEMP);
    vread(cb, bufsize, f);
    bogus =
        ReadCompressedLayer1(obstruct, (layer[0].sizex * layer[0].sizey), cb);
    if (bogus) {
        Sys_Error("LoadMAP: %s: bogus compressed obstruction data", fname);
    }
    vfree(cb);

    // zone grid
    vread(&bufsize, 4, f);
    cb = (char*)valloc(bufsize, "LoadMAP:cb (3)", OID_TEMP);
    vread(cb, bufsize, f);
    bogus = ReadCompressedLayer1(zone, (layer[0].sizex * layer[0].sizey), cb);
    if (bogus) {
        Sys_Error("LoadMAP: %s: bogus compressed zone data", fname);
    }
    vfree(cb);
    V_memset(zones, 0, sizeof zones);
    vread(&numzones, 4, f);
    vread(zones, numzones * 50, f);

    V_memset(chrlist, 0, sizeof chrlist);
    vread(&nmchr, 1, f);
    vread(chrlist, 60 * nmchr, f);

    // Cheese is good, cheese is nice. Cheese is better, than body lice.

    V_memset(entity, 0, sizeof entity);
    vread(&entities, 1, f);
    vread(entity, (sizeof(entity) / 256 * entities), f);
    for (i = 0; i < entities; i++) {
        entity[i].tx = (short)entity[i].x;
        entity[i].ty = (short)entity[i].y;
        entity[i].x *= 16;
        entity[i].y *= 16;
        entity[i].visible = 1;
        entity[i].on = 1;
    }

    vread(&nms, 1, f);
    vread(&i, 4, f);
    vread(msbuf, nms * 4, f);
    if (nms) {
        ms = (char*)valloc(i, "LoadMAP:ms", OID_MAP);
        vread(ms, i, f);
    } else {
        vseek(f, i, 0);
        ms = (char*)valloc(16, "LoadMAP:ms", OID_MAP);
    }
    vread(&i, 4, f); // # of things
    LoadMapVC(f);
    vclose(f);

    V_memset(chr, 0, sizeof chr);
    LoadVSP(vspname);
    LoadCHRList();
    Log(va("Loaded %d map events ", mapevents));
    if (V_strlen(musname))
        PlayMusic(musname);
    ExecuteEvent(0);
    timer_count = 0;
}

void FreeMAP() {
    int n;

    if (obstruct)
        vfree(obstruct);
    if (zone)
        vfree(zone);
    for (n = 0; n < numlayers; n += 1) {
        if (layers[n] != NULL) {
            vfree(layers[n]);
            layers[n] = NULL;
        }
    }
    if (ms != NULL) {
        vfree(ms);
        ms = NULL;
    }

    player = 0;
    V_memset(entity, 0, sizeof entity);
    entities = 0;
    nmchr = 0;
    numchrs = 0;
    xwin = 0;
    ywin = 0;

    if (mapvc != NULL) {
        vfree(mapvc);
        mapvc = NULL;
    }
}

int PlayerObstructed(char dir) {
    if (phantom)
        return 0;
    switch (dir) {
    case 0:
        if (ObstructionAt(player->tx, player->ty + 1))
            return 1;
        break;
    case 1:
        if (ObstructionAt(player->tx, player->ty - 1))
            return 1;
        break;
    case 2:
        if (ObstructionAt(player->tx - 1, player->ty))
            return 1;
        break;
    case 3:
        if (ObstructionAt(player->tx + 1, player->ty))
            return 1;
        break;
    }
    switch (dir) {
    case 0:
        if (EntityObsAt(player->tx, player->ty + 1))
            return 1;
        break;
    case 1:
        if (EntityObsAt(player->tx, player->ty - 1))
            return 1;
        break;
    case 2:
        if (EntityObsAt(player->tx - 1, player->ty))
            return 1;
        break;
    case 3:
        if (EntityObsAt(player->tx + 1, player->ty))
            return 1;
        break;
    }
    return 0;
}

void CheckZone() {
    int curzone;
    static int lz = 0, zonedelay = 0;

    curzone = zone[(player->ty * layer[0].sizex) + player->tx];
    if (lz != curzone) {
        zonedelay = 0;
        lz = curzone;
    }
    if (!zones[curzone].percent)
        return;
    if (zonedelay < zones[curzone].delay) {
        zonedelay++;
        return;
    }
    if (curzone)
        if (zones[curzone].script && zones[curzone].percent >= rnd(0, 255)) {
            ExecuteEvent(zones[curzone].script);
            timer_count = 0;
            zonedelay = 0;
        }
}

int InvFace() {
    switch (player->facing) {
    case 0:
        return 1;
    case 1:
        return 0;
    case 2:
        return 3;
    case 3:
        return 2;
    }
    return -1;
}

void Activate() {
    byte tz;
    int ax = 0, ay = 0;

    switch (player->facing) {
    case 0:
        ax = player->tx;
        ay = player->ty + 1;
        break;
    case 1:
        ax = player->tx;
        ay = player->ty - 1;
        break;
    case 2:
        ax = player->tx - 1;
        ay = player->ty;
        break;
    case 3:
        ax = player->tx + 1;
        ay = player->ty;
        break;
    }
    tz = (byte)EntityAt(ax, ay);
    if (tz) {
        tz--;
        if (entity[tz].face) {
            // entity[tz].facing=(byte)InvFace();
            EntitySetFace(tz, InvFace());
            AnimateEntity(&entity[tz]);
        }
        if (entity[tz].actscript) {
            lastent = tz;
            ExecuteEvent(entity[tz].actscript);
            timer_count = 0;
            return;
        }
    }
    tz = zone[(ay * layer[0].sizex) + ax];
    if (zones[tz].aaa) {
        ExecuteEvent(zones[tz].script);
        timer_count = 0;
    }
}

void ResetFollowers() {
    int i;

    player->x = player->tx << 4;
    player->y = player->ty << 4;
    player->moving = 0;
    player->movecnt = 0;
    player->reset = 1;
    for (i = 0; i < numfollowers; i++) {
        entity[follower[i]].x = player->x;
        entity[follower[i]].y = player->y;
        entity[follower[i]].tx = player->tx;
        entity[follower[i]].ty = player->ty;
        // entity[follower[i]].facing=player->facing;
        EntitySetFace(follower[i], player->facing);
        entity[follower[i]].reset = 1;
    }
    V_memset(laststeps, 0, 10);
}

void MoveFollowers() {
    int i;

    for (i = 0; i < numfollowers; i++) {
        entity[follower[i]].moving = laststeps[i + 1];
        entity[follower[i]].movecnt = 15;
        if (entity[follower[i]].reset ||
            entity[follower[i]].facing != laststeps[i + 1] - 1) {
            // player->animofs=chr[player->chrindex].uanim;
            entity[follower[i]].delayct = 0;
            entity[follower[i]].reset = 0;
        }
        // entity[follower[i]].facing=(byte)(laststeps[i+1]-1);
        EntitySetFace(follower[i], laststeps[i + 1] - 1);
    }
}

//--- zero 5.7.99

void WritePalette(FILE* f) {
    int n;
    unsigned char write_pal[3 * 256];

    for (n = 0; n < 3 * 256; n += 1)
        write_pal[n] = (unsigned char)(gfx.pal[n] << 2);

    // b=12; fwrite(&b, 1, 1, f);
    fputc(12, f);
    // fwrite(pal, 1, 768, f);
    fwrite(write_pal, 3, 256, f);
}

void WritePCXLine(unsigned char* p, int len, FILE* pcxf) {
    int i;
    unsigned char byt, samect, repcode;

    i = 0;
    do {
        byt = p[i++];
        samect = 1;
        while (samect < (unsigned)63 && i < len && byt == p[i]) {
            samect++;
            i++;
        }
        if (samect > 1 || (byt & 0xC0) != 0) {
            repcode = (unsigned char)(0xC0 | samect);
            fwrite(&repcode, 1, 1, pcxf);
        }
        fwrite(&byt, 1, 1, pcxf);
    } while (i < len);
}

void WriteBMP24(void) // the Speed Bump
{
    int n;
    word w;
    word* scr;

    int x, y;
    unsigned int i;
    char fnamestr[13];
    FILE* bmpf;

    n = 0;
    do {
        sprintf(fnamestr, "%d.bmp", n);
        bmpf = _fopen(fnamestr, "r");
        i = (int)bmpf;
        if (bmpf)
            fclose(bmpf);
        n++;
    } while (i);
    n--;

    // Takes a snapshot of the current screen.

    sprintf(fnamestr, "%d.bmp", n);
    bmpf = _fopen(fnamestr, "wb");

    w = 19778;
    fwrite(&w, 1, 2, bmpf); // file marker, always = 'BM'
    i = (gfx.scrx * gfx.scry * 3) + 54;
    fwrite(&i, 1, 4, bmpf); // size of file=size of screen + size of header
    w = 0;
    fwrite(&w, 1, 2, bmpf); // reserved1
    w = 0;
    fwrite(&w, 1, 2, bmpf); // reserved2
    i = 54;
    fwrite(&i, 1, 4, bmpf); // bmp data is right after the header
    i = 40;
    fwrite(&i, 1, 4, bmpf); // size of image
    i = gfx.scrx;
    fwrite(&i, 1, 4, bmpf); // bmp width
    i = gfx.scry;
    fwrite(&i, 1, 4, bmpf); // bmp height
    w = 1;
    fwrite(&w, 1, 2, bmpf); // just one color plane
    w = 24;
    fwrite(&w, 1, 2, bmpf); // 24 bit image
    i = 0;
    fwrite(&i, 1, 4, bmpf); // no compression
    i = gfx.scrx * gfx.scry * 3;
    fwrite(&i, 1, 4, bmpf);
    i = 0;
    fwrite(&i, 1, 4, bmpf);
    i = 0;
    fwrite(&i, 1, 4, bmpf);
    i = 0;
    fwrite(&i, 1, 4, bmpf);
    i = 0;
    fwrite(&i, 1, 4, bmpf); // all the colors are important

    fseek(bmpf, 54, SEEK_SET);
    y = gfx.scry;

    //  gfx.Lock();
    scr = (word*)gfx.screen;

    while (y > 0) {
        for (x = 0; x < gfx.scrx; x++) {
            int r, g, b;
            w = scr[(y - 1) * gfx.scrx + x];

            gfx.UnPackPixel(w, r, g, b);
            putc(b, bmpf);
            putc(g, bmpf);
            putc(r, bmpf);
        }
        y--;
    }
    //  gfx.UnLock();

    fclose(bmpf);
    wasm_syncFileSystem();
}

static int ss = 0;

void ScreenShot() {
    if (gfx.bpp > 1) {
        WriteBMP24();
        return;
    }

    unsigned char b1;
    unsigned short w1;
    int i, n;
    char fnamestr[13];
    FILE* pcxf;

    n = 0;
    do {
        sprintf(fnamestr, "%d.pcx", n);
        pcxf = _fopen(fnamestr, "r");
        i = (int)pcxf;
        if (pcxf)
            fclose(pcxf);
        n++;
    } while (i);
    n--;

    // Takes a snapshot of the current screen.

    sprintf(fnamestr, "%d.pcx", n);

    pcxf = _fopen(fnamestr, "wb");
    ss++;

    // Write PCX header

    b1 = 10;
    fwrite(&b1, 1, 1, pcxf); // manufacturer always = 10
    b1 = 5;
    fwrite(&b1, 1, 1, pcxf); // version = 3.0, >16 colors
    b1 = 1;
    fwrite(&b1, 1, 1, pcxf); // encoding always = 1
    b1 = 8;
    fwrite(&b1, 1, 1, pcxf); // 8 bits per pixel, for 256 colors
    w1 = 0;
    fwrite(&w1, 1, 2, pcxf); // xmin = 0;
    w1 = 0;
    fwrite(&w1, 1, 2, pcxf); // ymin = 0;
    w1 = (unsigned short)(gfx.scrx - 1);
    fwrite(&w1, 1, 2, pcxf); // xmax = 319;
    w1 = (unsigned short)(gfx.scry - 1);
    fwrite(&w1, 1, 2, pcxf); // ymax = 199;
    w1 = (unsigned short)gfx.scrx;
    fwrite(&w1, 1, 2, pcxf); // hres = 320;
    w1 = (unsigned short)gfx.scry;
    fwrite(&w1, 1, 2, pcxf); // vres = 200;

    fwrite(gfx.screen, 1, 48, pcxf);

    b1 = 0;
    fwrite(&b1, 1, 1, pcxf); // reserved always = 0.
    b1 = 1;
    fwrite(&b1, 1, 1, pcxf); // number of color planes. Just 1 for 8bit.
    w1 = (unsigned short)gfx.scrx;
    fwrite(&w1, 1, 2, pcxf); // number of bytes per line

    w1 = 0;
    fwrite(&w1, 1, 1, pcxf);
    // fwrite(screen, 1, 59, pcxf);          // filler
    for (w1 = 0; w1 < 59; w1++)
        fputc(0, pcxf);

    for (w1 = 0; w1 < gfx.scry; w1++)
        WritePCXLine(gfx.screen + w1 * gfx.scrx, gfx.scrx, pcxf);

    WritePalette(pcxf);
    fclose(pcxf);

    wasm_syncFileSystem();
    // timer_count=0;
}
//---

void LastMove(char dir) {
    laststeps[9] = laststeps[8];
    laststeps[8] = laststeps[7];
    laststeps[7] = laststeps[6];
    laststeps[6] = laststeps[5];
    laststeps[5] = laststeps[4];
    laststeps[4] = laststeps[3];
    laststeps[3] = laststeps[2];
    laststeps[2] = laststeps[1];
    laststeps[1] = laststeps[0];
    laststeps[0] = dir;

    MoveFollowers();
}

// This is called in ProcessControls1, instead of the keyboard handler itself.
void Key_Game() {
    input.Poll();
    input.Update();

    // HookKey'd stuff
    while (char c = input.GetKey())
        if (bindarray[c])
            HookKey(bindarray[c]);

    if (input.key[DIK_GRAVE]) {
        Console_Activate();
    }

    if (input.key[DIK_LMENU] && input.key[DIK_X])
        Sys_Error("");
    // FIXME: it doesn't work. :P (switching to windowed mode results in a
    // window,
    // but no change in display mode, switching to fullscreen results in a
    // crash)
    /* if (input.key[DIK_LALT] && input.key[DIK_ENTER])
      {
       gfx.SetMode(0,0,gfx.bpp*8,!gfx.IsFullScreen());
       input.key[DIK_ENTER]=0;
       input.key[DIK_LALT]=0;
      }*/
}

void ProcessControls1() {
    if (!player->moving) {
        // at this point, player is not moving
        Key_Game(); // update keyboard and stuff
        if (input.down) {
            if (PlayerObstructed(0)) {
                player->animofs = 0;
                player->facing = 0;
                return;
            }

            if (player->facing != 0) {
                player->animofs = 0; // chr[player->chrindex].danim;
                                     // player->delayct = 0;
            }

            player->facing = 0;
            player->moving = 1, player->movecnt = 15;
            player->ty++, player->y++;
        } else if (input.up) {
            if (PlayerObstructed(1)) {
                player->animofs = 0;
                player->facing = 1;
                return;
            }

            if (player->facing != 1) {
                player->animofs = 0; // chr[player->chrindex].uanim;
                                     // player->delayct = 0;
            }

            player->facing = 1;
            player->moving = 2, player->movecnt = 15;
            player->ty--, player->y--;
        } else if (input.left) {
            if (PlayerObstructed(2)) {
                player->animofs = 0;
                player->facing = 2;
                return;
            }

            if (player->facing != 2) {
                player->animofs = 0; // chr[player->chrindex].lanim;
                                     // player->delayct = 0;
            }

            player->facing = 2;
            player->moving = 3, player->movecnt = 15;
            player->tx--, player->x--;
        } else if (input.right) {
            if (PlayerObstructed(3)) {
                player->animofs = 0;
                player->facing = 3;
                return;
            }

            if (player->facing != 3) {
                player->animofs = 0; // chr[player->chrindex].ranim;
                                     // player->delayct = 0;
            }

            player->facing = 3;
            player->moving = 4, player->movecnt = 15;
            player->tx++, player->x++;
        }
    } // !player moving
    /*
    if (!player->moving)
    {
      if (down && !PlayerObstructed(0))
      {
        player->ty++;
        player->moving=1;
        player->movecnt=15;
        player->y++;
        if (player->reset || player->facing != 0)
        {
          player->animofs=chr[player->chrindex].danim;
          player->delayct=0;
          player->reset=0;
        }
        player->facing=0;
        LastMove(1);
        return;
      }
      if (up && !PlayerObstructed(1))
      {
        player->ty--;
        player->moving=2;
        player->movecnt=15;
        player->y--;
        if (player->reset || player->facing != 1)
        {
          player->animofs=chr[player->chrindex].uanim;
          player->delayct=0;
          player->reset=0;
        }
        player->facing=1;
        LastMove(2);
        return;
      }
      if (left && !PlayerObstructed(2))
      {
        player->tx--;
        player->moving=3;
        player->movecnt=15;
        player->x--;
        if (player->reset || player->facing != 2)
        {
          player->animofs=chr[player->chrindex].lanim;
          player->delayct=0;
          player->reset=0;
        }
        player->facing=2;
        LastMove(3);
        return;
      }
      if (right && !PlayerObstructed(3))
      {
        player->tx++;
        player->moving=4;
        player->movecnt=15;
        player->x++;
        if (player->reset || player->facing != 3)
        {
          player->animofs=chr[player->chrindex].ranim;
          player->delayct=0;
          player->reset=0;
        }
        player->facing=3;
        LastMove(4);
        return;
      }
      if (down) player->facing=0;
      if (up) player->facing=1;
      if (left) player->facing=2;
      if (right) player->facing=3;
    }
    */

    switch (player->moving) {
    case 0:
        player->reset = 1;
        player->animofs = 0;
        player->delayct = 0;
        break;
    case 1:
        player->y++;
        player->movecnt--;
        break;
    case 2:
        player->y--;
        player->movecnt--;
        break;
    case 3:
        player->x--;
        player->movecnt--;
        break;
    case 4:
        player->x++;
        player->movecnt--;
        break;
    }

    if (!player->movecnt && player->moving) {
        player->moving = 0;
        CheckZone();
    }

    if (!player->movecnt && input.b1)
        Activate();
}

void ProcessControls() {
    if (!player)
        return;

    ProcessControls1();
    AnimateEntity(player);
}

void GameTick() {
    input.Update();

    ProcessControls();
    if (speeddemon && input.key[DIK_LCONTROL])
        ProcessControls();
    ProcessEntities();
}
