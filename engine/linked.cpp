/*
Copyright (C) 1998 BJ Eirich (aka vecna)
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "linked.h"

void linked_list::insert_before_current(linked_node* pn) {
    pn->set_prev(current()->prev());
    pn->set_next(current());

    current()->prev()->set_next(pn);
    current()->set_prev(pn);

    ++nn;
}

void linked_list::insert_after_current(linked_node* pn) {
    go_next();
    insert_before_current(pn);
    go_prev();
}

// unlink takes a node out of the linked list, but does not dispose of the
// memory
int linked_list::unlink(linked_node* pn) {
    if (!head())
        return 0;

    // if they want to unlink the first node
    if (head() == pn) {
        head()->prev()->set_next(
            head()->next()); // set the first's last's next to the first's next
        head()->next()->set_prev(
            head()->prev()); // set the first next's last to the first last

        // if there is only one node
        if (head()->next() == head()) {
            // clear the list
            fn = 0;
            cn = 0;
        } else {
            // else point the first pointer to the next node
            fn = pn->next();
        }
        // decrement the number of nodes in the list
        --nn;

        return 1;
    } else {
        // find the node in the list
        go_head();
        go_next();
        while (current() != pn && current() != head()) {
            go_next();
        }
        // is it in the list at all?
        if (current() != head()) {
            // yes unlink the pointers
            current()->prev()->set_next(current()->next());
            current()->next()->set_prev(current()->prev());

            // decrement the number of nodes
            --nn;

            return 1;
        }
    }

    return 0;
}

// this function clears all the nodes in a linked list and dispose of the
// memory for each one by calling it's destructor
linked_list::~linked_list() {
    if (head()) {
        // set the last nodes next to NULL
        // so we can go until we hit NULL
        head()->prev()->set_next(0);
    }

    // repeat until we get to that node
    while (fn != 0) {
        go_head();
        go_next();
        // cn=head()->next();
        delete fn; // delete the old node
        fn = current();
    }
    // clear the list
    cn = 0;
    // set the number of nodes to 0
    nn = 0;
}

// this function returns the node number a node is in a linked list
// it start at the node and goes backwards until it reaches the first
// node
int linked_list::node_number(linked_node* pn) {
    int x = 1;
    while (pn != head()) {
        x++;
        pn = pn->prev();
    }
    return x;
}

// this function returns a pointer to the xth node
linked_node* linked_list::get_node(int x) {
    // start at the first node
    go_head();

    if (x > 0 && nn) {
        x--;
        x %= nn;
        x++;
        // go forward X-1 nodes
        while (x-- > 1) {
            go_next();
        }
    }

    return current();
}

// this function adds a node to the end of a linked_list
void linked_list::insert_tail(linked_node* pn) {
    // if there are no nodes, then this one becomes the first
    if (0 == head()) {
        fn = pn;
        // and it poits to itself for first and last
        head()->set_next(head());
        head()->set_prev(head());

        ++nn;
    } else {
        go_head();
        insert_before_current(pn);
    }
}

// to add a node at the fron of the list, just add it at the end and set
// the first pointer to it
void linked_list::insert_head(linked_node* pn) {
    insert_tail(pn);
    fn = pn;
}

// insert adds a node in the list according to is sort value
void linked_list::insert(linked_node* pn) {
    // if there are nodes, or it belongs at the beginin call add front
    if ((0 == head()) || (pn->compare(head()) > 0)) {
        insert_head(pn);
    }
    // else if it goes at the ned call add_end
    else if (pn->compare(head()->prev()) <= 0) {
        insert_tail(pn);
    }
    // otherwise we have to find the right spot for it.
    else {
        // iter starts at head
        go_head();
        while (current() != head()->prev()) {
            go_next();
            // repeat until we find a value greater than the one we are
            // inserting
            if (pn->compare(current()) > 0) {
                insert_before_current(pn);

                break;
            }
        }
    }
}

linked_list::linked_list(linked_node* first) {
    fn = first;
    cn = first;
    nn = 0;

    if (first) {
        linked_node* prev;

        go_head();
        do {
            ++nn;
            prev = current();
            go_next();
        } while (current() && current() != head());

        if (0 == current()) {
            head()->set_prev(prev);
            prev->set_next(head());
        }
        go_head();
    }
}