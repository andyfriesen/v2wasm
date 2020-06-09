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
// <aen, may 5>
// + added extern decs for silhouette routines
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#if !defined(VDRIVER_H)
#define VDRIVER_H

extern int tmask;
extern int trans_mask;
extern word lucentmask;
extern int morphed;
extern unsigned short LoToHi(int index);
extern unsigned short _24ToHi(int r, int g, int b);

/*
struct video_mode_t
{
        char	id[256];
        int		width, length;
        int		bpp;
};
extern linked_list modelist;
*/

struct rect_t {
    int x, y, xend, yend;
};
extern rect_t clip;
extern int true_screen_width;
extern int true_screen_length;
extern int screen_width;
extern int screen_length;

extern int vsync;

extern unsigned char *screen;
extern unsigned char *video;
extern unsigned char *vscreen;

extern unsigned char game_palette[3 * 256];
extern unsigned char *translucency_table;
extern word *morphlut;

extern char *DriverDesc;

extern void CalcLucent(int tlevel);  //- tSB

extern int GFX_SetMode(int xres, int yres);
extern void VidInfo(void);
extern void GFX_SetPalette(unsigned char *pal);
extern void GFX_GetPalette(void);
extern void GFX_SetPaletteIntensity(int intensity);

// driver interface

extern int (*ShutdownVideo)(int i);
extern int (*ShowPage)(void);

// LFB routines for all driver functions

extern int LFB_ShowPage(void);

extern void LFB_BlitStippleTile(int x, int y, int color);
extern void LFB_ClearScreen();

// extern void ColorField(int x, int y, byte c);
// extern void ClearScreen(void);

extern void MakePalMorphTable(
    int mr, int mg, int mb, int percent, int intensity);

extern void MorphPalette(void);

extern void LFB_BlitMapLine(int x,
    int y,
    int y_offset,
    unsigned short *source,
    int masked,
    int color_mapped);

extern void LFB_Blit(int x,
    int y,
    int width,
    int length,
    unsigned char *source,
    int masked,
    int color_mapped);

extern void LFB_BlitZoom(int x,
    int y,
    int sw,
    int sh,
    int dw,
    int dh,
    unsigned char *src,
    int masked,
    int color_mapped);

extern void LFB_BlitBop(int x,
    int y,
    int width,
    int length,
    int color,
    unsigned char *source,
    int color_mapped);

extern void LFB_BlitMask(unsigned char *source,
    unsigned char *mask,
    int width,
    int length,
    unsigned char *dest);

extern void LFB_ChangeAll(unsigned char *source,
    int width,
    int length,
    int source_color,
    int dest_color);

// extern void LFB_BlitCoat_8(int x, int y, int width, int length, int color,
// unsigned char* source);

extern int LFB_GetPixel(int x, int y);
extern void LFB_BlitPixel(int x, int y, int color, int color_mapped);
extern void LFB_BlitHLine(int x, int y, int x2, int color, int color_mapped);
extern void LFB_BlitVLine(int x, int y, int y2, int color, int color_mapped);
extern void LFB_BlitRect(
    int x, int y, int x2, int y2, int color, int filled, int color_mapped);
extern void LFB_BlitCircle(
    int x, int y, int radius, int color, int filled, int color_mapped);
extern void LFB_BlitLine(
    int x, int y, int x2, int y2, int color, int color_mapped);

extern void LFB_BlitWrap(quad x,
    quad y,
    int width,
    int length,
    unsigned char *source,
    int masked,
    int color_mapped);

extern void LFB_BlitRotZoom(int posx,
    int posy,
    quad width,
    quad length,
    float angle,
    float scale,
    unsigned char *source,
    int masked,
    int color_mapped);

extern unsigned char *InitMosaicTable(void);
extern void Mosaic(quad xlevel,
    quad ylevel,
    byte *tbl,
    quad xmin,
    quad ymin,
    quad xmax,
    quad ymax);

#endif  // VDRIVER_INC
