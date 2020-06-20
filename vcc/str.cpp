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

VCString& VCString::operator=(const VCString& rhs) {
    if (&rhs != this) {
        F_position = 0;
        write(rhs.F_data, rhs.F_position);
    }
    return *this;
}

VCString& VCString::operator+=(const VCString& rhs) {
    F_position--; // backtrack over terminator
    write(rhs.F_data, rhs.F_position);
    return *this;
}

/*
// don't want this behavior; expressions such as x=y+z imply x=(y+=z); // blech
VCString& VCString::operator+ (const VCString& rhs)
{
        return *this += rhs;
}
*/

VCString VCString::operator()(long offset, long count) const {
    VCString s;

    if (offset >= length() || count < 1)
        return s;

    if (offset < 0) {
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

VCString VCString::upper() {
    VCString s = *this;
    strupr((char*)s.F_data);
    return s;
}

VCString VCString::lower() {
    VCString s = *this;
    strlwr((char*)s.F_data);
    return s;
}

bool VCString::operator!=(const VCString& s) const {
    return (const char*)F_data != s;
}

bool VCString::operator==(const VCString& s) const {
    return (const char*)F_data == s;
}

bool VCString::operator<(const VCString& s) const {
    return (const char*)F_data < s;
}

bool VCString::operator<=(const VCString& s) const {
    return (const char*)F_data <= s;
}

bool VCString::operator>(const VCString& s) const {
    return (const char*)F_data > s;
}

bool VCString::operator>=(const VCString& s) const {
    return (const char*)F_data >= s;
}

char VCString::operator[](long offset) const {
    return (char)((unsigned long)(offset) >= length() ? 0 : F_data[offset]);
}

void strupr(char* s) {
    while (*s) {
        *s = toupper(*s);
        ++s;
    }    
}

void strlwr(char* s) {
    while (*s) {
        *s = tolower(*s);
        ++s;
    }
}

int stricmp(const char* a, const char* b) {
    while (*a && *b) {        
        if (!*a) return -1;
        if (!*b) return 1;

        char aa = tolower(*a);
        char bb = tolower(*b);
        if (aa < bb) return -1;
        if (aa > bb) return 1;

        ++a;
        ++b;
    }

    return 0;
}
