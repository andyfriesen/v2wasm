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

#include "memstr.h"
#include "str.h"

using std::endl;
using std::ostream;

int memorystream_t::setcapacity(unsigned long capacity) {
    // special case for 0 capacity
    if (capacity < 1) {
        free();
        F_data = NULL;
        F_capacity = 0;
        return 1;
    }
    unsigned char* newdata = new unsigned char[capacity * F_hunksize];
    if (!newdata)
        // failure
        return 0;
    // copy existing data into new data buffer
    memcpy(newdata, F_data, F_size);
    // free existing data buffer and reassign pointers
    free();
    F_data = newdata;
    // reset capacity member
    F_capacity = capacity;
    // success
    return 1;
}

long memorystream_t::seek(long offset, origin_t origin)
// offset should be >= 0 for soFromBeginning.
// offset should be <  0 for soFromEnd.
{
    switch (origin) {
    case soFromBeginning:
        F_position = 0 + offset;
        break;
    case soFromCurrent:
        F_position = F_position + offset;
        break;
    case soFromEnd:
        F_position = F_size + offset;
        break;
    }
    return F_position;
}

int memorystream_t::setsize(unsigned long size) {
    // trivial return
    if (size == F_size)
        // success
        return 1;
    // determine optimal capacity for requested size
    unsigned long newcapacity = (size + (F_hunksize - 1)) / F_hunksize;
    // if the new capacity is larger than the current
    // capacity, reallocate the buffer.
    if (newcapacity > F_capacity) {
        // if setcapacity fails, so do we
        if (!setcapacity(newcapacity))
            return 0;
    }
    // reset size
    F_size = size;
    // if the new size was 0, force an optimization
    if (!F_size)
        optimize();
    // success
    return 1;
}

int memorystream_t::optimize() {
    // determine optimal capacity for current size
    unsigned long newcapacity = (F_size + (F_hunksize - 1)) / F_hunksize;
    // if the new capacity is smaller than the current
    // capacity, reallocate the buffer.
    if (newcapacity < F_capacity)
        return setcapacity(newcapacity);
    // success
    return 1;
}

long memorystream_t::read(void* dest, long length) {
    // heinous length
    if (length < 0)
        return -1;
    if (!dest)
        return -2;
    // prefix out of bounds
    if (F_position < 0)
        return -3;
    // suffix out of bounds
    if (F_position >= F_size)
        return -5;
    // suffix clip
    if (F_position + length >= F_size)
        length = F_size - F_position;
    // perform read
    memcpy((char*)dest, F_data + F_position, length);
    // update position
    F_position += length;
    // return successfully copied bytes
    return length;
}

long memorystream_t::write(const void* source, long length) {
    // heinous length
    if (length < 0)
        return -1;
    if (!source)
        return -2;
    // prefix out of bounds
    if (F_position < 0)
        return -3;
    // extend buffer?
    if (F_position + length > F_size) {
        // if setsize fails, so do we
        if (!setsize(F_position + length))
            return -4;
    }
    // perform write
    memcpy(F_data + F_position, (char*)source, length);
    // update position
    F_position += length;
    // return successfully copied bytes
    return length;
}

long memorystream_t::write(char c, long length) {
    // heinous length
    if (length < 0)
        return -1;
    // prefix out of bounds
    if (F_position < 0)
        return -3;
    // extend buffer?
    if (F_position + length > F_size) {
        // if setsize fails, so do we
        if (!setsize(F_position + length))
            return -4;
    }
    // perform write
    memset(F_data + F_position, c, length);
    // update position
    F_position += length;
    // return successfully copied bytes
    return length;
}

int memorystream_t::loadfromfile(const char* filename)
// self-explanatory ^_^
{
    if (!filename)
        return -1;
    FILE* fp = fopen(filename, "rb");
    if (!fp)
        return -2;
    fseek(fp, 0, SEEK_END);
    setsize(ftell(fp));
    fseek(fp, 0, SEEK_SET);
    if (fread(F_data, 1, F_size, fp) != F_size) {
        fclose(fp);
        return -3;
    }
    fclose(fp);
    setposition(F_size);
    return 1;
}

int memorystream_t::savetofile(const char* filename) const
// self-explanatory ^_^
{
    if (!filename)
        return -1;
    FILE* fp = fopen(filename, "wb");
    if (!fp)
        return -2;
    if (fwrite(F_data, 1, F_size, fp) != F_size) {
        fclose(fp);
        return -3;
    }
    fclose(fp);
    return 1;
}

void memorystream_t::diagnostics(ostream& os) {
    os << "---" << endl
       << "contents: " << (char*)F_data << endl
       << "    size: " << F_size << endl
       << "position: " << F_position << endl
       << "capacity: " << F_capacity << endl;
}

void memorystream_t::readstring(VCString& s, long length) {
    s.setsize(length + 1);
    read(s.F_data, length);
    s.F_data[length] = '\0';
}

long memorystream_t::insert(long position, const void* source, long length) {
    // heinous length
    if (length < 0)
        return -1;
    // non-existent source
    if (!source)
        return -2;
    // prefix out of bounds
    if (position < 0)
        return -3;

    // expand buffer
    if (!setsize(F_size + length))
        return -4;

    // move length bytes at insert position ahead length bytes
    memcpy(F_data + position + length, F_data + position, length);
    // reset position to requested location and write the new bytes in
    F_position = position;

    return write(source, length);
}

long memorystream_t::remove(long position, long length) {
    // heinous length
    if (length < 0)
        return -1;
    // prefix out of bounds
    if (position < 0)
        return -3;

    // suffix clip
    if (position + length >= F_size)
        length = F_size - position;

    // determine remaining bytes after position+length (inclusive)
    int remainder = F_size - length;
    // move remainder bytes at position+length to position
    memcpy(F_data + position, F_data + position + length, remainder);

    // now shrink the buffer
    if (!setsize(F_size - length))
        return -4;

    // and reset position in buffer
    F_position = position;
    // return successfully removed bytes
    return length;
}

long memorystream_t::read(memorystream_t& dest, long length) {
    // *** added layer of protection for
    // *** interfacing memorystream_t instances

    // dest prefix out of bounds
    if (dest.F_position < 0)
        return -3;
    // dest suffix out of bounds
    if (dest.F_position >= dest.F_size)
        return -5;
    // dest suffix clip
    if (dest.F_position + length >= dest.F_size)
        length = dest.F_size - dest.F_position;

    // perform read
    return read(dest.F_data + dest.F_position, length);
}

long memorystream_t::write(const memorystream_t& source, long length) {
    // *** added layer of protection for
    // *** interfacing memorystream_t instances

    // source prefix out of bounds
    if (source.F_position < 0)
        return -3;
    // source suffix out of bounds
    if (source.F_position >= source.F_size)
        return -5;
    // source suffix clip
    if (source.F_position + length >= source.F_size)
        length = source.F_size - source.F_position;

    // perform write
    return write(source.F_data + source.F_position, length);
}
