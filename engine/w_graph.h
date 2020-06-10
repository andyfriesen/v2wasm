#pragma once

#include <cstdint>

struct RECT {
    int left;
    int top;
    int right;
    int bottom;
};

class GrDriver {
  protected:
    uint8_t* truescreen; // pointer to real screen data
    int xres, yres;
    bool fullscreen;      // fullscreen/windowed flag
    char driverdesc[255]; // string carrying the name of the driver

    // Look at all the look up tables! @_@
    char* lucentlut8;      // table for 8bit lucency
    uint16_t* lucentlut16; // hicolour lucency table
    uint16_t* morphlut;    // hicolour PaletteMorph emulation table

    // precalculated stuff for tSB's Supafast Lucency Stuff(tm)
    uint32_t lucentmask;
    int rpos, gpos, bpos;    // pixel format information
    int rsize, gsize, bsize; // ditto

    int vsync; // vertal sync flag

    RECT clip;

    void GetPixelFormat(); // sets rpos, rsize, etc...

  public:
    GrDriver();
    ~GrDriver();

    unsigned short trans_mask;
    int bpp;         // BYTES per pixel
    uint8_t* screen; // the virual screen.  Can be changed with SetRenderDest()

    uint8_t gamepal[768]; // the current palette, unmorphed
    uint8_t pal[768];     // the current palette, post-PaletteMorph

    // initialization/etc...
    bool Init(int x, int y, int bpp); // starts the whole thing up
    int SetMode(int x,
        int y,
        int bpp,
        bool fullscreen); // changes the resolution/full-screenedness
    void ShutDown();
    void VSync(bool on); // turns vsync on/off

    // colour stuff
    unsigned int Conv8(int c);                   // 8 bit colour -> screen pixel
    unsigned int Conv16(int c);                  // 5:6:5 pixel -> screen pixel
    unsigned int PackPixel(int r, int g, int b); // 8:8:8 pixel -> screen pixel
    void UnPackPixel(int c,
        int& r,
        int& g,
        int& b);                      // screen pixel -> 8:8:8 pixel
    int SetPalette(uint8_t* p);       // char[768]
    int GetPalette(uint8_t* p);       // ditto
    int InitLucentLUT(uint8_t* data); // initializes the 8bit lookup table
    void CalcLucentLUT(int lucency);  // set the lucency table up

    // accessors
    int BPP();
    int XRes(); // the width of the actual screen
    int YRes();

    int scrx, scry; // clip width/height

    bool IsFullScreen();
    char* DriverDesc();

    void Clear();

    //   blits
    // opaque blits
    void CopySprite(int x, int y, int width, int height, uint8_t* src);
    void TCopySprite(int x, int y, int width, int height, uint8_t* src);
    void ScaleSprite(int x,
        int y,
        int iwidth,
        int iheight,
        int dwidth,
        int dheight,
        uint8_t* src);
    void TScaleSprite(int x,
        int y,
        int iwidth,
        int iheight,
        int dwidth,
        int dheight,
        uint8_t* src);
    void RotScale(int posx,
        int posy,
        int width,
        int height,
        float angle,
        float scale,
        uint8_t* src);
    void WrapBlit(int x, int y, int width, int height, uint8_t* src);
    void TWrapBlit(int x, int y, int width, int height, uint8_t* src);
    void BlitStipple(int x, int y, int colour);
    // silhouette

    inline void SetPixelLucent(uint16_t* dest, int c, int lucentmode);
    // translucent blits
    void CopySpriteLucent(
        int x, int y, int width, int height, uint8_t* src, int lucentmode);
    void TCopySpriteLucent(
        int x, int y, int width, int height, uint8_t* src, int lucentmode);
    void ScaleSpriteLucent(int x,
        int y,
        int iwidth,
        int iheight,
        int dwidth,
        int dheight,
        uint8_t* src,
        int lucent);
    void TScaleSpriteLucent(int x,
        int y,
        int iwidth,
        int iheight,
        int dwidth,
        int dheight,
        uint8_t* src,
        int lucent);
    void RotScaleLucent(int posx,
        int posy,
        int width,
        int height,
        float angle,
        float scale,
        uint8_t* src,
        int lucent);
    void WrapBlitLucent(
        int x, int y, int width, int height, uint8_t* src, int lucent);
    void TWrapBlitLucent(
        int x, int y, int width, int height, uint8_t* src, int lucent);

    // Primatives
    void SetPixel(int x, int y, int colour, int lucent);
    int GetPixel(int x, int y);
    void HLine(int x, int y, int x2, int colour, int lucent);
    void VLine(int x, int y, int y2, int colour, int lucent);
    void Line(int x1, int y1, int x2, int y2, int colour, int lucent);
    void Rect(int x1, int y1, int x2, int y2, int colour, int lucent);
    void RectFill(int x1, int y1, int x2, int y2, int colour, int lucent);
    void Circle(int x, int y, int radius, int colour, int lucent);
    void CircleFill(int x, int y, int radius, int colour, int lucent);

    // polygon crap
    void FlatPoly(int x1, int y1, int x2, int y2, int x3, int y3, int colour);

  protected:
    void tmaphline(int x1,
        int x2,
        int y,
        int tx1,
        int tx2,
        int ty1,
        int ty2,
        int texw,
        int texh,
        uint8_t* image);

  public:
    void TMapPoly(int x1,
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
        uint8_t* img);

    void Mask(
        uint8_t* src, uint8_t* mask, int width, int height, uint8_t* dest);
    void Silhouette(
        int width, int height, uint8_t* src, uint8_t* dest, int colour);
    void ChangeAll(
        int width, int height, uint8_t* src, int srccolour, int destcolour);

    void ShowPage();

    // weird rendering magic stuff ;)
    void SetClipRect(RECT clip);
    void RestoreRenderSettings();
    void SetRenderDest(int x, int y, uint8_t* dest);

  protected:
    int morph_step(int S, int D, int mix, int light);

  public:
    void PaletteMorph(int r, int g, int b, int percent, int intensity);
};
