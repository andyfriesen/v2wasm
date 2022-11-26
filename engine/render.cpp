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

// ┌─────────────────────────────────────────────────────────────────────┐
// │                          The VERGE Engine                           │
// │              Copyright (C)1998 BJ Eirich (aka vecna)                │
// │                          Rendering module                           │
// └─────────────────────────────────────────────────────────────────────┘

#include "verge.h"
#include <math.h>

// INTERFACE DATA
// //////////////////////////////////////////////////////////////////////////////////

unsigned char animate = 0;
unsigned char cameratracking = 1;
unsigned char tracker = 0;
unsigned char showobs = 0;
unsigned char showzone = 0;

// IMPLEMENTATION DATA
// /////////////////////////////////////////////////////////////////////////////

static unsigned char curlayer = 0;
byte inside = 0;

// IMPLEMENTATION CODE
// /////////////////////////////////////////////////////////////////////////////

//#pragma off (unreferenced);

void Map_BlitLayer(int lay, int masked, int color_mapped) {
    int c;
    int x;
    int y;
    int x_sub;
    int y_sub;
    int clip_width;
    int clip_length;
    unsigned short* source;

    // validate arguments
    if (lay < 0 || lay >= numlayers)
        return;

    // is this layer visible?
    if (!layertoggle[lay])
        return;

    // adjust view according to parallax
    x = xwin * layer[lay].pmultx / layer[lay].pdivx;
    y = ywin * layer[lay].pmulty / layer[lay].pdivy;

    // make my life easier; don't allow scrolling past map edges
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    // get subtile position while we still have pixel precision
    x_sub = -(x & 15);
    y_sub = -(y & 15);
    // determine upper left tile coords of camera
    x >>= 4;
    y >>= 4;

    // calculate tiled rows and columns
    clip_width = (gfx.scrx + 31) / 16;
    clip_length = (gfx.scry + 31) / 16;

    // safeguard; this should never happen due to camera bounding
    // ie. if a map is set as visible, there should be something to draw at all
    // times
    if (x + clip_width - 1 < 0 || x >= layer[lay].sizex ||
        y + clip_length - 1 < 0 || y >= layer[lay].sizey) {
        return;
    }

    // clip upper left
    if (x + clip_width - 1 >= layer[lay].sizex)
        clip_width = layer[lay].sizex - x;
    if (y + clip_length - 1 >= layer[lay].sizey)
        clip_length = layer[lay].sizey - y;

    // clip lower right
    if (x < 0) {
        clip_width += x;
        x = 0;
    }
    if (y < 0) {
        clip_length += y;
        y = 0;
    }

    source = layers[lay] + y * layer[lay].sizex + x;
    y = y_sub;
    do {
        y_sub = x_sub; // don't try this at home
        x = clip_width;
        int xcount = 0;
        do {
            // validate tile request
            c = *source;
            if (c < 0 || c >= numtiles)
                c = 0;
            // validate it again
            c = tileidx[c];
            if (c >= 0 && c < numtiles) {
                if (curlayer) {
                    if (c) {
                        if (masked) {
                            if (color_mapped)
                                gfx.TCopySpriteLucent(y_sub, y, 16, 16,
                                    vsp + (c << 8) * gfx.bpp, color_mapped);
                            else
                                gfx.TCopySprite(
                                    y_sub, y, 16, 16, vsp + (c << 8) * gfx.bpp);
                        } else {
                            if (color_mapped)
                                gfx.CopySpriteLucent(y_sub, y, 16, 16,
                                    vsp + (c << 8) * gfx.bpp, color_mapped);
                            else
                                gfx.CopySprite(
                                    y_sub, y, 16, 16, vsp + (c << 8) * gfx.bpp);
                        }
                    }
                } else {
                    gfx.CopySprite(y_sub, y, 16, 16, vsp + (c << 8) * gfx.bpp);
                }
            }
            source += 1;
            x -= 1;
            y_sub += 16; // x screen position
        } while (x);
        source += (layer[lay].sizex - clip_width);
        y += 16;
        clip_length -= 1;
    } while (clip_length);
}

// INTERFACE CODE
// //////////////////////////////////////////////////////////////////////////////////

void BlitLayer(int lay) {
    if (lay < 0 || lay >= numlayers)
        return;

    // hline takes precedence
    if (layer[lay].hline) {
        ExecuteEvent(layer[lay].hline);
    }
    // solid / mask / color_mapped are backseat
    else {
        Map_BlitLayer(lay, curlayer, layer[lay].trans);
    }
}

void DrawObstructions() {
    int x;
    int y;
    int x_sub;
    int y_sub;
    int clip_width;
    int clip_length;
    unsigned char* source;

    // debugging for now
    //	if (gfx.bpp>1) return;

    // adjust view according to parallax
    x = xwin * layer[0].pmultx / layer[0].pdivx;
    y = ywin * layer[0].pmulty / layer[0].pdivy;

    // make my life easier; don't allow scrolling past map edges
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    // get subtile position while we still have pixel precision
    x_sub = -(x & 15);
    y_sub = -(y & 15);
    // determine upper left tile coords of camera
    x >>= 4;
    y >>= 4;

    // calculate tiled rows and columns
    clip_width = (gfx.scrx + 31) / 16;
    clip_length = (gfx.scry + 31) / 16;

    // safeguard; this should never happen due to camera bounding
    // ie. if a map is set as visible, there should be something to draw at all
    // times
    if (x + clip_width - 1 < 0 || x >= layer[0].sizex ||
        y + clip_length - 1 < 0 || y >= layer[0].sizey) {
        return;
    }

    // clip upper left
    if (x + clip_width - 1 >= layer[0].sizex)
        clip_width = layer[0].sizex - x;
    if (y + clip_length - 1 >= layer[0].sizey)
        clip_length = layer[0].sizey - y;

    // clip lower right
    if (x < 0) {
        clip_width += x;
        x = 0;
    }
    if (y < 0) {
        clip_length += y;
        y = 0;
    }

    source = obstruct + y * layer[0].sizex + x;
    y = y_sub;
    do {
        y_sub = x_sub; // don't try this at home
        x = clip_width;
        int xcount = 0;
        do {
            if (*source)
                gfx.BlitStipple(y_sub, y, *source);

            source += 1;
            x -= 1;
            y_sub += 16; // x screen position
        } while (x);
        source += (layer[0].sizex - clip_width);
        y += 16;
        clip_length -= 1;
    } while (clip_length);
}

void DrawZones() {
    int x;
    int y;
    int x_sub;
    int y_sub;
    int temp;
    int clip_width;
    int clip_length;
    unsigned char* source;

    // debugging for now
    //	if (gfx.bpp>1) return;

    // adjust view according to parallax
    x = xwin * layer[0].pmultx / layer[0].pdivx;
    y = ywin * layer[0].pmulty / layer[0].pdivy;

    // make my life easier; don't allow scrolling past map edges
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    // get subtile position while we still have pixel precision
    x_sub = -(x & 15);
    y_sub = -(y & 15);
    // determine upper left tile coords of camera
    x >>= 4;
    y >>= 4;

    // calculate tiled rows and columns
    clip_width = (gfx.XRes() + 31) / 16;
    clip_length = (gfx.YRes() + 31) / 16;

    // safeguard; this should never happen due to camera bounding
    // ie. if a map is set as visible, there should be something to draw at all
    // times
    if (x + clip_width - 1 < 0 || x >= layer[0].sizex ||
        y + clip_length - 1 < 0 || y >= layer[0].sizey) {
        return;
    }

    // clip upper left
    if (x + clip_width - 1 >= layer[0].sizex)
        clip_width = layer[0].sizex - x;
    if (y + clip_length - 1 >= layer[0].sizey)
        clip_length = layer[0].sizey - y;

    // clip lower right
    if (x < 0) {
        clip_width += x;
        x = 0;
    }
    if (y < 0) {
        clip_length += y;
        y = 0;
    }

    source = zone + y * layer[0].sizex + x;
    y = y_sub;
    do {
        y_sub = x_sub; // don't try this at home
        x = clip_width;
        int xcount = 0;
        do {
            if (*source)
                gfx.BlitStipple(y_sub, y, *source);

            source += 1;
            x -= 1;
            y_sub += 16; // x screen position
        } while (x);
        source += (layer[0].sizex - clip_width);
        y += 16;
        clip_length -= 1;
    } while (clip_length);

    if (player) {
        x = player->tx * 16 - xwin;
        y = player->ty * 16 - ywin;
        gfx.BlitStipple(x, y, 31);
    }

    for (temp = 0; temp < entities; temp++) {
        x = entity[temp].tx * 16 - xwin;
        y = entity[temp].ty * 16 - ywin;
        gfx.BlitStipple(x, y, 32);
    }
}

void HookScriptThing(int& rpos) {
    int mark = rpos + 1;
    while (rpos < rstring.length() &&
           ('X' != rstring[rpos] && 'x' != rstring[rpos])) {
        rpos++;
    }
    int ev = V_atoi(rstring.mid(mark, rpos - mark).c_str());
    ExecuteEvent(ev);
}

void RenderMAP() {
    int rpos;

    // LFB_ClearScreen();

    curlayer = 0;
    rpos = 0;

    while (rpos < rstring.length()) {
        switch (rstring[rpos]) {
        case '1':
            BlitLayer(0);
            curlayer++;
            break;
        case '2':
            BlitLayer(1);
            curlayer++;
            break;
        case '3':
            BlitLayer(2);
            curlayer++;
            break;
        case '4':
            BlitLayer(3);
            curlayer++;
            break;
        case '5':
            BlitLayer(4);
            curlayer++;
            break;
        case '6':
            BlitLayer(5);
            curlayer++;
            break;
        case 'e':
        case 'E':
            RenderEntities();
            break;
        case 's':
        case 'S':
            HookScriptThing(rpos);
            break;
        case 'r':
        case 'R':
            if (!inside) {
                inside = 1;
                HookRetrace();
                curlayer++;
                inside = 0;
            }
            break;
        }
        rpos += 1;
    }
    if (showobs)
        DrawObstructions();
    if (showzone)
        DrawZones();
}

void CameraFocusOn(entity_r* focus) {
    if (!focus)
        return;

    // kludge for maps smaller than screen dimensions
    int w = gfx.scrx / 16;
    if (layer[0].sizex < w)
        w = layer[0].sizex;
    int h = gfx.scry / 16;
    if (layer[0].sizey < h)
        h = layer[0].sizey;
    w *= 16;
    h *= 16;

    // center in on him
    if (focus->x + 8 > (w / 2))
        xwin = (focus->x + 8 - (w / 2));
    else
        xwin = 0;
    if (focus->y + 8 > (h / 2))
        ywin = (focus->y + 8 - (h / 2));
    else
        ywin = 0;
    if (xwin > ((layer[0].sizex << 4) - w))
        xwin = ((layer[0].sizex << 4) - w);
    if (ywin > ((layer[0].sizey << 4) - h))
        ywin = ((layer[0].sizey << 4) - h);
}

void DoCameraTracking() {
    entity_r* focus;

    // there's 3 basic camera tracking modes:
    //		#1	focus on player
    //		#2	focus on specific entity; could be anyone
    //		#?	anything else disables camera tracking

    if ((1 == cameratracking && player) || (2 == cameratracking)) {
        // determine the star of the show
        if (1 == cameratracking)
            focus = player;
        else
            focus = entity + tracker;

        CameraFocusOn(focus);
    }
}

void Render() {
    //        static inside = 0;

    // for twerps who call Render() in a HookRetrace or HookTimer
    /* Tweaked.  Now the 'R' part of the renderstring is simply
       ignored if this is in a hooked script :) */
    if (!enable_recursive_render && inside)
        return;
    //        inside = 1;

    DoCameraTracking();

    SiftEntities();
    RenderMAP();

    //        inside = 0;
}

int rnd(int lo, int hi) {
    hi = hi - lo + 1;

    if (hi > 0)
        hi = rand() % hi;
    else
        hi = 0;

    return lo + hi;
}

void AnimateTile(int i, int l) {
    if (i >= 100)
        return;
    if (l < 0 || l >= numtiles)
        return;

    switch (vspanim[i].mode) {
    case 0:
        if (tileidx[l] < vspanim[i].finish)
            tileidx[l] += 1;
        else
            tileidx[l] = vspanim[i].start;
        break;

    case 1:
        if (tileidx[l] > vspanim[i].start)
            tileidx[l] -= 1;
        else
            tileidx[l] = vspanim[i].finish;
        break;

    case 2:
        tileidx[l] = (unsigned short)rnd(vspanim[i].start, vspanim[i].finish);
        break;

    case 3:
        if (flipped[l]) {
            if (tileidx[l] != vspanim[i].start)
                tileidx[l] -= 1;
            else {
                tileidx[l] += 1;
                flipped[l] = 0;
            }
        } else {
            if (tileidx[l] != vspanim[i].finish)
                tileidx[l] += 1;
            else {
                tileidx[l] -= 1;
                flipped[l] = 1;
            }
        }
    }
}

void Animate(int i) {
    int l;

    vadelay[i] = 0;
    for (l = vspanim[i].start; l <= vspanim[i].finish; l += 1)
        AnimateTile(i, l);
}

void CheckTileAnimation() {
    int i;

    if (!animate)
        return;
    if (!vsp)
        return;

    for (i = 0; i < 100; i += 1) {
        if ((vspanim[i].delay) && (vspanim[i].delay < vadelay[i]))
            Animate(i);
        vadelay[i] += 1;
    }
}
