// i_png.cpp
// PNG loading caca

#include "i_png.h"
#include "verge.h"

// ----------------- PNG ---------------------
// I think this was originally in the Sphere source, so, give Aegis credit for
// this.  I just hacked it up a bit. --tSB

/**********************
*  bool Import_PNG()  *
***********************
* Group:   Graphics
* Job:     loads a PNG image into a png_image structure
* Returns: pointer to the image on success, NULL otherwise
**********************/
png_image *Import_PNG(const char *filename) {
    Log("Import_PNG started...");

    png_image *pis = new png_image;

    // open file
    VFILE *file = vopen(filename);
    if (file == NULL) {
        delete pis;
        return NULL;
    }

    //  Log("(%) File Opened");

    // verify signature
    byte sig[8];
    vread(sig, 8, file);
    if (png_sig_cmp(sig, 0, 8)) {
        vclose(file);
        delete pis;
        return NULL;
    }

    //  Log("(%) Signature Verified");

    // read struct
    png_structp png_ptr =
        png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        vclose(file);
        delete pis;
        return NULL;
    }

    //  Log("(%) Read Struct Created");

    // info struct
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        vclose(file);
        delete pis;
        return NULL;
    }

    //  Log("(%) Info Struct Created");

    png_set_read_fn(png_ptr, file, PNG_read_function);

    //  Log("(%) Read Function Set");

    png_set_sig_bytes(png_ptr, 8);
    int png_transform =
        PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_PACKSWAP;
    png_read_png(png_ptr, info_ptr, NULL, NULL);

    // initialize the png_image members
    pis->width = png_get_image_width(png_ptr, info_ptr);
    pis->height = png_get_image_height(png_ptr, info_ptr);
    pis->pixels = new RGBA[pis->width * pis->height];

    //  Log("(%) members initialized");

    if (png_get_rows(png_ptr, info_ptr) == NULL) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        vclose(file);
        delete pis;
        return NULL;
    }

    //  Log("(%) PNG_GET_ROWS successful");

    // decode based on pixel depth
    int bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    int num_channels = png_get_channels(png_ptr, info_ptr);
    void **row_pointers = (void **)png_get_rows(png_ptr, info_ptr);

    if (bit_depth == 8 && num_channels == 4) {
        for (int i = 0; i < pis->height; i++) {
            RGBA *row = (RGBA *)(row_pointers[i]);
            for (int j = 0; j < pis->width; j++)
                pis->pixels[i * pis->width + j] = row[j];
        }
    } else if (bit_depth == 8 && num_channels == 3) {
        for (int i = 0; i < pis->height; i++) {
            RGB *row = (RGB *)(row_pointers[i]);
            for (int j = 0; j < pis->width; j++) {
                RGBA p = {row[j].red, row[j].green, row[j].blue, 255};
                pis->pixels[i * pis->width + j] = p;
            }
        }
    } else if (bit_depth == 8 && num_channels == 2) {
        png_colorp palette;
        int num_palette = 0;
        png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);

        // if there is no palette, use black and white
        if (num_palette == 0) {
            for (int i = 0; i < pis->height; i++) {
                byte *row = (byte *)(row_pointers[i]);
                for (int j = 0; j < pis->width; j++) {
                    pis->pixels[i * pis->width + j].red = row[j * 2];
                    pis->pixels[i * pis->width + j].green = row[j * 2];
                    pis->pixels[i * pis->width + j].blue = row[j * 2];
                    pis->pixels[i * pis->width + j].alpha = row[j * 2 + 1];
                }
            }
        } else  // otherwise use the palette
        {
            for (int i = 0; i < pis->height; i++) {
                byte *row = (byte *)(row_pointers[i]);
                for (int j = 0; j < pis->width; j++) {
                    pis->pixels[i * pis->width + j].red =
                        palette[row[j * 2]].red;
                    pis->pixels[i * pis->width + j].green =
                        palette[row[j * 2]].green;
                    pis->pixels[i * pis->width + j].blue =
                        palette[row[j * 2]].blue;
                    pis->pixels[i * pis->width + j].alpha = row[j * 2 + 1];
                }
            }
        }
    } else if (bit_depth == 8 && num_channels == 1) {
        png_colorp palette;
        int num_palette = 0;
        png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);

        png_bytep trans;
        int num_trans = 0;
        png_color_16p trans_values;
        png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, &trans_values);

        // if there is no palette, use black and white
        if (num_palette == 0) {
            for (int i = 0; i < pis->height; i++) {
                byte *row = (byte *)(row_pointers[i]);
                for (int j = 0; j < pis->width; j++) {
                    byte alpha = 255;
                    for (int k = 0; k < num_trans; k++) {
                        if (trans[k] == row[j]) {
                            alpha = 0;
                        }
                    }
                    pis->pixels[i * pis->width + j].red = row[j];
                    pis->pixels[i * pis->width + j].green = row[j];
                    pis->pixels[i * pis->width + j].blue = row[j];
                    pis->pixels[i * pis->width + j].alpha = alpha;
                }
            }
        } else  // otherwise use the palette
        {
            for (int i = 0; i < pis->height; i++) {
                byte *row = (byte *)(row_pointers[i]);
                for (int j = 0; j < pis->width; j++) {
                    byte alpha = 255;
                    for (int k = 0; k < num_trans; k++) {
                        if (trans[k] == row[j]) {
                            alpha = 0;
                        }
                    }
                    pis->pixels[i * pis->width + j].red = palette[row[j]].red;
                    pis->pixels[i * pis->width + j].green =
                        palette[row[j]].green;
                    pis->pixels[i * pis->width + j].blue = palette[row[j]].blue;
                    pis->pixels[i * pis->width + j].alpha = alpha;
                }
            }
        }
    } else if (bit_depth == 4 && num_channels == 1) {
        png_colorp palette;
        int num_palette;
        png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);

        for (int i = 0; i < pis->height; i++) {
            RGBA *dst = pis->pixels + i * pis->width;

            byte *row = (byte *)(row_pointers[i]);
            for (int j = 0; j < pis->width / 2; j++) {
                byte p1 = *row >> 4;
                byte p2 = *row & 0xF;

                dst->red = palette[p1].red;
                dst->green = palette[p1].green;
                dst->blue = palette[p1].blue;
                dst->alpha = 255;
                dst++;

                dst->red = palette[p2].red;
                dst->green = palette[p2].green;
                dst->blue = palette[p2].blue;
                dst->alpha = 255;
                dst++;

                row++;
            }

            if (pis->width % 2) {
                byte p = *row >> 4;
                dst->red = palette[p].red;
                dst->green = palette[p].green;
                dst->blue = palette[p].blue;
                dst->alpha = 255;
                dst++;
            }
        }
    } else if (bit_depth == 1 && num_channels == 1) {
        png_colorp palette;
        int num_palette;
        png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);

        for (int i = 0; i < pis->height; i++) {
            RGBA *dst = pis->pixels + i * pis->width;

            int mask = 1;
            byte *p = (byte *)(row_pointers[i]);

            for (int j = 0; j < pis->width; j++) {
                dst->red = palette[(*p & mask) > 0].red;
                dst->green = palette[(*p & mask) > 0].green;
                dst->blue = palette[(*p & mask) > 0].blue;
                dst->alpha = 255;
                dst++;

                mask <<= 1;
                if (mask == 256) {
                    p++;
                    mask = 1;
                }
            }
        }
    } else {
        delete[] pis->pixels;
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        vclose(file);
        delete pis;
        return NULL;
    }

    //  Log("(%) Decoding Successful!");

    // we're done
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    vclose(file);
    return pis;
}

/******************************************
*  static void CDECL PNG_read_function()  *
*******************************************
* Group:   Graphics
* Job:     Reads a PNG file
* Returns: nothing.
******************************************/
static void CDECL PNG_read_function(
    png_structp png_ptr, png_bytep data, png_size_t length) {
    VFILE *file = (VFILE *)png_get_io_ptr(png_ptr);
    vread(data, length, file);
}
