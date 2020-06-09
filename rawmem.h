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

/*
	Raw Memory Module Header
	coded by aen

	mod log:
	21	December	1999	Created.
							Tweaked.
*/

#ifndef rawmem_inc
#define rawmem_inc

#include "vtypes.h"

#define RAWMEM_USAGE_STRING_LENGTH 256

extern void die_on_violation(u32 flag);

class rawptr;
class rawmem
{
	// what are we using this memory for?
	char m_use[RAWMEM_USAGE_STRING_LENGTH +1];

	u8* m_data;			// the raw buffer
	u32 m_length;		// user size request for buffer; mostly for suffix corruption detection
	u32 m_hunks;		// hunks currently allocated
	u32 m_touched;		// byte total of memory in-use

	// used by ctors
	void zero_all();
	// total amount of memory this rawmem object currently contains
	u32 bytes_allocated() const;

public:
	rawmem(s32 requested=0, const char *use=0L);
	~rawmem();

	u32 hunks() const;
	u32 touched() const;
	void touch(u32 rhand);
	void untouch();
	u32 length() const;

	void destroy();
	void resize(s32 requested, const char *use=0L);
	void become_string(const char* text);

	// reallocate if hunks required to hold length() bytes is less than
	// the currently allocated number of blocks
	void consolidate();
	void report();

	u8* get(s32 n, u32 z) const;
	u32* get_u32(s32 n) const;
	u16* get_u16(s32 n) const;
	u8* get_u8(s32 n) const;
	u8& operator[](s32 n) const;

	void set_use(const char *use);
	char* get_use() const;

	friend void rawmem_fill_u8(rawptr& dest, u32, s32);
	friend void rawmem_xfer(rawptr& dest, rawptr& source, s32);
};

class rawptr {
	rawmem *raw;
	rangenum offset;
public:
	rawptr(){}
	rawptr(rawmem* r, u32 pos=0) {
		point_to(r);
		set_pos(pos);
	}

	void touch(u32 count) {
		if (!raw) return;
		raw->touch(get_pos()+count);
	}

	void point_to(rawmem* r) {
		if (!r) return;
		raw=r;
		offset.set_limits(0,r->length()-1);
	}
	//rawmem* pointing_to() const { return raw; }

	u8 *get(u32 count) {
		if (!raw) return 0L;
		return raw->get(get_pos(),count);
	}

	void set_pos(u32 x) {
		offset.set(x);
	}
	s32 get_pos() const { return offset.get(); }
	void inc_pos(s32 x) { offset.inc(x); }

	void set_start() {
		set_pos(0);
	}
	void set_end() {
		if (!raw) return;
		set_pos(raw->length()-1);
	}
};

#endif // rawmem_inc
