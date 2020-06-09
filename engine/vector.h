
#ifndef VECTOR_INC
#define VECTOR_INC

#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>

// vector/stack class
template <class T>
	class vector_t
{
protected:
	int F_hunksize,
		F_size,
		F_capacity;
	T*	F_data;

	int getcapacity() const
		{ return F_capacity; }
	int setcapacity(int capacity)
	{
                if (capacity < 1)                       // special case for 0 capacity
		{
			free();
			F_data = NULL;
			F_capacity = 0;
			return 1;
		}
		T* newdata = new T [capacity * F_hunksize];
                if (!newdata) return 0;                 // failure
                for (int n = 0; n < F_size; n++)
                 newdata[n] = F_data[n];                // copy existing data into new data buffer

                free();                                 // free existing data buffer and reassign pointers
		F_data = newdata;
                F_capacity = capacity;                  // reset capacity member
                return 1;                               // success
	}

	int setsize(int size)
	{
                if (size == F_size)                     // trivial return
			return 1;
		int newcapacity = (size + (F_hunksize - 1)) / F_hunksize;
                                                        // determine optimal capacity for requested size
        
	// capacity, reallocate the buffer.
                if (newcapacity > F_capacity)           // if the new capacity is larger than the current
		{
                  if (!setcapacity(newcapacity))        // if setcapacity fails, so do we
                   return 0;
		}
	// reset size
		F_size = size;
	// if the new size was 0, force an optimization
		if (!F_size) optimize();
	// success
		return 1;
	}

	void destroy()
		{ delete[] F_data; F_data = NULL; }

public:
	void free()
		{ if (F_data) destroy(); }
	void clear()
		{ F_size = 0; }

	vector_t<T>()
		: F_hunksize(256), F_capacity(0)
		, F_data(NULL), F_size(0)
		{ }
	vector_t<T>(const vector_t<T>& rhs)
		: F_hunksize(rhs.F_hunksize), F_capacity(0)
		, F_data(NULL), F_size(0)
	{
		push_vector(rhs);
	}
~vector_t<T>()
	{ free(); }

// assignment op
	vector_t<T>& operator=(const vector_t<T>& rhs)
		{ push_vector(rhs); return *this; }

	T& push_vector(const vector_t<T>& v)
	{
	// hot damn
		T* base = &push_count(v.size());
		for (int n = 0; n < v.size(); n++)
			base[n] = v[n];
		return *base;
	}

	T& operator[](int n) const
	{
	static T dummy;
		return (n >= 0 && n < size()) ? F_data[n] : dummy;
	}

	int optimize()
	{
	// determine optimal capacity for current size
		int newcapacity = (F_size + (F_hunksize - 1)) / F_hunksize;
	// if the new capacity is smaller than the current
	// capacity, reallocate the buffer.
		if (newcapacity < F_capacity) return setcapacity(newcapacity);
	// success
		return 1;
	}

	int size() const
		{ return F_size; }

// last entry in vector
	T& top()
		{ return F_data[size() - 1]; }

// push an uninitialized element and return it
	virtual T& push()
		{ setsize(1 + size()); return top(); }
// push an element and return it
	T& push(T item)
		{ return push() = item; }
	T& insert(int index, T item)
		{
			push();
			for (int n = size() - 1; n > index; --n)
				F_data[n] = F_data[n - 1];
			return F_data[index] = item;
		}

// for quickly adding storage
	T& push_count(int count)
	{
	// on error, just return current top
		if (count < 1) return top();

		setsize(count + size());
	// return first of the 'count' elements we just 'pushed'
		return F_data[size() - count];
	}
// return last entry in vector before the pop
	T& pop()
		{ return F_data[--F_size]; }

	int gethunksize() const
		{ return F_hunksize; }
	void sethunksize(int hunksize)
		{ F_hunksize = (hunksize < 1) ? 1 : hunksize; }
};

template <class T>
	class queue_t
		: public vector_t<T>
{
protected:
	int F_head, F_tail;

public:
	queue_t()
		: F_head(0), F_tail(0)
		{ setsize(256); }

// override these
	T& pop()
	{
		if (F_head != F_tail)
			return F_data[F_head++%F_size];
		else
			return F_data[F_tail];
	}
	virtual T& push()
	{
		return F_data[++F_tail%F_size];
	}
};

#endif // VECTOR_INC
