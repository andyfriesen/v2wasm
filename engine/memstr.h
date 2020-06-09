
#ifndef MEMSTR_INC
#define MEMSTR_INC

#include <iostream>
#include <stdio.h>

#include "linked.h"

class string_t;
class memorystream_t : public linked_node {
   protected:
    unsigned long F_hunksize;
    unsigned long F_capacity;
    unsigned char *F_data;

    unsigned long F_size;
    long F_position;

    unsigned long getcapacity() const { return F_capacity; }
    int setcapacity(unsigned long capacity);
    void destroy() {
        delete F_data;
        F_data = NULL;
    }

   public:
    int compare(void *c) { return ((memorystream_t *)c)->F_size > F_size; }

    enum origin_t { soFromBeginning, soFromCurrent, soFromEnd };

    memorystream_t()
        : F_hunksize(256),
          F_capacity(0),
          F_data(NULL),
          F_size(0),
          F_position(0) {}
    memorystream_t(const void *source, long length)
        : F_hunksize(256),
          F_capacity(0),
          F_data(NULL),
          F_size(0),
          F_position(0) {
        write(source, length);
    }
    ~memorystream_t() { clear(); }

    const void *getdata() const { return F_data; }
    const void *getposdata() const { return F_data + F_position; }

    void free() {
        if (F_data) destroy();
    }
    void clear() {
        setposition(0);
        setsize(0);
    }

    unsigned long gethunksize() const { return F_hunksize; }
    void sethunksize(unsigned long hunksize) {
        F_hunksize = (hunksize < 1) ? 1 : hunksize;
    }
    long getposition() const { return F_position; }
    void setposition(long position) { F_position = position; }

    long seek(long offset, origin_t origin);

    int setsize(unsigned long size);
    unsigned long getsize() const { return F_size; }
    int optimize();

    // interface w/ raw data
    long read(void *dest, long length);
    long write(const void *source, long length);
    // interface w/ string_t instances
    void readstring(string_t &s, long length);
    // interface w/ memorystream_t instances
    long read(memorystream_t &dest, long length);
    long write(const memorystream_t &source, long length);

    long insert(long position, const void *source, long length);
    long remove(long position, long length);

    int loadfromfile(const char *filename);
    int savetofile(const char *filename) const;

    void diagnostics(std::ostream &os);
};

#endif  // MEMSTR_INC
