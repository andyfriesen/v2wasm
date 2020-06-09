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

#include "str.h" //#include "verge.h"

string_t& string_t::operator =(const string_t& rhs)
{
	if (&rhs != this)
	{
		F_position = 0;
		write(rhs.F_data, rhs.F_position);
	}
	return *this;
}

string_t& string_t::operator+=(const string_t& rhs)
{
	F_position--; // backtrack over terminator
	write(rhs.F_data, rhs.F_position);
	return *this;
}

/*
// don't want this behavior; expressions such as x=y+z imply x=(y+=z); // blech
string_t& string_t::operator+ (const string_t& rhs)
{
	return *this += rhs;
}
*/

string_t string_t::operator()(long offset, long count) const
{
string_t s;

	if (offset >= length() || count < 1)
		return s;

	if (offset < 0)
	{
		count += offset;
		offset = 0;
	}
	if (offset + count > length())
		count = length() - offset;

	s.F_position--; // backtrack over terminator
	s.write(F_data + offset, count);
	s.write("", 1);

	return s;
}

string_t string_t::upper()
{
string_t s = *this;
	strupr((char*)s.F_data);
	return s;
}

string_t string_t::lower()
{
string_t s = *this;
	strlwr((char*)s.F_data);
	return s;
}

bool string_t::operator !=(const string_t& s) const
	{ return (const char*)F_data != s; }

bool string_t::operator==(const string_t& s) const
	{ return (const char*)F_data == s; }

bool string_t::operator< (const string_t& s) const
	{ return (const char*)F_data <  s; }

bool string_t::operator<=(const string_t& s) const
	{ return (const char*)F_data <= s; }

bool string_t::operator> (const string_t& s) const
	{ return (const char*)F_data >  s; }

bool string_t::operator>=(const string_t& s) const
	{ return (const char*)F_data >= s; }

char string_t::operator[](long offset) const
{
	return (char)((offset < 0 || offset >= length()) ? 0 : F_data[offset]);
}