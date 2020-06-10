#ifndef I_PNG_H
#define I_PNG_H

#include "png.h"

typedef unsigned char byte;

#pragma warning(disable : 4103)
#pragma pack(push, 1)
typedef struct {
    byte red;
    byte green;
    byte blue;
    byte alpha;
} RGBA;
#pragma pack(pop)

#pragma warning(disable : 4103)
#pragma pack(push, 1)
typedef struct {
    byte red;
    byte green;
    byte blue;
} RGB;
#pragma pack(pop)

typedef struct {
    int width;
    int height;
    RGBA* pixels;
} png_image;

png_image* Import_PNG(const char* filename);
static void __cdecl PNG_read_function(
    png_structp png_ptr, png_bytep data, png_size_t length);

#endif // def I_PNG_H