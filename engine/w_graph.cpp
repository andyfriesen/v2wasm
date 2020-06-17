// tSB's DirectDraw module for WinV2k

// Portability: Win32 only.  Compiles under mingw32 and MSVC. (untested on all
// others)
// This is designed to work in a manner as close to the old DOS driver as
// conceivably possible.
// Thus, tiles, CHRs and such are not stored on surfaces.  Consider changing
// this after
// the port is complete.

// This, in fact, is technicly triple buffering, since the main buffer is a
// simple array of chars,
// to mimic the DOS framebuffer.  It is then copied to the back surface, and
// flipped. (or blitted in windowed mode)

// There is a little bit of window-moving in here, mostly to make windows fit,
// and such when in windowed mode.
// In fact, the rest of the engine doesn't really care about the window, so this
// is about the only place where
// it gets toyed with during runtime.

// Note: All of the clipping coords are inclusive.  (0,0)-(319,199) means that
// 319 is the last pixel to write on the X axis, etc...

#include <math.h> // for RotScale

#include "verge.h"
#include "wasm.h"

GrDriver::GrDriver() {
    screen = nullptr;

    morphlut = nullptr;
    lucentlut16 = nullptr;
    lucentlut8 = nullptr;
}

GrDriver::~GrDriver() { ShutDown(); }

bool GrDriver::Init(int x, int y, bool bpp16) {
    xres = scrx = x;
    yres = scry = y;
    bpp = bpp16 ? 2 : 1;

    clip.left = 0;
    clip.top = 0;
    clip.right = xres;
    clip.bottom = yres;

    truescreen.resize(x * y * bpp);
    screen32.resize(x * y);
    screen = truescreen.data();

    wasm_initvga(x, y);
    return true;
}

int GrDriver::SetMode(int x, int y) {
    wasm_vgaresize(x, y);

    truescreen.resize(x * y * bpp);
    screen32.resize(x * y);

    xres = scrx = x;
    yres = scry = y;

    clip.left = 0;
    clip.top = 0;
    clip.right = xres;
    clip.bottom = yres;

    return 1;
}

void GrDriver::ShutDown() {}

void GrDriver::ShowPage() {
    quad srcinc, destinc; // incrememt values for the copy loop
    int yl;

    RenderGUI(); // gah! --tSB

    cpubyte = PFLIP;
    uint32_t* d = screen32.data();

    if (bpp == 1) {
        uint8_t* s = screen;

        // Convert 8bpp to 32bpp
        for (int y = 0; y < yres; ++y) {
            for (int x = 0; x < xres; ++x) {
                auto e = *s * 3;
                *d = 0xFF000000 | pal[e + 0] << 2 | pal[e + 1] << 10 | pal[e + 2] << 18;
                ++s;
                ++d;
            }
        }
    } else {
        uint16_t* s = reinterpret_cast<uint16_t*>(screen);

        for (int y = 0; y < yres; ++y) {
            for (int x = 0; x < xres; ++x) {
                uint32_t src = *s;
                *d = (src & 0b0000000000011111) << 3 |
                     (src & 0b0000011111100000) << 6 |
                     (src & 0b1111100000000000) << 8;

                ++d;
                ++s;
            }
        }
    }

    wasm_vgadump(screen32.data(), screen32.size() * sizeof(uint32_t));
    wasm_nextFrame();
}

// accessors
int GrDriver::XRes() { return xres; }

int GrDriver::YRes() { return yres; }

bool GrDriver::IsFullScreen() { return true; }

char* GrDriver::DriverDesc() { return driverdesc; }

void GrDriver::Clear() { memset(screen, 0, xres * yres * bpp); }

void GrDriver::VSync(bool on) {}

// ===================================== OPAQUE BLITS
// =====================================

void GrDriver::CopySprite(int x, int y, int width, int height, uint8_t* src) {
    int xl, yl, xs, ys;

    if (screen == NULL)
        return;

    cpubyte = RENDER;
    xl = width;
    yl = height;
    xs = ys = 0;

    if (x >= clip.right || y >= clip.bottom || x + xl <= clip.left ||
        y + yl <= clip.top)
        return;

    if (x + xl - 1 >= clip.right)
        xl = clip.right - x + 1;
    if (y + yl - 1 >= clip.bottom)
        yl = clip.bottom - y + 1;
    if (x < clip.left) {
        xs = clip.left - x;
        xl -= xs;
        x = clip.left;
    }
    if (y < clip.top) {
        ys = clip.top - y;
        yl -= ys;
        y = clip.top;
    }

    uint8_t* s8 = src + (ys * width + xs) * bpp;
    uint8_t* d8 = screen + (y * scrx + x) * bpp;

    xl *= bpp;
    width *= bpp;
    scrx *= bpp;

    for (; yl; yl--) {
        memcpy(d8, s8, xl);
        s8 += width;
        d8 += scrx;
    }
}

void GrDriver::TCopySprite(int x, int y, int width, int height, uint8_t* src) {
    int xl, yl, xs, ys;
    int srci, desti; // source inc, dest inc

    if (screen == NULL)
        return;

    cpubyte = RENDER;
    xl = width;
    yl = height;
    xs = ys = 0;
    if (x >= clip.right || y >= clip.bottom || x + xl <= clip.left ||
        y + yl <= clip.top)
        return;

    if (x + xl - 1 > clip.right)
        xl = clip.right - x + 1;
    if (y + yl - 1 > clip.bottom)
        yl = clip.bottom - y + 1;
    if (x < clip.left) {
        xs = clip.left - x;
        xl -= xs;
        x = clip.left;
    }
    if (y < clip.top) {
        ys = clip.top - y;
        yl -= ys;
        y = clip.top;
    }

    desti = scrx - xl;
    srci = width - xl;

    if (bpp > 1) // blit for hicolour
    {
        word *s16, *d16;

        s16 = (word*)(src) + ys * width + xs;
        d16 = (word*)(screen) + y * scrx + x;
        for (; yl > 0; yl--) {
            int a = xl;
            while (a--) {
                if (*s16 != trans_mask)
                    *d16 = *s16;
                d16++;
                s16++;
            }
            d16 += desti;
            s16 += srci;
        }
    } else // 8bit blit
    {
        uint8_t *s8, *d8;
        s8 = src + ys * width + xs;
        d8 = screen + y * scrx + x;

        for (; yl; yl--) {
            x = xl;
            while (x--) {
                if (*s8)
                    *d8 = *s8;
                d8++;
                s8++;
            }
            d8 += desti;
            s8 += srci;
        }
    }
    cpubyte = ETC;
}

void GrDriver::ScaleSprite(int x,
    int y,
    int iwidth,
    int iheight,
    int dwidth,
    int dheight,
    uint8_t* src) {
    int xerr, yerr;
    int xerr_start, yerr_start;
    int xadj, yadj;
    int xl, yl, xs, ys;
    int i, j, c;

    cpubyte = RENDER;

    if (dwidth < 1 || dheight < 1)
        return;

    xl = dwidth;
    yl = dheight;
    xs = ys = 0;
    if (x > clip.right || y > clip.bottom || x + xl < clip.left ||
        y + yl < clip.top)
        return;

    if (x + xl - 1 > clip.right)
        xl = clip.right - x + 1;
    if (y + yl - 1 > clip.bottom)
        yl = clip.bottom - y + 1;

    if (x < clip.left) {
        xs = clip.left - x;
        xl -= xs;
        x = clip.left;
    }
    if (y < clip.top) {
        ys = clip.top - y;
        yl -= ys;
        y = clip.top;
    }

    xadj = (iwidth << 16) / dwidth;
    yadj = (iheight << 16) / dheight;
    xerr_start = xadj * xs;
    yerr_start = yadj * ys;

    yerr = yerr_start & 0xffff;

    if (bpp == 1) {
        uint8_t *s8, *d8;

        s8 = src + ((yerr_start >> 16) * iwidth);
        d8 = screen + y * scrx + x;

        for (i = 0; i < yl; i += 1) {
            xerr = xerr_start;
            for (j = 0; j < xl; j += 1) {
                c = s8[(xerr >> 16)];
                d8[j] = c;
                xerr += xadj;
            }
            d8 += scrx;
            yerr += yadj;
            s8 += (yerr >> 16) * iwidth;
            yerr &= 0xffff;
        }
    } else {
        word *s16, *d16;

        s16 = (word*)(src) + (yerr_start >> 16) * iwidth;
        d16 = (word*)(screen) + y * scrx + x;

        for (i = 0; i < yl; i += 1) {
            xerr = xerr_start;
            for (j = 0; j < xl; j += 1) {
                c = s16[(xerr >> 16)];
                d16[j] = c;
                xerr += xadj;
            }
            d16 += scrx;
            yerr += yadj;
            s16 += (yerr >> 16) * iwidth;
            yerr &= 0xffff;
        }
    }
}

void GrDriver::TScaleSprite(int x,
    int y,
    int iwidth,
    int iheight,
    int dwidth,
    int dheight,
    uint8_t* src) {
    int xerr, yerr;
    int xerr_start, yerr_start;
    int xadj, yadj;
    int xl, yl, xs, ys;
    int i, j, c;

    cpubyte = RENDER;

    if (dwidth < 1 || dheight < 1)
        return;

    xl = dwidth;
    yl = dheight;
    xs = ys = 0;
    if (x > clip.right || y > clip.bottom || x + xl < clip.left ||
        y + yl < clip.top)
        return;

    if (x + xl - 1 > clip.right)
        xl = clip.right - x + 1;
    if (y + yl - 1 > clip.bottom)
        yl = clip.bottom - y + 1;

    if (x < clip.left) {
        xs = clip.left - x;
        xl -= xs;
        x = clip.left;
    }
    if (y < clip.top) {
        ys = clip.top - y;
        yl -= ys;
        y = clip.top;
    }

    xadj = (iwidth << 16) / dwidth;
    yadj = (iheight << 16) / dheight;
    xerr_start = xadj * xs;
    yerr_start = yadj * ys;

    yerr = yerr_start & 0xffff;

    if (bpp == 1) {
        uint8_t *s8, *d8;

        s8 = src + ((yerr_start >> 16) * iwidth);
        d8 = screen + y * scrx + x;

        for (i = 0; i < yl; i += 1) {
            xerr = xerr_start;
            for (j = 0; j < xl; j += 1) {
                c = s8[(xerr >> 16)];
                if (c)
                    d8[j] = c;
                xerr += xadj;
            }
            d8 += scrx;
            yerr += yadj;
            s8 += (yerr >> 16) * iwidth;
            yerr &= 0xffff;
        }
    } else {
        word *s16, *d16;

        s16 = (word*)(src) + (yerr_start >> 16) * iwidth;
        d16 = (word*)(screen) + y * scrx + x;

        for (i = 0; i < yl; i += 1) {
            xerr = xerr_start;
            for (j = 0; j < xl; j += 1) {
                c = s16[(xerr >> 16)];
                if (c != trans_mask)
                    d16[j] = c;
                xerr += xadj;
            }
            d16 += scrx;
            yerr += yadj;
            s16 += (yerr >> 16) * iwidth;
            yerr &= 0xffff;
        }
    }
}

void GrDriver::RotScale(int posx,
    int posy,
    int width,
    int height,
    float angle,
    float scale,
    uint8_t* src) {
    // new! shamelessly ripped off from alias.zip
    // except the atan2 stuff which i had to make up myself AEN so there :p

    int xs, ys, xl, yl;
    int sinas, cosas, xc, yc, srcx, srcy, x, y, tempx, tempy, T_WIDTH_CENTER,
        T_HEIGHT_CENTER, W_WIDTH_CENTER, W_HEIGHT_CENTER, W_HEIGHT, W_WIDTH;
    word pt;
    float ft;

    ft = (float)atan2((float)width, (float)height);

    T_WIDTH_CENTER = width >> 1;
    T_HEIGHT_CENTER = height >> 1;
    W_WIDTH =
        (int)((float)width / scale * sin(ft) + (float)height / scale * cos(ft));
    W_HEIGHT = W_WIDTH;
    W_HEIGHT_CENTER = W_HEIGHT >> 1;
    W_WIDTH_CENTER = W_HEIGHT_CENTER; // W_WIDTH/2;

    sinas = (int)(sin(-angle) * 65536 * scale);
    cosas = (int)(cos(-angle) * 65536 * scale);

    xc = T_WIDTH_CENTER * 65536 - (W_HEIGHT_CENTER * (cosas + sinas));
    yc = T_HEIGHT_CENTER * 65536 - (W_WIDTH_CENTER * (cosas - sinas));
    posx -= W_WIDTH_CENTER;
    posy -= W_HEIGHT_CENTER;

    // clipping
    if (W_WIDTH < 2 || W_HEIGHT < 2)
        return;
    xl = W_WIDTH;
    yl = W_HEIGHT;
    xs = ys = 0;
    if (posx > clip.right || posy > clip.bottom || posx + xl < clip.left ||
        posy + yl < clip.top)
        return;
    if (posx + xl - 1 > clip.right)
        xl = clip.right - posx + 1;
    if (posy + yl - 1 > clip.bottom)
        yl = clip.bottom - posy + 1;
    if (posx < clip.left) {
        xs = clip.left - posx;
        xl -= xs;
        posx = clip.left;

        xc += cosas * xs; // woo!
        yc -= sinas * xs;
    }
    if (posy < clip.top) {
        ys = clip.top - posy;
        yl -= ys;
        posy = clip.top;

        xc += sinas * ys; // woo!
        yc += cosas * ys;
    }

    if (bpp == 1) {
        uint8_t *s8, *d8;
        d8 = screen + posx + posy * scrx;
        s8 = src;
        for (y = 0; y < yl; y++) {
            srcx = xc;
            srcy = yc;

            for (x = 0; x < xl; x++) {
                tempx = (srcx >> 16);
                tempy = (srcy >> 16);

                if (tempx >= 0 && tempx < width && tempy >= 0 &&
                    tempy < height) {
                    pt = s8[tempx + tempy * width];
                    if (pt)
                        d8[x] = (uint8_t)
                            pt; // dest[x]=translucency_table[(pt<<8)|dest[x]];
                }

                srcx += cosas;
                srcy -= sinas;
            }

            d8 += scrx;

            xc += sinas;
            yc += cosas;
        }
    } else {
        word *s16, *d16;
        d16 = (word*)screen + posx + posy * scrx;
        s16 = (word*)src;
        for (y = 0; y < yl; y++) {
            srcx = xc;
            srcy = yc;

            for (x = 0; x < xl; x++) {
                tempx = (srcx >> 16);
                tempy = (srcy >> 16);

                if (tempx >= 0 && tempx < width && tempy >= 0 &&
                    tempy < height) {
                    pt = s16[tempx + tempy * width];
                    if (pt != trans_mask)
                        d16[x] =
                            pt; // dest[x]=translucency_table[(pt<<8)|dest[x]];
                }

                srcx += cosas;
                srcy -= sinas;
            }

            d16 += scrx;

            xc += sinas;
            yc += cosas;
        }
    }
}

void GrDriver::WrapBlit(int x, int y, int width, int height, uint8_t* src) {
    int cur_x, sign_y;

    if (width < 1 || height < 1)
        return;

    x %= width, y %= height;
    sign_y = 0 - y;

    for (; sign_y < scry; sign_y += height) {
        for (cur_x = 0 - x; cur_x < scrx; cur_x += width)
            CopySprite(cur_x, sign_y, width, height, src);
    }
}

void GrDriver::TWrapBlit(int x, int y, int width, int height, uint8_t* src) {
    int cur_x, sign_y;

    if (width < 1 || height < 1)
        return;

    x %= width, y %= height;
    sign_y = 0 - y;

    for (; sign_y < scry; sign_y += height) {
        for (cur_x = 0 - x; cur_x < scrx; cur_x += width)
            TCopySprite(cur_x, sign_y, width, height, src);
    }
}

// ===================================== TRANSLUCENT BLITS
// =====================================

inline void GrDriver::SetPixelLucent(word* dest, int c, int lucentmode)
// only for hicolour.  It should be noted that this simply accepts a pointer to
// dest, insted of x and y coordinates.  This is simply because the blits
// themselves can calculate dest more efficiently themselves.  (speed is good)
// TODO: ASM!  Everywhere, anywhere!
{
    int rs, gs, bs; // rgb of source pixel
    int rd, gd, bd; // rgb of dest pixel

    switch (lucentmode) {
    case 0:
        *dest = c;
        return; // wtf? oh well, return it anyway
    case 1:
        *dest = ((*dest & lucentmask) + (c & lucentmask)) >> 1;
        return; // ooh easy stuff.
    case 2:     // variable lucency
        UnPackPixel(*dest, rs, gs, bs);
        UnPackPixel(c, rd, gd, bd);
        rd = lucentlut16[rd << 8 | rs];
        gd = lucentlut16[gd << 8 | gs];
        bd = lucentlut16[bd << 8 | bs];
        *dest = PackPixel(rd, gd, bd);
        return;
    case 3: // addition
        UnPackPixel(*dest, rs, gs, bs);
        UnPackPixel(c, rd, gd, bd);
        rs += rd;
        gs += gd;
        bs += bd;
        if (rs > 255)
            rs = 255;
        if (gs > 255)
            gs = 255;
        if (bs > 255)
            bs = 255;
        *dest = PackPixel(rs, gs, bs);
        return;
    case 4: // subtraction
        UnPackPixel(*dest, rs, gs, bs);
        UnPackPixel(c, rd, gd, bd);
        rs -= rd;
        gs -= gd;
        bs -= bd;
        if (rs < 0)
            rs = 0;
        if (gs < 0)
            gs = 0;
        if (bs < 0)
            bs = 0;
        *dest = PackPixel(rs, gs, bs);
        return;
    /* this is quasi-slick, IMO.
       Instead of dividing the colour values, it simply blends them with black,
       using the variable lucency table.  In effect, doing the same thing, but
       fasta! --tSB*/
    case 5:
        UnPackPixel(*dest, rs, gs, bs);
        UnPackPixel(c, rd, gd, bd);
        rd = lucentlut16[rd << 8];
        gd = lucentlut16[gd << 8];
        bd = lucentlut16[bd << 8];
        rs += rd;
        gs += gd;
        bs += bd;
        if (rs > 255)
            rs = 255;
        if (gs > 255)
            gs = 255;
        if (bs > 255)
            bs = 255;
        *dest = PackPixel(rs, gs, bs);
        return;

    case 6:
        UnPackPixel(*dest, rs, gs, bs);
        UnPackPixel(c, rd, gd, bd);
        rd = lucentlut16[rd << 8];
        gd = lucentlut16[gd << 8];
        bd = lucentlut16[bd << 8];
        rs -= rd;
        gs -= gd;
        bs -= bd;
        if (rs < 0)
            rs = 0;
        if (gs < 0)
            gs = 0;
        if (bs < 0)
            bs = 0;
        *dest = PackPixel(rs, gs, bs);
        return;
    }
}

void GrDriver::CopySpriteLucent(
    int x, int y, int width, int height, uint8_t* src, int lucentmode) {
    int xl, yl, xs, ys;
    int a;

    if (screen == NULL)
        return;

    cpubyte = RENDER;
    xl = width;
    yl = height;
    xs = ys = 0;
    if (x > clip.right || y > clip.bottom || x + xl < clip.left ||
        y + yl < clip.top)
        return;

    if (x + xl - 1 > clip.right)
        xl = clip.right - x + 1;
    if (y + yl - 1 > clip.bottom)
        yl = clip.bottom - y + 1;
    if (x < clip.left) {
        xs = clip.left - x;
        xl -= xs;
        x = clip.left;
    }
    if (y < clip.top) {
        ys = clip.top - y;
        yl -= ys;
        y = clip.top;
    }

    if (bpp > 1) // blit for hicolour
    {
        word *s16, *d16;

        s16 = (word*)(src) + ys * width + xs;
        d16 = (word*)(screen) + y * scrx + x;
        for (; yl; yl--) // TODO: asm
        {
            for (a = 0; a < xl; a++)
                SetPixelLucent(&d16[a], s16[a], lucentmode);
            s16 += width;
            d16 += scrx;
        }
    } else // 8bit blit
    {
        uint8_t *s8, *d8;
        s8 = src + ys * width + xs;
        d8 = screen + y * scrx + x;

        for (; yl; yl--) // TODO: asm
        {
            for (a = 0; a < xl; a++)
                d8[a] = lucentlut8[d8[a] | (s8[a] << 8)]; // wee
            s8 += width;
            d8 += scrx;
        }
    }
    cpubyte = ETC;
}

void GrDriver::TCopySpriteLucent(
    int x, int y, int width, int height, uint8_t* src, int lucentmode) {
    int xl, yl, xs, ys;
    int a;

    if (screen == NULL)
        return;

    cpubyte = RENDER;
    xl = width;
    yl = height;
    xs = ys = 0;
    if (x > clip.right || y > clip.bottom || x + xl < clip.left ||
        y + yl < clip.top)
        return;

    if (x + xl - 1 > clip.right)
        xl = clip.right - x + 1;
    if (y + yl - 1 > clip.bottom)
        yl = clip.bottom - y + 1;
    if (x < clip.left) {
        xs = clip.left - x;
        xl -= xs;
        x = clip.left;
    }
    if (y < clip.top) {
        ys = clip.top - y;
        yl -= ys;
        y = clip.top;
    }

    if (bpp > 1) // blit for hicolour
    {
        word *s16, *d16;

        s16 = (word*)(src) + ys * width + xs;
        d16 = (word*)(screen) + y * scrx + x;
        //    xl=xl*2;
        for (; yl; yl--) // TODO: asm
        {
            for (a = 0; a < xl; a++)
                if (s16[a] != trans_mask)
                    SetPixelLucent(&d16[a], s16[a], lucentmode);
            s16 += width;
            d16 += scrx;
        }
    } else // 8bit blit
    {
        uint8_t *s8, *d8;
        s8 = src + ys * width + xs;
        d8 = screen + y * scrx + x;

        for (; yl; yl--) // TODO: asm
        {
            for (a = 0; a < xl; a++)
                if (s8[a])
                    d8[a] = lucentlut8[d8[a] | (s8[a] << 8)]; // wee
            s8 += width;
            d8 += scrx;
        }
    }
    cpubyte = ETC;
}

void GrDriver::ScaleSpriteLucent(int x,
    int y,
    int iwidth,
    int iheight,
    int dwidth,
    int dheight,
    uint8_t* src,
    int lucent) {
    int i, j;
    int xerr, yerr;
    int xerr_start, yerr_start;
    int xadj, yadj;
    int xl, yl, xs, ys;
    int c;

    cpubyte = RENDER;

    if (dwidth < 1 || dheight < 1)
        return;

    xl = dwidth;
    yl = dheight;
    xs = ys = 0;
    if (x > clip.right || y > clip.bottom || x + xl < clip.left ||
        y + yl < clip.top)
        return;

    if (x + xl - 1 > clip.right)
        xl = clip.right - x + 1;
    if (y + yl - 1 > clip.bottom)
        yl = clip.bottom - y + 1;

    if (x < clip.left) {
        xs = clip.left - x;
        xl -= xs;
        x = clip.left;
    }
    if (y < clip.top) {
        ys = clip.top - y;
        yl -= ys;
        y = clip.top;
    }

    xadj = (iwidth << 16) / dwidth;
    yadj = (iheight << 16) / dheight;
    xerr_start = xadj * xs;
    yerr_start = yadj * ys;

    yerr = yerr_start & 0xffff;

    if (bpp == 1) {
        uint8_t *s8, *d8;

        s8 = src + (ys * iwidth);
        d8 = screen + y * scrx + x;

        for (i = 0; i < yl; i += 1) {
            xerr = xerr_start;
            for (j = 0; j < xl; j += 1) {
                c = s8[(xerr >> 16)];
                d8[j] = lucentlut8[d8[j] | (c << 8)];
                xerr += xadj;
            }
            d8 += scrx;
            yerr += yadj;
            s8 += (yerr >> 16) * iwidth;
            yerr &= 0xffff;
        }
    } else {
        word *s16, *d16;

        s16 = (word*)(src) + (yerr_start >> 16) * iwidth;
        d16 = (word*)(screen) + y * scrx + x;

        for (i = 0; i < yl; i += 1) {
            xerr = xerr_start;
            for (j = 0; j < xl; j += 1) {
                c = s16[(xerr >> 16)];
                //       if (c!=trans_mask)
                SetPixelLucent(&d16[j], c, lucent); // d16[j] = c;
                xerr += xadj;
            }
            d16 += scrx;
            yerr += yadj;
            s16 += (yerr >> 16) * iwidth;
            yerr &= 0xffff;
        }
    }
}

void GrDriver::TScaleSpriteLucent(int x,
    int y,
    int iwidth,
    int iheight,
    int dwidth,
    int dheight,
    uint8_t* src,
    int lucent) {
    int i, j;
    int xerr, yerr;
    int xerr_start, yerr_start;
    int xadj, yadj;
    int xl, yl, xs, ys;
    int c;

    cpubyte = RENDER;

    if (dwidth < 1 || dheight < 1)
        return;

    xl = dwidth;
    yl = dheight;
    xs = ys = 0;
    if (x > clip.right || y > clip.bottom || x + xl < clip.left ||
        y + yl < clip.top)
        return;

    if (x + xl - 1 > clip.right)
        xl = clip.right - x + 1;
    if (y + yl - 1 > clip.bottom)
        yl = clip.bottom - y + 1;

    if (x < clip.left) {
        xs = clip.left - x;
        xl -= xs;
        x = clip.left;
    }
    if (y < clip.top) {
        ys = clip.top - y;
        yl -= ys;
        y = clip.top;
    }

    xadj = (iwidth << 16) / dwidth;
    yadj = (iheight << 16) / dheight;
    xerr_start = xadj * xs;
    yerr_start = yadj * ys;

    yerr = yerr_start & 0xffff;

    if (bpp == 1) {
        uint8_t *s8, *d8;

        s8 = src + (ys * iwidth);
        d8 = screen + y * scrx + x;

        for (i = 0; i < yl; i += 1) {
            xerr = xerr_start;
            for (j = 0; j < xl; j += 1) {
                c = s8[(xerr >> 16)];
                if (c)
                    d8[j] = c; // translucency_table[d[j] | (c << 8)];
                xerr += xadj;
            }
            d8 += scrx;
            yerr += yadj;
            s8 += (yerr >> 16) * iwidth;
            yerr &= 0xffff;
        }
    } else {
        word *s16, *d16;

        s16 = (word*)(src) + (yerr_start >> 16) * iwidth;
        d16 = (word*)(screen) + y * scrx + x;

        for (i = 0; i < yl; i += 1) {
            xerr = xerr_start;
            for (j = 0; j < xl; j += 1) {
                c = s16[(xerr >> 16)];
                if (c != trans_mask)
                    SetPixelLucent(&d16[j], c, lucent); // d16[j] = c;
                xerr += xadj;
            }
            d16 += scrx;
            yerr += yadj;
            s16 += (yerr >> 16) * iwidth;
            yerr &= 0xffff;
        }
    }
}

void GrDriver::RotScaleLucent(int posx,
    int posy,
    int width,
    int height,
    float angle,
    float scale,
    uint8_t* src,
    int lucent) {
    // new! shamelessly ripped off from alias.zip
    // except the atan2 stuff which i had to make up myself AEN so there :p

    int xs, ys, xl, yl;
    int sinas, cosas, xc, yc, srcx, srcy, x, y, tempx, tempy, T_WIDTH_CENTER,
        T_HEIGHT_CENTER, W_WIDTH_CENTER, W_HEIGHT_CENTER, W_HEIGHT, W_WIDTH;
    word pt;
    float ft;

    ft = (float)atan2((float)width, (float)height);

    T_WIDTH_CENTER = width >> 1;
    T_HEIGHT_CENTER = height >> 1;
    W_WIDTH =
        (int)((float)width / scale * sin(ft) + (float)height / scale * cos(ft));
    W_HEIGHT = W_WIDTH;
    W_HEIGHT_CENTER = W_HEIGHT >> 1;
    W_WIDTH_CENTER = W_HEIGHT_CENTER; // W_WIDTH/2;

    sinas = (int)(sin(-angle) * 65536 * scale);
    cosas = (int)(cos(-angle) * 65536 * scale);

    xc = T_WIDTH_CENTER * 65536 - (W_HEIGHT_CENTER * (cosas + sinas));
    yc = T_HEIGHT_CENTER * 65536 - (W_WIDTH_CENTER * (cosas - sinas));
    posx -= W_WIDTH_CENTER;
    posy -= W_HEIGHT_CENTER;

    // clipping
    if (W_WIDTH < 2 || W_HEIGHT < 2)
        return;
    xl = W_WIDTH;
    yl = W_HEIGHT;
    xs = ys = 0;
    if (posx > clip.right || posy > clip.bottom || posx + xl < clip.left ||
        posy + yl < clip.top)
        return;
    if (posx + xl - 1 > clip.right)
        xl = clip.right - posx + 1;
    if (posy + yl - 1 > clip.bottom)
        yl = clip.bottom - posy + 1;
    if (posx < clip.left) {
        xs = clip.left - posx;
        xl -= xs;
        posx = clip.left;

        xc += cosas * xs; // woo!
        yc -= sinas * xs;
    }
    if (posy < clip.top) {
        ys = clip.top - posy;
        yl -= ys;
        posy = clip.top;

        xc += sinas * ys; // woo!
        yc += cosas * ys;
    }

    if (bpp == 1) {
        uint8_t *s8, *d8;
        d8 = screen + posx + posy * scrx;
        s8 = src;
        for (y = 0; y < yl; y++) {
            srcx = xc;
            srcy = yc;

            for (x = 0; x < xl; x++) {
                tempx = (srcx >> 16);
                tempy = (srcy >> 16);

                if (tempx >= 0 && tempx < width && tempy >= 0 &&
                    tempy < height) {
                    pt = s8[tempx + tempy * width];
                    if (pt)
                        d8[x] = lucentlut8[(pt << 8) | d8[x]];
                }

                srcx += cosas;
                srcy -= sinas;
            }

            d8 += scrx;

            xc += sinas;
            yc += cosas;
        }
    } else {
        word *s16, *d16;
        d16 = (word*)(screen) + posy * scrx + posx;
        s16 = (word*)src;
        for (y = 0; y < yl; y++) {
            srcx = xc;
            srcy = yc;

            for (x = 0; x < xl; x++) {
                tempx = (srcx >> 16);
                tempy = (srcy >> 16);

                if (tempx >= 0 && tempx < width && tempy >= 0 &&
                    tempy < height) {
                    pt = s16[tempx + tempy * width];
                    if (pt)
                        SetPixelLucent(&d16[x], pt,
                            lucent); // d16[x]=pt;//dest[x]=translucency_table[(pt<<8)|dest[x]];
                }

                srcx += cosas;
                srcy -= sinas;
            }

            d16 += scrx;

            xc += sinas;
            yc += cosas;
        }
    }
}

void GrDriver::BlitStipple(int x, int y, int colour) {
    int xl, yl, xs, ys;
    int ax, ay;

    xl = 16;
    yl = 16;
    xs = ys = 0;
    if (x > clip.right || y > clip.bottom || x + xl < clip.left ||
        y + yl < clip.top)
        return;
    if (x + xl - 1 > clip.right)
        xl = clip.right - x + 1;
    if (y + yl - 1 > clip.bottom)
        yl = clip.bottom - y + 1;
    if (x < clip.left) {
        xl += x - clip.left;
        x = clip.left;
    }
    if (y < clip.top) {
        yl += y - clip.top;
        y = clip.top;
    }

    if (bpp == 1) {
        uint8_t* d8;
        d8 = screen + y * scrx + x;
        for (ay = 0; ay < yl; ay++) {
            for (ax = 0; ax < xl; ax++)
                if ((ax + ay) & 1)
                    d8[ax] = colour;
            d8 += scrx;
        }
    } else {
        word* d16;
        d16 = (word*)(screen) + y * scrx + x;
        for (ay = 0; ay < yl; ay++) {
            ax = xl;
            while (ax--)
                SetPixelLucent(d16++, colour, 1);
            d16 += scrx - xl;
        }
    }
}

void GrDriver::WrapBlitLucent(
    int x, int y, int width, int height, uint8_t* src, int lucent) {
    int cur_x, sign_y;

    if (width < 1 || height < 1)
        return;

    x %= width, y %= height;
    sign_y = 0 - y;

    for (; sign_y < scry; sign_y += height) {
        for (cur_x = 0 - x; cur_x < scrx; cur_x += width)
            CopySpriteLucent(cur_x, sign_y, width, height, src, lucent);
    }
}

void GrDriver::TWrapBlitLucent(
    int x, int y, int width, int height, uint8_t* src, int lucent) {
    int cur_x, sign_y;

    if (width < 1 || height < 1)
        return;

    x %= width, y %= height;
    sign_y = 0 - y;

    for (; sign_y < scry; sign_y += height) {
        for (cur_x = 0 - x; cur_x < scrx; cur_x += width)
            TCopySpriteLucent(cur_x, sign_y, width, height, src, lucent);
    }
}

// primitives

void GrDriver::SetPixel(int x, int y, int colour, int lucent) {
    int ofs;
    ofs = y * scrx + x;

    if (x < clip.left || y < clip.top || x > clip.right || y > clip.bottom)
        return;

    if (bpp == 1) {
        uint8_t* p = screen + ofs;
        if (lucent)
            *p = lucentlut8[colour || *p << 8];
        else
            *p = colour;
    } else {
        word* p = (word*)screen + ofs;
        if (lucent)
            SetPixelLucent(p, colour, lucent);
        else
            *p = colour;
    }
}

int GrDriver::GetPixel(int x, int y) {
    if (x < clip.left || y < clip.top || x > clip.right || y > clip.bottom)
        return 0;

    if (bpp == 1) {
        uint8_t* c;
        c = screen + y * scrx + x;
        return *c;
    } else {
        word* w;
        w = (word*)screen + y * scrx + x;
        return *w;
    }
}

void GrDriver::HLine(int x, int y, int x2, int colour, int lucent) {
    int xl;
    // range checking
    if (x > x2) {
        int a = x2;
        x2 = x;
        x = a;
    } // swap 'em

    if (x < clip.left)
        x = clip.left;
    if (x2 > clip.right)
        x2 = clip.right;

    // Is the line even onscreen?
    if (y < clip.top)
        return;
    if (y > clip.bottom)
        return;
    if (x > clip.right)
        return;
    if (x2 < clip.left)
        return;

    if (bpp == 1) {
        uint8_t* d8;
        d8 = screen + (y * scrx + x);
        xl = x2 - x + 1;
        if (lucent)
            while (xl--)
                *d8 = lucentlut8[*d8++ | (colour << 8)];
        else
            memset(d8, (char)colour, xl); // woo. hard
    } else {
        word* d16;
        d16 = (word*)(screen) + y * scrx + x;
        xl = x2 - x + 1;
        if (lucent)
            while (xl--)
                SetPixelLucent(d16++, colour, lucent);
        else
            while (xl--)
                *d16++ = (word)colour; // woo. also hard.
    }
}

void GrDriver::VLine(int x, int y, int y2, int colour, int lucent) {
    if (y > y2) {
        int a = y2;
        y2 = y;
        y = a;
    } // swap 'em

    if (y < clip.top)
        y = clip.top;
    if (y2 > clip.bottom)
        y2 = clip.bottom;

    // Is the line even onscreen?
    if (y2 < clip.top)
        return;
    if (y > clip.bottom)
        return;
    if (x > clip.right)
        return;
    if (x < clip.left)
        return;

    if (bpp == 1) {
        uint8_t* d8;
        d8 = screen + (y * scrx + x);
        int yl = y2 - y + 1;
        if (lucent)
            while (yl--) {
                *d8 = lucentlut8[*d8 | (colour << 8)];
                d8 += scrx;
            }
        else
            while (yl--) {
                *d8 = (uint8_t)colour;
                d8 += scrx;
            }
    } else {
        word* d16;
        d16 = (word*)(screen) + y * scrx + x;
        int yl = y2 - y + 1;
        if (lucent)
            while (yl--) {
                SetPixelLucent(d16, colour, lucent);
                d16 += scrx;
            }
        else
            while (yl--) {
                *d16 = (word)colour;
                d16 += scrx;
            }
    }
}

void swap(int& a, int& b) {
    int c;
    c = a;
    a = b;
    b = c;
}

void GrDriver::Line(int x1, int y1, int x2, int y2, int colour, int lucent) {
    short i, xc, yc, er, n, m, xi, yi, xcxi, ycyi, xcyi;
    unsigned dcy, dcx;

    cpubyte = RENDER;
    // check to see if the line is completly clipped off
    if ((x1 < clip.left && x2 < clip.left) ||
        (x1 > clip.right && x2 > clip.right) ||
        (y1 < clip.top && y2 < clip.top) ||
        (y1 > clip.bottom && y2 > clip.bottom)) {
        cpubyte = ETC;
        return;
    }

    if (x1 > x2) {
        swap(x1, x2);
        swap(y1, y2);
    }

    // clip the left side
    if (x1 < clip.left) {
        int myy = (y2 - y1);
        int mxx = (x2 - x1), b;
        if (!mxx) {
            cpubyte = ETC;
            return;
        }
        if (myy) {
            b = y1 - (y2 - y1) * x1 / mxx;
            y1 = myy * clip.left / mxx + b;
            x1 = clip.left;
        } else
            x1 = clip.left;
    }

    // clip the right side
    if (x2 > clip.right) {
        int myy = (y2 - y1);
        int mxx = (x2 - x1), b;
        if (!mxx) {
            cpubyte = ETC;
            return;
        }
        if (myy) {
            b = y1 - (y2 - y1) * x1 / mxx;
            y2 = myy * clip.right / mxx + b;
            x2 = clip.right;
        } else
            x2 = clip.right;
    }

    if (y1 > y2) {
        swap(x1, x2);
        swap(y1, y2);
    }

    // clip the bottom
    if (y2 > clip.bottom) {
        int mxx = (x2 - x1);
        int myy = (y2 - y1), b;
        if (!myy) {
            cpubyte = ETC;
            return;
        }
        if (mxx) {
            b = y1 - (y2 - y1) * x1 / mxx;
            x2 = (clip.bottom - b) * mxx / myy;
            y2 = clip.bottom;
        } else
            y2 = clip.bottom;
    }

    // clip the top
    if (y1 < clip.top) {
        int mxx = (x2 - x1);
        int myy = (y2 - y1), b;
        if (!myy) {
            cpubyte = ETC;
            return;
        }
        if (mxx) {
            b = y1 - (y2 - y1) * x1 / mxx;
            x1 = (clip.top - b) * mxx / myy;
            y1 = clip.top;
        } else
            y1 = clip.top;
    }

    // ???
    // see if it got cliped into the box, out out
    if (x1 < clip.left || x2 < clip.left || x1 > clip.right ||
        x2 > clip.right || y1 < clip.top || y2 < clip.top || y1 > clip.bottom ||
        y2 > clip.bottom) {
        cpubyte = ETC;
        return;
    }

    if (x1 > x2) {
        xc = x2;
        xi = x1;
    } else {
        xi = x2;
        xc = x1;
    }

    // assume y1<=y2 from above swap operation
    yi = y2;
    yc = y1;

    dcx = x1;
    dcy = y1;
    xc = (x2 - x1);
    yc = (y2 - y1);
    if (xc < 0)
        xi = -1;
    else
        xi = 1;
    if (yc < 0)
        yi = -1;
    else
        yi = 1;
    n = abs(xc);
    m = abs(yc);
    ycyi = abs(2 * yc * xi);
    er = 0;

    if (bpp == 1) {
        if (n > m) {
            xcxi = abs(2 * xc * xi);
            if (lucent)
                for (i = 0; i <= n; i++) {
                    screen[(dcy * scrx) + dcx] =
                        lucentlut8[colour | screen[(dcy * scrx) + dcx] << 8];
                    if (er > 0) {
                        dcy += yi;
                        er -= xcxi;
                    }
                    er += ycyi;
                    dcx += xi;
                }
            else
                for (i = 0; i <= n; i++) {
                    screen[(dcy * scrx) + dcx] = colour;
                    if (er > 0) {
                        dcy += yi;
                        er -= xcxi;
                    }
                    er += ycyi;
                    dcx += xi;
                }
        } else {
            xcyi = abs(2 * xc * yi);
            if (lucent)
                for (i = 0; i <= m; i++) {
                    screen[(dcy * scrx) + dcx] =
                        lucentlut8[colour | screen[(dcy * scrx) + dcx] << 8];
                    if (er > 0) {
                        dcx += xi;
                        er -= ycyi;
                    }
                    er += xcyi;
                    dcy += yi;
                }
            else
                for (i = 0; i <= m; i++) {
                    screen[(dcy * scrx) + dcx] = colour;
                    if (er > 0) {
                        dcx += xi;
                        er -= ycyi;
                    }
                    er += xcyi;
                    dcy += yi;
                }
        }
    } else {
        word* s16 = (word*)screen;
        if (n > m) {
            xcxi = abs(2 * xc * xi);
            if (lucent)
                for (i = 0; i <= n; i++) {
                    //         s16[(dcy*scrx)+dcx]=colour;
                    SetPixelLucent(&s16[(dcy * scrx) + dcx], colour, lucent);
                    if (er > 0) {
                        dcy += yi;
                        er -= xcxi;
                    }
                    er += ycyi;
                    dcx += xi;
                }
            else
                for (i = 0; i <= n; i++) {
                    s16[(dcy * scrx) + dcx] = colour;
                    if (er > 0) {
                        dcy += yi;
                        er -= xcxi;
                    }
                    er += ycyi;
                    dcx += xi;
                }
        } else {
            xcyi = abs(2 * xc * yi);
            for (i = 0; i <= m; i++) {
                s16[(dcy * scrx) + dcx] = colour;
                if (er > 0) {
                    dcx += xi;
                    er -= ycyi;
                }
                er += xcyi;
                dcy += yi;
            }
        }
    }
    cpubyte = ETC;
    return;
}

void GrDriver::Rect(int x1, int y1, int x2, int y2, int colour, int lucent) {
    int a;
    if (x1 > x2) {
        a = x1;
        x1 = x2;
        x2 = a;
    } // swap 'em
    if (y1 > y2) {
        a = y1;
        y1 = y2;
        y2 = a;
    }

    // range checking.  Bleh, hline and vline do it anyway. --tSB
    /* if (x1<clip.left) x1=clip.left;
     if (x2>clip.right) x2=clip.right;
     if (y1<clip.top) y1=clip.top;
     if (y2>clip.bottom) y2=clip.bottom;*/

    VLine(x1, y1, y2, colour, lucent);
    VLine(x2, y1, y2, colour, lucent);
    HLine(x1, y1, x2, colour, lucent);
    HLine(x1, y2, x2, colour, lucent);
}

void GrDriver::RectFill(
    int x1, int y1, int x2, int y2, int colour, int lucent) {
    int a;
    if (x1 > x2) {
        a = x1;
        x1 = x2;
        x2 = a;
    } // swap 'em
    if (y1 > y2) {
        a = y1;
        y1 = y2;
        y2 = a;
    }

    // range checking
    if (x1 < clip.left)
        x1 = clip.left;
    if (x2 > clip.right)
        x2 = clip.right;
    if (y1 < clip.top)
        y1 = clip.top;
    if (y2 > clip.bottom)
        y2 = clip.bottom;

    for (a = y1; a <= y2; a++)
        HLine(x1, a, x2, colour, lucent);
}

void GrDriver::Circle(int x, int y, int radius, int colour, int lucent) {
    int cx = 0;
    int cy = radius;
    int df = 1 - radius;
    int d_e = 3;
    int d_se = -2 * radius + 5;

    cpubyte = RENDER;

    do {
        SetPixel(x + cx, y + cy, colour, lucent);
        if (cx)
            SetPixel(x - cx, y + cy, colour, lucent);
        if (cy)
            SetPixel(x + cx, y - cy, colour, lucent);
        if (cx && cy)
            SetPixel(x - cx, y - cy, colour, lucent);

        if (cx != cy) {
            SetPixel(x + cy, y + cx, colour, lucent);
            if (cx)
                SetPixel(x + cy, y - cx, colour, lucent);
            if (cy)
                SetPixel(x - cy, y + cx, colour, lucent);
            if (cx && cy)
                SetPixel(x - cy, y - cx, colour, lucent);
        }

        if (df < 0) {
            df += d_e;
            d_e += 2;
            d_se += 2;
        } else {
            df += d_se;
            d_e += 2;
            d_se += 4;
            cy -= 1;
        }
        cx += 1;
    } while (cx <= cy);

    cpubyte = ETC;
}

void GrDriver::CircleFill(int x, int y, int radius, int colour, int lucent) {
    int cx = 0;
    int cy = radius;
    int df = 1 - radius;
    int d_e = 3;
    int d_se = -2 * radius + 5;

    cpubyte = RENDER;

    do {
        HLine(x - cy, y - cx, x + cy, colour, lucent);
        if (cx)
            HLine(x - cy, y + cx, x + cy, colour, lucent);

        if (df < 0) {
            df += d_e;
            d_e += 2;
            d_se += 2;
        } else {
            if (cx != cy) {
                HLine(x - cx, y - cy, x + cx, colour, lucent);
                if (cy)
                    HLine(x - cx, y + cy, x + cx, colour, lucent);
            }
            df += d_se;
            d_e += 2;
            d_se += 4;
            cy -= 1;
        }
        cx += 1;
    } while (cx <= cy);

    cpubyte = ETC;
}

void GrDriver::FlatPoly(
    int x1, int y1, int x2, int y2, int x3, int y3, int colour) {
    int xstep, xstep2;
    int xval, xval2;
    int yon;
    int swaptemp;

    if (y1 > y3) {
        swaptemp = x1;
        x1 = x3;
        x3 = swaptemp;
        swaptemp = y1;
        y1 = y3;
        y3 = swaptemp;
    }
    if (y2 > y3) {
        swaptemp = x2;
        x2 = x3;
        x3 = swaptemp;
        swaptemp = y2;
        y2 = y3;
        y3 = swaptemp;
    }
    if (y1 > y2) {
        swaptemp = x1;
        x1 = x2;
        x2 = swaptemp;
        swaptemp = y1;
        y1 = y2;
        y2 = swaptemp;
    }

    xstep2 = ((x3 - x1) << 16) / (y3 - y1);
    xval2 = x1 << 16;

    if (y1 != y2) {
        xstep = ((x2 - x1) << 16) / (y2 - y1);
        xval = x1 << 16;
        for (yon = y1; yon < y2; yon++) {
            if ((yon > clip.top) && (yon < clip.bottom)) {
                HLine(xval >> 16, yon, xval2 >> 16, colour, 0);
            }
            xval += xstep;
            xval2 += xstep2;
        }
    }
    if (y2 != y3) {
        xstep = ((x3 - x2) << 16) / (y3 - y2);
        xval = x2 << 16;
        for (yon = y2; yon < y3; yon++) {
            if ((yon > clip.top) && (yon < clip.bottom)) {
                HLine(xval >> 16, yon, xval2 >> 16, colour, 0);
            }
            xval += xstep;
            xval2 += xstep2;
        }
    }
}

inline void GrDriver::tmaphline(int x1,
    int x2,
    int y,
    int tx1,
    int tx2,
    int ty1,
    int ty2,
    int texw,
    int texh,
    uint8_t* image) {
    int i;
    int txstep, txval;
    int tystep, tyval;

    if (x1 != x2) {
        if (x2 < x1) {
            i = x1;
            x1 = x2;
            x2 = i;
            i = tx1;
            tx1 = tx2;
            tx2 = i;
            i = ty1;
            ty1 = ty2;
            ty2 = i;
        }
        if ((x1 > scrx) || (x2 < 0))
            return;
        txstep = ((tx2 - tx1) << 16) / (x2 - x1);
        tystep = ((ty2 - ty1) << 16) / (x2 - x1);
        txval = tx1 << 16;
        tyval = ty1 << 16;
        word* s16 = (word*)screen;
        word* d16 = (word*)image;

        if (bpp == 1)
            for (i = x1; i < x2; i++) {
                screen[y * scrx + i] =
                    image[(tyval >> 16) * texw + (txval >> 16)];
                txval += txstep;
                tyval += tystep;
            }
        else
            for (i = x1; i < x2; i++) {
                s16[y * scrx + i] = d16[(tyval >> 16) * texw + (txval >> 16)];
                txval += txstep;
                tyval += tystep;
            }
    }
}

void GrDriver::TMapPoly(int x1,
    int y1,
    int x2,
    int y2,
    int x3,
    int y3,
    int tx1,
    int ty1,
    int tx2,
    int ty2,
    int tx3,
    int ty3,
    int tw,
    int th,
    uint8_t* img) {
    int xstep, xstep2;
    int xval, xval2;
    int txstep, txstep2;
    int tystep, tystep2;
    int txval, txval2;
    int tyval, tyval2;
    int yon;
    int swaptemp;

    uint8_t* image;
    int texw, texh;

    image = img;
    texw = tw;
    texh = th;
    if (y1 > y3) {
        swaptemp = x1;
        x1 = x3;
        x3 = swaptemp;
        swaptemp = y1;
        y1 = y3;
        y3 = swaptemp;
        swaptemp = tx1;
        tx1 = tx3;
        tx3 = swaptemp;
        swaptemp = ty1;
        ty1 = ty3;
        ty3 = swaptemp;
    }
    if (y2 > y3) {
        swaptemp = x2;
        x2 = x3;
        x3 = swaptemp;
        swaptemp = y2;
        y2 = y3;
        y3 = swaptemp;
        swaptemp = tx2;
        tx2 = tx3;
        tx3 = swaptemp;
        swaptemp = ty2;
        ty2 = ty3;
        ty3 = swaptemp;
    }
    if (y1 > y2) {
        swaptemp = x1;
        x1 = x2;
        x2 = swaptemp;
        swaptemp = y1;
        y1 = y2;
        y2 = swaptemp;
        swaptemp = tx1;
        tx1 = tx2;
        tx2 = swaptemp;
        swaptemp = ty1;
        ty1 = ty2;
        ty2 = swaptemp;
    }
    xstep2 = ((x3 - x1) << 16) / (y3 - y1);
    xval2 = x1 << 16;
    txstep2 = ((tx3 - tx1) << 16) / (y3 - y1);
    txval2 = tx1 << 16;
    tystep2 = ((ty3 - ty1) << 16) / (y3 - y1);
    tyval2 = ty1 << 16;

    if (y1 != y2) {
        xstep = ((x2 - x1) << 16) / (y2 - y1);
        xval = x1 << 16;
        txstep = ((tx2 - tx1) << 16) / (y2 - y1);
        txval = tx1 << 16;
        tystep = ((ty2 - ty1) << 16) / (y2 - y1);
        tyval = ty1 << 16;

        for (yon = y1; yon < y2; yon++) {
            if ((yon > clip.top) && (yon < clip.bottom))
                tmaphline(xval >> 16, xval2 >> 16, yon, txval >> 16,
                    txval2 >> 16, tyval >> 16, tyval2 >> 16, texw, texh, image);

            xval += xstep;
            xval2 += xstep2;
            txval += txstep;
            txval2 += txstep2;
            tyval += tystep;
            tyval2 += tystep2;
        }
    }
    if (y2 != y3) {
        xstep = ((x3 - x2) << 16) / (y3 - y2);
        xval = x2 << 16;
        txstep = ((tx3 - tx2) << 16) / (y3 - y2);
        txval = tx2 << 16;
        tystep = ((ty3 - ty2) << 16) / (y3 - y2);
        tyval = ty2 << 16;

        for (yon = y2; yon < y3; yon++) {
            if ((yon > clip.top) && (yon < clip.bottom)) {
                tmaphline(xval >> 16, xval2 >> 16, yon, txval >> 16,
                    txval2 >> 16, tyval >> 16, tyval2 >> 16, texw, texh, image);
            }
            xval += xstep;
            xval2 += xstep2;
            txval += txstep;
            txval2 += txstep2;
            tyval += tystep;
            tyval2 += tystep2;
        }
    }
}

void GrDriver::Mask(
    uint8_t* src, uint8_t* mask, int width, int height, uint8_t* dest) {
    int i = width * height;
    if (bpp == 2) {
        word* s16 = (word*)src;
        word* m16 = (word*)mask;
        word* d16 = (word*)dest; // bleh, makes my life easier

        while (i--) {
            *d16 = (*s16 & *m16);
            d16++;
            s16++;
            m16++;
        }
        // hmm.. simple enough. ;)
    } else {
        while (i--) {
            *dest = (*src & *mask);
            dest++;
            mask++;
            src++;
        }
    }
}

void GrDriver::Silhouette(
    int width, int height, uint8_t* src, uint8_t* dest, int colour) {
    width *= height;
    if (bpp == 1) {
        while (width--) {
            if (*src)
                *dest = colour;
            src++;
            dest++;
        }
    } else {
        word* s16 = (word*)src;
        word* d16 = (word*)dest;
        while (width--) {
            if (*s16 != trans_mask)
                *d16 = colour;
            s16++;
            d16++;
        }
    }
}

void GrDriver::ChangeAll(
    int width, int height, uint8_t* src, int srccolour, int destcolour) {
    width *= height;
    if (bpp == 1) {
        while (width--) {
            if (*src == srccolour)
                *src = destcolour;
            src++;
        }
    } else {
        word* s16 = (word*)src;
        while (width--) {
            if (*s16 == srccolour)
                *s16 = destcolour;
            s16++;
        }
    }
}

// Pixel/palette crap

void GrDriver::GetPixelFormat() {
    rpos = 0;
    gpos = 8;
    bpos = 16;
    rsize = gsize = bsize = 8;

    bpp = 2;

    lucentmask = ~((1 << rpos) | (1 << gpos) | (1 << bpos));
}

// converts an 8 bit palettized pixel to the current bit depth
unsigned int GrDriver::Conv8(int c) {
    int i, r, g, b;
    static int lowestsofar, highestsofar;

    if (bpp == 1)
        return c; // we'll assume it's the same palette for now.

    i = c * 3; // pedal to the metal! ;D
    r = gamepal[i++];
    g = gamepal[i++];
    b = gamepal[i++];
    if (!c)
        return trans_mask;
    else
        return PackPixel(r << 2, g << 2, b << 2);
}

unsigned int GrDriver::Conv16(int c)
// converts a 5:6:5 hicolour pixel to the current bit depth
{
    int r, g, b;

    b = c & 31;
    g = (c >> 5) & 63;
    r = (c >> 11) & 31;
    // b<<=1; r<<=1;

    return PackPixel(r << 3, g << 2, b << 3);
}

unsigned int GrDriver::PackPixel(int r, int g, int b) {
    if (rsize < 8)
        r >>= 8 - rsize;
    if (gsize < 8)
        g >>= 8 - gsize;
    if (bsize < 8)
        b >>= 8 - bsize;

    return (r << rpos) | (g << gpos) | (b << bpos);
    // TODO: add closest-matching-colour function for 8bit modes.
}

void GrDriver::UnPackPixel(int c, int& r, int& g, int& b) {
    r = c >> rpos;
    g = c >> gpos;
    b = c >> bpos;

    if (rsize < 8)
        r = (r << (8 - rsize)) & 255;
    if (gsize < 8)
        g = (g << (8 - gsize)) & 255;
    if (bsize < 8)
        b = (b << (8 - bsize)) & 255;
}

int GrDriver::SetPalette(uint8_t* p) // p is a char[768]
{
    static int beenhere = 0;

    memcpy(pal, p, 768);

    if (!beenhere) {
        memcpy(
            gamepal, p, 768);  // one time only, copy the default VERGE palette
        beenhere = 1;
    }

    return 1;
}

int GrDriver::GetPalette(uint8_t* p) // p is a char[768]
{
    memcpy(p, pal, 768);
    return 1;
}

int GrDriver::InitLucentLUT(uint8_t* data)
// this just copies the data into lucentlut8 so the lucent stuff can work in
// 8bit mode.
{
    if (lucentlut8 != NULL)
        delete lucentlut8;
    lucentlut8 = new char[256 * 256];
    memcpy(lucentlut8, data, 256 * 256);
    return 1; // bleh, simple
}

void GrDriver::CalcLucentLUT(int lucency) {
    int biggest;

    biggest = rsize;
    biggest = biggest < gsize ? gsize : biggest;
    biggest = biggest < bsize ? bsize : biggest;

    if (lucentlut16 == NULL)
        lucentlut16 = new word[65535]; // o_O --tSB

    int i, j;

    for (i = 0; i < 256; i++)
        for (j = 0; j < 256; j++) {
            lucentlut16[i * 256 + j] =
                (i * lucency + j * (255 - lucency)) / 256;
            // i*32+j == (i*tlevel+j*(255-tlevel))/256;
        }
}

void GrDriver::SetClipRect(RECT newrect) {
    memcpy(&clip, &newrect, sizeof clip);
}

void GrDriver::SetRenderDest(int x, int y, uint8_t* dest) {
    scrx = x;
    scry = y;
    clip.top = clip.left = 0;
    clip.right = x - 1;
    clip.bottom = y - 1;
    screen = dest;
}

void GrDriver::RestoreRenderSettings() {
    scrx = xres;
    scry = yres;
    clip.top = clip.left = 0;
    clip.right = xres - 1;
    clip.bottom = yres - 1;
    screen = truescreen.data();
}

inline int GrDriver::morph_step(int S, int D, int mix, int light) {
    return (mix * (S - D) + (100 * D)) * light / 100 / 64;
}

void GrDriver::PaletteMorph(
    int mr, int mg, int mb, int percent, int intensity) {
    int rgb[3], n;
    uint8_t pmorph_palette[3 * 256];

    int i;
    int wr, wg, wb;
    int r, g, b;

    if (bpp == 2) {
        if (percent && intensity == 63) {
            if (morphlut)
                delete morphlut;
            morphlut = NULL;
            return;
        }
        if (morphlut == NULL)
            morphlut =
                new word[65536]; // YES, I like look-up tables, thank you very
                                 // much. --tSB
        for (i = 0; i < 65535; i++) {
            UnPackPixel(i, r, g, b);

            wr = morph_step(r, mr, percent, intensity);
            wg = morph_step(g, mg, percent, intensity);
            wb = morph_step(b, mb, percent, intensity);

            morphlut[i] = PackPixel(wr, wg, wb);
        }
    } else {
        rgb[0] = mr;
        rgb[1] = mg;
        rgb[2] = mb;

        for (n = 0; n < 3; n += 1) {
            if (rgb[n] < 0)
                rgb[n] = 0;
            else if (rgb[n] > 63)
                rgb[n] = 63;
        }

        // pull the colors
        for (n = 0; n < 3 * 256; n += 1) {
            pmorph_palette[n] = (unsigned char)morph_step(
                gfx.gamepal[n], rgb[n % 3], percent, intensity);
        }

        // enforce new palette
        SetPalette(pmorph_palette);
    }
}
