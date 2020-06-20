
#ifndef VECTOR_INC
#define VECTOR_INC

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "linked.h"

template <class T> class vector_t {
  protected:
    unsigned long F_hunksize;
    unsigned long F_capacity;
    T* F_data;
    unsigned long F_size;

    unsigned long getcapacity() const { return F_capacity; }
    int setcapacity(unsigned long capacity) {
        // special case for 0 capacity
        if (capacity < 1) {
            free();
            F_data = NULL;
            F_capacity = 0;
            return 1;
        }
        T* newdata = new T[capacity * F_hunksize];
        if (!newdata)
            // failure
            return 0;
        // copy existing data into new data buffer
        for (int n = 0; n < F_size; n++)
            newdata[n] = F_data[n];
        // free existing data buffer and reassign pointers
        free();
        F_data = newdata;
        // reset capacity member
        F_capacity = capacity;
        // success
        return 1;
    }

    int setsize(unsigned long size) {
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

    void destroy() {
        delete[] F_data;
        F_data = NULL;
    }

  public:
    void free() {
        if (F_data)
            destroy();
    }
    void clear() { F_size = 0; }

    vector_t<T>() : F_hunksize(16), F_capacity(0), F_data(NULL), F_size(0) {}
    vector_t<T>(const vector_t<T>& rhs)
        : F_hunksize(16), F_capacity(0), F_data(NULL), F_size(0) {
        setsize(rhs.F_size);
        for (int n = 0; n < rhs.F_size; n++)
            F_data[n] = rhs.F_data[n];
    }

    ~vector_t<T>() { free(); }

    // assignment op
    vector_t<T>& operator=(const vector_t<T>& rhs) {
        setsize(rhs.F_size);
        for (int n = 0; n < rhs.F_size; n++)
            F_data[n] = rhs.F_data[n];
        return *this;
    }

    T& operator[](int n) const { return F_data[n]; }

    int optimize() {
        // determine optimal capacity for current size
        unsigned long newcapacity = (F_size + (F_hunksize - 1)) / F_hunksize;
        // if the new capacity is smaller than the current
        // capacity, reallocate the buffer.
        if (newcapacity < F_capacity)
            return setcapacity(newcapacity);
        // success
        return 1;
    }

    unsigned long size() const { return F_size; }

    unsigned long top() { return size() - 1; }
    void push_top(T item) {
        setsize(1 + size());
        F_data[top()] = item;
    }
    // for quickly adding storage
    void push_count(int count) {
        if (count < 1)
            return;
        setsize(count + size());
    }
    void pop_top() { --F_size; }

    unsigned long gethunksize() const { return F_hunksize; }
    void sethunksize(unsigned long hunksize) {
        F_hunksize = (hunksize < 1) ? 1 : hunksize;
    }
};

#endif // VECTOR_INC
