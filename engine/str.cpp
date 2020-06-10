
#include "str.h"
#include "verge.h"
#include <iostream>

string_t& string_t::operator=(const string_t& rhs) {
    if (&rhs != this) {
        F_position = 0;
        write(rhs.F_data, rhs.F_position);
    }
    return *this;
}

string_t& string_t::operator+=(const string_t& rhs) {
    F_position--; // backtrack over terminator
    write(rhs.F_data, rhs.F_position);
    return *this;
}

string_t& string_t::operator+(const string_t& rhs) { return *this += rhs; }

string_t string_t::operator()(long offset, long count) const {
    string_t s;

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

string_t string_t::upper() {
    string_t s = *this;
    strupr((char*)s.F_data);
    return s;
}

string_t string_t::lower() {
    string_t s = *this;
    strlwr((char*)s.F_data);
    return s;
}

bool string_t::operator==(const string_t& s) const {
    return V_strcmp((char*)F_data, (char*)s.F_data) == 0;
}

bool string_t::operator<(const string_t& s) const {
    return V_strcmp((char*)F_data, (char*)s.F_data) < 0;
}

bool string_t::operator<=(const string_t& s) const {
    return V_strcmp((char*)F_data, (char*)s.F_data) <= 0;
}

bool string_t::operator>(const string_t& s) const {
    return V_strcmp((char*)F_data, (char*)s.F_data) > 0;
}

bool string_t::operator>=(const string_t& s) const {
    return V_strcmp((char*)F_data, (char*)s.F_data) >= 0;
}

char string_t::operator[](int offset) const {
    return (char)((offset < 0 || offset >= length()) ? 0 : F_data[offset]);
}
