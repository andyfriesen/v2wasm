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

// ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
// ³                          The VERGE Engine                           ³
// ³              Copyright (C)1998 BJ Eirich (aka vecna)                ³
// ³                           Imaging module                            ³
// ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
// Mega kudos to aen for porting that GIF code.

/*
	mod log:

    28  November    2000
    <tSB>
    *   Obligatory tweaking to better suit the new graphics driver interface.

    8       January         2000
    <aen>
    *       Revamp/tinkering. Interface is: Image_LoadBuf(), Image_Width(), Image_Length()

    23      December        1999
    <aen>
    *       Fixed PCX loading code. Again.
*/

#include <conio.h>
#include "verge.h"
#include "i_png.h"

// TODO: cleanup

///////////////////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION /////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

static int _24bit		=0;

static int image_width	=0;
static int image_length	=0;

struct pcx_header_t
{
	char	manufacturer;
	char	version;
	char	encoding;
	char	bits_per_pixel;
	unsigned short	xmin, ymin;
	unsigned short	xmax, ymax;
	unsigned short	hres, vres;
	char	palette[48];
	char	reserved;
	char	color_planes;
	word	bytes_per_line;
	word	palette_type;
	char	filler[58];
};

//#pragma off (unreferenced);

static void read_pcx_line(VFILE* vf, byte* start, short skip, int width)
{
	int c, n, run;

	n = 0;
	while (n < width)
	{
		run = 1;
		c = vgetc(vf) & 0xff;
		if ((c & 0xc0) == 0xc0)
		{
			run = c & 0x3f;
			c = vgetc(vf);
		}
		n += run;
		do
		{
			*start = (byte) c;
			start += skip;
			run -= 1;
		}
		while (run);
	}
}

//#pragma on (unreferenced);

static byte* Image_LoadPCXBuf(const char* fname)
{
	VFILE*	vf;
	byte	*dest, *buf;
	int		c, n, run;
	pcx_header_t	h;

	vf	=vopen(fname);
	if (!vf)
	{
		Log(va("Image_LoadPCXBuf: %s: unable to open", fname));
		return (byte *)0;
	}

// validate header info
	vread(&h.manufacturer, 1, vf);
	if (h.manufacturer != 10)
	{
		Log(va("Image_LoadPCXBuf: %s: not a valid PCX file (manufacturer %d)", fname, h.manufacturer));
		return (byte *)0;
	}
	vread(&h.version, 1, vf);
	if (h.version != 5)
	{
		Log(va("Image_LoadPCXBuf: %s: not a valid PCX file (version %d)", fname, h.version));
		return (byte *)0;
	}
	vread(&h.encoding, 1, vf);
	if (h.encoding != 1)
	{
		Log(va("Image_LoadPCXBuf: %s: not a valid PCX file (encoding %d)", fname, h.encoding));
		return (byte *)0;
	}
	vread(&h.bits_per_pixel, 1, vf);
	if (h.bits_per_pixel != 8)
	{
		Log(va("Image_LoadPCXBuf: %s: not a valid PCX file (bpp %d)", fname, h.bits_per_pixel));
		return (byte *)0;
	}

	vread(&h.xmin, 2, vf);
	vread(&h.ymin, 2, vf);
	vread(&h.xmax, 2, vf);
	vread(&h.ymax, 2, vf);
	vread(&h.hres, 2, vf);
	vread(&h.vres, 2, vf);

	vseek(vf, 48L, SEEK_CUR);	// skip colormap

	vread(&h.reserved, 1, vf);

	vread(&h.color_planes, 1, vf);

	_24bit = (3 == h.color_planes);

	vread(&h.bytes_per_line, 2, vf);
	vread(&h.palette_type, 2, vf);

	vseek(vf, 128L, SEEK_SET);	// skip filler

	image_width		=h.xmax-h.xmin+1;
	image_length	=h.ymax-h.ymin+1;

// decompress image into buffer
	buf	=(byte *)valloc(image_width*image_length*(_24bit?3:1)+4, "Image_LoadPCXBuf:buf", OID_IMAGE);
	if (!buf)
	{
		Sys_Error("Image_LoadPCXBuf: %s: memory exhausted on buf", fname);
	}
	dest=buf;
	n=0;
	int row=image_length;
	if (_24bit)
	{
		for (n=0; n<row; n++)
		{
			read_pcx_line(vf, dest+0, 3, h.bytes_per_line);
			read_pcx_line(vf, dest+1, 3, h.bytes_per_line);
			read_pcx_line(vf, dest+2, 3, h.bytes_per_line);
			dest += (image_width*3);
		}
	}
	else
	{
		while (row) //n<image_width*image_length)
		{
			n=0;
			while (n<h.bytes_per_line)
			{
				run=1;
				c=vgetc(vf);
				if ((c & 192) == 192)
				{
					run=c & 63;
					c=vgetc(vf);
				}
				V_memset(dest+n, c, run);
				n+=run;
			}
			dest+=image_width;
			row--;
		}
	}

// palette avail?
	if (12 == vgetc(vf))
	{
		vread(gfx.gamepal, 3*256, vf);
		for (int n = 0; n < 3*256; n += 1) {
			gfx.gamepal[n] >>= 2;
		}
	}
	else if (!_24bit)
	{
		Log(va("%s: does not contain palette info", fname));
	}

	vclose(vf);

	return buf;
}

/*struct RGBQUAD // redundant in Win32
{
	unsigned char r, g, b, a;
};*/

/*typedef struct tagRGBQUAD {
	BYTE	rgbBlue;
	BYTE	rgbGreen;
	BYTE	rgbRed;
	BYTE	rgbReserved;
} RGBQUAD;
*/

static RGBQUAD bmppalette[1024];

static byte* Image_LoadBMPBuf(const char* fname)
{ byte pad[4], *buf;
  VFILE* bmpfile;
  int y,x;
  byte r,g,b;
  byte colordepth;
  byte *p;
  char pal[768];

  if (!(bmpfile=vopen(fname))) Sys_Error("Could not open BMP file %s.",fname);
  vseek(bmpfile, 18, SEEK_SET);
  vread(&image_width, 4, bmpfile); 
  vread(&image_length, 4, bmpfile);
  vseek(bmpfile, 54, SEEK_SET);
  vread(bmppalette, 1024, bmpfile);

  vseek(bmpfile,28,SEEK_SET);
  vread(&colordepth,1,bmpfile);
  if (colordepth==8)
   {
    buf=(byte*) valloc(2*image_width*image_length, "LoadBMPBuf:buf", OID_IMAGE);
    p=(byte*) valloc(image_width*image_length, "LoadBMPBuf:p", OID_IMAGE);
    vseek(bmpfile,54+1024,SEEK_SET);
    for (y=0; y<256; y++)
     {
      pal[y*3  ]=bmppalette[y].rgbBlue  >> 2;
      pal[y*3+1]=bmppalette[y].rgbGreen >> 2;
      pal[y*3+2]=bmppalette[y].rgbRed   >> 2;
     }

    for (y=image_length-1; y>=0; y--)
     {
      vread((char *)((int)p+y*image_width),image_width,bmpfile);
      //vread(pad,image_width%4,bmpfile);
      vread(pad,((4 - (image_width % 4)) % 4),bmpfile); // skip a few bytes
     }
    gfx.SetPalette((byte*)pal);
    vclose(bmpfile);
    return p;
   }
  else if (colordepth==24)
   {
    if (gfx.bpp==1)
     {
      vclose(bmpfile);
      Sys_Error(va("Attempted to load truecolour BMP %s in 256 colour mode.  Aborting",fname));
     }

    _24bit=1;
    buf=(byte*) valloc(3*image_width*image_length, "LoadBMPBuf:buf", OID_IMAGE);
    int ofs;
    vseek(bmpfile,54,SEEK_SET);
    for (y=image_length-1; y>=0; y--)
     {
      for (x=0; x<=image_width-1; x++)
       {
        vread(&b,1,bmpfile);
        vread(&g,1,bmpfile);
        vread(&r,1,bmpfile);
        ofs=(y*image_width+x)*3;
        buf[ofs  ]=r;
        buf[ofs+1]=g;
        buf[ofs+2]=b;
 
 //       q[y*image_width+x]=gfx.PackPixel(r,g,b);//(r<<rshift)+(g<<gshift)+(b<<bshift);
       }
     //vread(pad,image_width%4,bmpfile);
      vread(pad,((4 - ((image_width*3) % 4)) % 4),bmpfile); // skip a few bytes
     }
    vclose(bmpfile);
    return buf;
   }
  else
   {
    Sys_Error("BMP file %s has incorrect color depth. (only 8 and 24 bit images are supported)\r\nWhat the hell are you doing trying to\r\nload a 4 color bmp for?",fname);
   }
 return NULL;
}

// ========================== GIF Imaging routines ===========================

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

typedef signed char s8;
typedef signed short s16;
typedef signed long s32;

typedef struct
{
	u8 bits;
	u8 background;
	u8 * palette;
	u8 * image;
	s16 wide, deep;
} gif_image_info;

typedef struct
{
	char sig[7];
	s16 screenwide, screendeep;
	u8 hflags;
	u8 background;
	u8 aspect;
} gif_header;

typedef struct
{
	s16 top, left;
	s16 wide, deep;
	u8 iflags;
} gif_image_descriptor;

#define NO_CODE					-1
#define	ERROR_EOF				0
#define ERROR_BAD_CODE			1
#define ERROR_BAD_HEADER		2
#define ERROR_BAD_STARTCODE		3
#define ERROR_BAD_FIRST_CODE	4
#define ERROR_BAD_FILE			5
#define ERROR_NO_IMAGE			6

/*
static char* gif_error_messages[] =
{
	"Unexpected end of file\n",
	"Bad code\n",
	"Bad gif header\n",
	"Bad symbol size\n",
	"Bad first code\n",
	"Error opening file\n",
	"This file doesn't contain an image\n"
};
*/

// read colour palette, vga palette values are 6 bit numbers
// while gif allows for 8 bit so shift right to get correct colours

static u8* gif_read_palette(FILE* fp, s32 bytes)
{
	s32		n;
	u8*		block;

	block = (u8*) valloc(3*256, "gifDec pal", OID_TEMP);
	if (!block)
	{
		Sys_Error("gif_read_palette: memory exhausted on block");
	}

	bytes = bytes/3*3;
	for (n = 0; n < bytes; n += 1)
		block[n] = (u8) (fgetc(fp) >> 2);

	return block;
}

// read a block of bytes into memory
static s32 block_mem_read(FILE* fp, u8* buffer, s32 bytes)
{
	s32 status = 0;

	status = fread(buffer, 1, bytes, fp);
	if (status != bytes) return EOF;

	return TRUE;
}

// read a unsigned 16 bit value from file, low byte first; note that this
// is reverse endian-ness (ie. fwrite(&s,1,2,fp); writes high byte first).

static s16 read_word_lbf(FILE* fp)
{
	s32 a, b;

	a = fgetc(fp);
	b = fgetc(fp);

	return (s16) ((b << 8) | a);
}

// read the GIF file header structure into a sequence
static gif_header* get_gif_header(FILE* fp)
{
	gif_header* h = 0L;

	h = (gif_header *)valloc(sizeof(gif_header), "gifDec header", OID_TEMP);
	if (!h)
	{
		Sys_Error("get_gif_header: memory exhausted on h");
	}

	fread(h->sig, 1, 6, fp);
	h->sig[6] = 0;

	if (V_memcmp(h->sig, "GIF", 3) != 0)
		return (gif_header *)0; //NULL;

	h->screenwide = read_word_lbf(fp);	image_width	=h->screenwide;
	h->screendeep = read_word_lbf(fp);	image_length=h->screendeep;
	h->hflags = (u8)fgetc(fp);
	h->background = (u8)fgetc(fp);
	h->aspect = (u8)fgetc(fp);

	return h;
}

// gif file can contain more than one image,
// each image is preceeded by a header structure
static gif_image_descriptor* get_image_descriptor(FILE* fp)
{
	gif_image_descriptor* id = 0L;

	id =(gif_image_descriptor *)valloc(sizeof(gif_image_descriptor), "gifDec image", OID_TEMP);
	if (!id)
	{
		Sys_Error("get_image_descriptor: memory exhausted on id");
	}

	id->left = read_word_lbf(fp);
	id->top = read_word_lbf(fp);
	id->wide = read_word_lbf(fp);
	id->deep = read_word_lbf(fp);
	id->iflags = (u8)fgetc(fp);

	return id;
}

static u16 word_mask_table[] =
{
	0x0000, 0x0001, 0x0003, 0x0007,
	0x000F, 0x001F, 0x003F, 0x007F,
	0x00FF, 0x01FF, 0x03FF, 0x07FF,
	0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF
};

static u8 inc_table[] = { 8,8,4,2,0 };
static u8 start_table[] = { 0,4,2,1,0 };

// enables me to use indices as per EUPHORiA (ie. converts to C's 0 base)
#define eui(i) ((i)-1)

// unpack an LZW compressed image
// returns a sequence containing screen display lines of the image
static u8* unpack_image(FILE* fp, s32 start_code_size, u32 width, u32 depth, u32 flags)
{
	u8* buffer;
	u8* line_buffer;

	u16 first_code_stack[4096];
	u16 last_code_stack[4096];
	u16 code_stack[4096];

	s32 bits_left;
	s32 clear_code;
	s32 code_size;
	s32 code_size2;
	s32 next_code;
	s32 this_code;
	s32 old_token;
	s32 current_code;
	s32 old_code;
	s32 block_size=0;
	s32 line;
	s32 a_byte;
	s32 pass;
	s32 u;

	u8 b[256]; // read buffer; for block reads
	u8* p; // current byte in read buffer
	u8* q; // last byte in read buffer + 1

	line_buffer = (u8 *) valloc(width, "gd_linebuf", OID_TEMP);
	if (!line_buffer)
	{
		Sys_Error("unpack_image: memory exhausted on line_buffer");
	}
	buffer      = (u8 *) valloc(width * depth, "gif image", OID_TEMP);
	if (!buffer)
	{
		Sys_Error("unpack_image: memory exhausted on buffer");
	}

	a_byte	=0;
	line	=0;
	pass	=0;
	bits_left	=8;

	if (start_code_size < 2 || start_code_size > 8)
	{
		vfree(line_buffer);
		vfree(buffer);

		Log("unpack_image: ERROR_BAD_STARTCODE"); // bad symbol size

		return (byte *)0;
	}

	p = b;
	q = b;

	clear_code = 1 << start_code_size; //pow(2, start_code_size);
	next_code = clear_code + 2;
	code_size = start_code_size + 1;
	code_size2 = 1 << code_size; //pow(2, code_size);
	old_code = NO_CODE;
	old_token = NO_CODE;

	while (1)
	{
		if (bits_left == 8)
		{
			++p;
			if (p >= q)
			{
				block_size = fgetc(fp);
				if (block_mem_read(fp, b, block_size) == EOF)
				{
					vfree(line_buffer);
					vfree(buffer);

					Log("unpack image: ERROR_EOF");

					return (byte *)0;
				}
				p = b;
				q = b + block_size;
			}
			bits_left = 0;
		}

		this_code = *p;
		current_code = code_size + bits_left;

		if (current_code <= 8)
		{
			*p = (u8)(*p >> code_size);
			bits_left = current_code;
		}
		else
		{
			++p;
			if (p >= q)
			{
				block_size = fgetc(fp);
				if (block_mem_read(fp, b, block_size) == EOF)
				{
					vfree(line_buffer);
					vfree(buffer);

					Log("unpack_image: ERROR_EOF");

					return (byte *)0;
				}
				p = b;
				q = b + block_size;
			}

			this_code |= (*p << (8 - bits_left));

			if (current_code <= 16)
			{
				bits_left = current_code - 8;
				*p = (u8)(*p >> bits_left);
			}
			else
			{
				if (++p >= q)
				{
					block_size = fgetc(fp);
					if (block_mem_read(fp, b, block_size) == EOF)
					{
						vfree(line_buffer);
						vfree(buffer);

						Log("unpack_image: ERROR_EOF");

						return (byte *)0;
					}
					p = b;
					q = b + block_size;
				}

				this_code |= (*p << (16 - bits_left));

				bits_left = current_code - 16;
				*p = (u8)(*p >> bits_left);
			}
		}

		this_code &= word_mask_table[code_size];
		current_code = this_code;

		if (this_code == (clear_code+1) || block_size == 0)
			break;

		if (this_code > next_code)
		{
			vfree(line_buffer);
			vfree(buffer);

			Log("unpack_image: ERROR_BAD_CODE");

			return (byte *)0;
		}

		if (this_code == clear_code)
		{
			next_code = clear_code + 2;
			code_size = start_code_size + 1;
			code_size2 = 1 << code_size; //pow(2, code_size);
			old_code = NO_CODE;
			old_token = NO_CODE;
		}
		else
		{
			u = 1;
			if (this_code == next_code)
			{
				if (old_code == NO_CODE)
				{
					vfree(line_buffer);
					vfree(buffer);

					Log("unpack_image: ERROR_BAD_FIRST_CODE");

					return (byte *)0;
				}

				first_code_stack[eui(u)] = (u16) old_token;
				u++;
				this_code = old_code;
			}

			while (this_code >= clear_code)
			{
				first_code_stack[eui(u)] = last_code_stack[eui(this_code)];
				u++;
				this_code = code_stack[eui(this_code)];
			}

			old_token = this_code;
			while (1)
			{
				line_buffer[a_byte] = (u8)this_code;
				a_byte++;
				if (a_byte >= (signed) width)
				{
					// full image line so add it into screen image
					V_memcpy(buffer + (line * width), line_buffer, width);

					a_byte = 0;
					if (flags & 0x40)
					{
						line += inc_table[pass];
						if (line >= (signed) depth)
						{
							pass++;
							line = start_table[pass];
						}
					}
					else
					{
						line++;
					}
				}

				// no more bytes on stack
				if (u == 1) break;

				u--;
				this_code = first_code_stack[eui(u)];
			}

			if (next_code < 4096 && old_code != NO_CODE)
			{
				code_stack[eui(next_code)] = (u16)old_code;
				last_code_stack[eui(next_code)] = (u16)old_token;
				next_code++;
				if (next_code >= code_size2 && code_size < 12)
				{
					code_size++;
					code_size2 = 1 << code_size; //pow(2, code_size);
				}
			}

			old_code = current_code;
		}
	}

	// completed reading the image so return it
	vfree(line_buffer);

	return buffer;
}

// skip the extension blocks as we are only after the image
static void skip_extension(FILE* fp)
{
	s32 n;
	char temp[256];

	n = fgetc(fp); // throwaway extension function code
	n = fgetc(fp); // get length of block

	while (n > 0 && n != EOF)
	{
		// throwaway block
		fread(temp, 1, n, fp);

		n = fgetc(fp); // get length of next block
	}
}

// unpack the GIF file
// returns ImageInfo sequence containing image and image data
static gif_image_info* unpack_gif(const char* filename)
{
	gif_header*				h	=0;
	gif_image_info*			ii	=0;
	gif_image_descriptor*	id	=0;
	int		i;
	VFILE*	f;
	FILE*	fp;
	s32		c, b;
	u8*		local_palette	=0;

	ii = (gif_image_info *)valloc(sizeof(gif_image_info), "gifDec info", OID_TEMP);
	if (!ii)
	{
		Sys_Error("unpack_gif: memory exhausted on ii");
	}

	f = vopen(filename);
	if (!f)
	{
		vfree(ii);
		Log(va("Could not open GIF file %s.",filename));
		return (gif_image_info *)0;
	}
	fp = f->fp;
	if (!fp)
	{
		vfree(ii);
		Log("\nBad filename");
		return (gif_image_info *)0;
	}

	// file starts with the Logical Screen Descriptor structure
	h	=get_gif_header(fp);

	// Size of Global Color Table
	ii->bits = (u8)((h->hflags & 7) + 1);
	ii->background = h->background;

	// get Global colour palette if there is one
	if (h->hflags & 0x80) // is flags bit 8 set?
	{
		c = 3 << ii->bits; // size of global colour map
		ii->palette = gif_read_palette(fp, c);
		V_memcpy(gfx.gamepal, ii->palette, 3*256); // <aen> aha; hidden bug
		for (i = 0; i < 3*256; i += 1)
			gfx.gamepal[i] >>= 2; //= (byte) (gfx.gamepal[i] >> 2);
		vfree(ii->palette);
	}
	vfree(h);

	c = fgetc(fp);

	while (c == 0x2c || c == 0x21 || c == 0)
	{
		// image separator so unpack the image
		if (c == 0x2c)
		{
			id = get_image_descriptor(fp); // get the Image Descriptor
			// if there is a local Color Table then overwrite the global table
			if (id->iflags & 0x80)
			{
				ii->bits = (u8)((id->iflags & 7) + 1);
				b = 3 << ii->bits;
				if (local_palette)
					vfree(local_palette);
				local_palette = gif_read_palette(fp, b);
				V_memcpy(gfx.gamepal, local_palette, 3*256);
				vfree(local_palette);
			}

			c = fgetc(fp); // get the LZW Minimum Code Size
			ii->image = unpack_image(fp, c, id->wide, id->deep, id->iflags);
			vclose(f);

			// error reading image
			if (!ii->image)
			{
				vfree(ii);
				Log("unpack_gif: error reading image data");
				return (gif_image_info *)0;
			}

			ii->wide = id->wide;
			ii->deep = id->deep;
			vfree(id);

			// return imagedata
			return ii;
		}
		// extension introducer
		else if (c == 0x21)
		{
			skip_extension(fp); // throw the extension away
		}

		c = fgetc(fp);
	}
	// no image?
	return (gif_image_info *)0;
}

static void SizeUpPalette(void)
{
	int		n;

	for (n = 0; n < 3*256; n += 1)
	{
		gfx.gamepal[n] <<= 2;//= (unsigned char) (gfx.pal[n] << 2);
	}
}

static byte* Image_LoadGIFBuf(const char* fname)
{
	gif_image_info*	giip	=0;
	byte*	t	=0;

	giip	=unpack_gif(fname);
	if (!giip)	return (byte *)0;

	image_width	=giip->wide;
	image_length=giip->deep;
	t	=(byte *)giip->image;

	vfree(giip);

	SizeUpPalette();

	return t;
}

static byte* Image_LoadPNGBuf(const char* fname)
{
 int x,y;
 byte* buf;
 byte* p; // current position
 png_image* png;

 png=Import_PNG(fname); 
 if (!png) return NULL; // fail?

 image_length = png->height;          // get the dimensions
 image_width  = png->width;

 buf=(byte*)valloc(image_width*image_length*3,"LoadPNG",OID_IMAGE);
	 //new byte[image_width*image_length*3];           // we'll return a 24 bit image, and let the other image stuff convert it to 16bpp

 p=buf; 
 for (y=0; y<image_length; y++)
  for (x=0; x<image_width; x++)
   {
    *p++ = png->pixels[(y*image_width)+x].red; 
    *p++ = png->pixels[(y*image_width)+x].green; 
    *p++ = png->pixels[(y*image_width)+x].blue; 
    //a = png->pixels[(y*image_width)+x].alpha; // alpha?  not yet
   }
 
 delete png;
 
 _24bit=1;
 return buf;
}

static int DetermineFileType(const char* fname)
{
	const char*	ext	=0;
	int		type	=255;

	V_strlwr((char*)fname);
	ext	=fname+V_strlen(fname)-3;

    if (!V_strcmp(ext, "pcx"))
     type=0;
    else if (!V_strcmp(ext, "gif"))
     type=1; 
    else if (!V_strcmp(ext, "bmp"))
     type=2;
    else if (!V_strcmp(ext, "png"))
     type=3;
	return type;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// INTERFACE //////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

int Image_Width()	{ return image_width;	}
int Image_Length()	{ return image_length;	}

byte* Image_LoadBuf(const char* filename)
{
	byte*	ptr	= 0;

	switch (DetermineFileType(filename))
	{
		case 0: ptr = Image_LoadPCXBuf(filename); break;
		case 1: ptr = Image_LoadGIFBuf(filename); break;
		case 2: ptr = Image_LoadBMPBuf(filename); break;
        case 3: ptr = Image_LoadPNGBuf(filename); break;

		default:
			Log(va("%s: unrecognized image type", filename));
	}

	if (!ptr)
		return ptr;

// heh, error check
    if (_24bit && gfx.bpp==1) Sys_Error(va("Attempted to load truecolour image %s while in 256 colour mode.",filename)); // silly VERGEr

// if in hicolor mode, must convert 8-bit to 16-bit
	if (gfx.bpp>1 && !_24bit)
	{
		word*	hip;
		int		n;

	// buffer for new 16-bit image
		hip = (word *) valloc(image_width*image_length*2, "Image_LoadBuf:hip", 0);
	// convert to 16-bit
		for (n = 0; n < image_width*image_length; n++)
			hip[n] = gfx.Conv8(ptr[n]);

		vfree(ptr);											// free old 8-bit data
		ptr = (byte *) hip;									// reassign new 16-bit data
	}
	if (_24bit)
	{
		word*	hip;
		int		n;
		byte*	p;

	// buffer for new 16-bit image
		hip = (word *) valloc(image_width*image_length*2, "Image_LoadBuf:hip", 0);
	// convert to 16-bit
		p = ptr;
		for (n = 0; n < image_width*image_length; n += 1)
		{
			
			if (p[0]==0xFF && !p[1] && p[2]==0xFF)
				hip[n] = gfx.trans_mask;
			else
				hip[n] = gfx.PackPixel(p[0], p[1], p[2]);
			p += 3;
		}

		vfree(ptr); // free old 8-bit data
		ptr = (byte *) hip;	// reassign new 16-bit data
	}

	return ptr;
}
