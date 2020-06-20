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

#ifndef LINKED_INC
#define LINKED_INC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class linked_node {
    linked_node* nex;
    linked_node* pre;

  public:
    linked_node* next() { return nex; }
    linked_node* prev() { return pre; }
    void set_next(linked_node* pn) { nex = pn; }
    void set_prev(linked_node* pn) { pre = pn; }

    virtual int compare(void* c) {
        (void)c;
        return 0;
    } // default is = (equal)

    virtual ~linked_node() {}
    linked_node() { nex = pre = NULL; }
};

class linked_list {
    linked_node* fn;
    linked_node* cn;
    int nn;

  public:
    ~linked_list();
    linked_list(linked_node* first = NULL);

    void insert_head(linked_node* pn);
    void insert_tail(linked_node* pn);
    void insert_before_current(linked_node* pn);
    void insert_after_current(linked_node* pn);
    void insert(linked_node* pn);

    linked_node* current() const { return cn; }
    linked_node* head() const { return fn; }
    linked_node* tail() const { return head()->prev(); }

    linked_node* get_node(int x);

    void set_current(linked_node* pn) { cn = pn; }

    void go_head() { cn = head(); }
    void go_tail() { cn = head()->prev(); }
    void go_next() { cn = current()->next(); }
    void go_prev() { cn = current()->prev(); }

    int number_nodes() const { return nn; }
    int node_number(linked_node* pn);

    int unlink(linked_node* pn);
};

#endif // LINKED_INC
