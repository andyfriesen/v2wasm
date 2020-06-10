#pragma once

#include "png.h"

typedef unsigned char byte;

#pragma warning(disable : 4103)
#pragma pack(push, 1)
struct RGBA {
    byte red;
    byte green;
    byte blue;
    byte alpha;
};
#pragma pack(pop)

#pragma warning(disable : 4103)
#pragma pack(push, 1)
struct RGB {
    byte red;
    byte green;
    byte blue;
};
#pragma pack(pop)

struct png_image {
    int width;
    int height;
    RGBA* pixels;
};

png_image* Import_PNG(const char* filename);
static void __cdecl PNG_read_function(
    png_structp png_ptr, png_bytep data, png_size_t length);