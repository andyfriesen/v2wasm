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

#include <iostream>
#include <string.h>

#include "memstr.h"

class VCString : public memorystream_t {
  public:
    unsigned long length() const { return F_position - 1; }

    VCString() : memorystream_t("", 1) {}
    VCString(char ch) : memorystream_t(" ", 2) { F_data[0] = ch; }
    VCString(const char* text) : memorystream_t(text, strlen(text) + 1) {}
    VCString(const VCString& rhs)
        : memorystream_t(rhs.F_data, rhs.F_position) {}

    VCString& operator=(const VCString& rhs);
    VCString& operator+=(const VCString& rhs);

    VCString operator()(long offset, long count) const;
    VCString left(long count) const { return operator()(0, count); }
    VCString right(long count) const {
        return operator()(length() - count, count);
    }
    VCString mid(long offset, long count) const {
        return operator()(offset, count);
    }

    char operator[](long offset) const;
    const char* c_str() const { return (const char*)F_data; }

    VCString upper();
    VCString lower();

    bool operator!=(const VCString& s) const;
    bool operator==(const VCString& s) const;
    bool operator<(const VCString& s) const;
    bool operator<=(const VCString& s) const;
    bool operator>(const VCString& s) const;
    bool operator>=(const VCString& s) const;

    void recalc() {
        setsize(strlen((const char*)F_data) + 1);
        F_position = F_size;
    }
};

inline bool operator<(const char* lhs, const VCString& rhs) {
    return strcmp(lhs, rhs.c_str()) < 0;
}

inline bool operator>(const char* lhs, const VCString& rhs) {
    return strcmp(lhs, rhs.c_str()) > 0;
}

inline bool operator<=(const char* lhs, const VCString& rhs) {
    return !(lhs > rhs);
}

inline bool operator>=(const char* lhs, const VCString& rhs) {
    return !(lhs < rhs);
}

inline bool operator==(const char* lhs, const VCString& rhs) {
    return strcmp(lhs, rhs.c_str()) == 0;
}

inline bool operator!=(const char* lhs, const VCString& rhs) {
    return !(lhs == rhs);
}

// this is the behavior we really want
inline VCString operator+(const VCString& lhs, const VCString& rhs) {
    return VCString(lhs) += rhs;
}

inline VCString operator+(const char* lhs, const VCString& rhs) {
    return VCString(lhs) += rhs;
}

void strupr(char* s);
void strlwr(char* s);
int stricmp(const char* a, const char* b);

#endif // STRING_INC
