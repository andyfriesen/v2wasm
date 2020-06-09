
#ifndef STRING_INC
#define STRING_INC

#include <iostream.h>
#include <string.h>

#include "memstr.h"

class string_t : public memorystream_t {
   public:
    unsigned long length() const { return F_position - 1; }

    string_t() : memorystream_t("", 1) {}
    string_t(char ch) : memorystream_t(" ", 2) { F_data[0] = ch; }
    string_t(const char *text) : memorystream_t(text, strlen(text) + 1) {}
    string_t(const string_t &rhs)
        : memorystream_t(rhs.F_data, rhs.F_position) {}

    string_t &operator=(const string_t &rhs);
    string_t &operator+=(const string_t &rhs);
    string_t &operator+(const string_t &rhs);

    string_t operator()(long offset, long count) const;
    string_t left(long count) const { return operator()(0, count); }
    string_t right(long count) const {
        return operator()(length() - count, count);
    }
    string_t mid(long offset, long count) const {
        return operator()(offset, count);
    }

    operator const char *() const { return (const char *)F_data; }
    operator char *() const {
        char *newdata = new char[F_position];
        memcpy(newdata, F_data, F_position);
        return newdata;
    }

    string_t upper();
    string_t lower();
    bool operator==(const string_t &s) const;
    bool operator<(const string_t &s) const;
    bool operator<=(const string_t &s) const;
    bool operator>(const string_t &s) const;
    bool operator>=(const string_t &s) const;
    char operator[](int offset) const;
};

#endif  // STRING_INC