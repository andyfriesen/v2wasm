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

#ifndef STRING_INC
#define STRING_INC

#include <iostream.h>
#include <string.h>

#include "memstr.h"

class string_t: public memorystream_t
{
public:
	unsigned long length() const
		{ return F_position - 1; }

	string_t()
	: memorystream_t("", 1)
		{ }
	string_t(char ch)
	: memorystream_t(" ", 2)
		{ F_data[0] = ch; }
	string_t(const char* text)
	: memorystream_t(text, strlen(text)+1)
		{ }
	string_t(const string_t& rhs)
	: memorystream_t(rhs.F_data, rhs.F_position)
		{ }

	string_t& operator =(const string_t& rhs);
	string_t& operator+=(const string_t& rhs);

	string_t operator()(long offset, long count) const;
	string_t left(long count) const
		{ return operator()(0, count); }
	string_t right(long count) const
		{ return operator()(length() - count, count); }
	string_t mid(long offset, long count) const
		{ return operator()(offset, count); }

	char operator[](long offset) const;
	const char* c_str() const { return (const char*)F_data; }

	string_t upper();
	string_t lower();

	bool operator !=(const string_t& s) const;
	bool operator ==(const string_t& s) const;
	bool operator < (const string_t& s) const;
	bool operator <=(const string_t& s) const;
	bool operator > (const string_t& s) const;
	bool operator >=(const string_t& s) const;

	void recalc()
		{
                        setsize(strlen((const char*)F_data) + 1);
			F_position = F_size;
		}
};

inline bool operator< (const char* lhs, const string_t& rhs)
	{ return strcmp(lhs, rhs.c_str()) < 0; }

inline bool operator> (const char* lhs, const string_t& rhs)
	{ return strcmp(lhs, rhs.c_str()) > 0; }

inline bool operator<=(const char* lhs, const string_t& rhs)
	{ return !(lhs > rhs); }

inline bool operator>=(const char* lhs, const string_t& rhs)
	{ return !(lhs < rhs); }

inline bool operator==(const char* lhs, const string_t& rhs)
	{ return strcmp(lhs, rhs.c_str()) == 0; }

inline bool operator!=(const char* lhs, const string_t& rhs)
	{ return !(lhs == rhs); }

// this is the behavior we really want
inline string_t operator+(const string_t& lhs, const string_t& rhs)
	{ return string_t(lhs) += rhs; }

inline string_t operator+ (const char* lhs, const string_t& rhs)
	{ return string_t(lhs) += rhs; }

#endif // STRING_INC
