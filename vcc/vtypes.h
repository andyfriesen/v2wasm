/*
VERGE 2.5+j (AKA V2k+j) -  A video game creation engine
Copyright (C) 1998-2000  Benjamin Eirich (AKA vecna), et al
Please see authors.txt for a complete list of contributing authors.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/*
	Standard Types Header
	coded by aen

	mod log:
	21	December	1999	Created.
*/

#ifndef types_inc
#define types_inc

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned long u32;
typedef signed long s32;

class rangenum {
	s32 lo,hi;
	s32 n;
public:
	rangenum(s32 l=0, s32 h=0) {
		set_limits(l,h);
		set(0);
	}

	void set_limits(s32 l, s32 h) {
		lo=l;
		hi=h;
	}
	void set(s32 x) {
		if (x<lo || x>hi)
			return;
		n=x;
	}
	void inc(s32 x) {
		set(get()+x);
	}
	s32 get() const { return n; }
	s32 get_lo() const { return lo; }
	s32 get_hi() const { return hi; }
};

#endif // types_inc