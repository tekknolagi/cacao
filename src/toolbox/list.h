/* src/toolbox/list.h - synchronized linked list

   Copyright (C) 1996-2005, 2006, 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


#ifndef _TOOLBOX_LIST_H
#define _TOOLBOX_LIST_H

#include "config.h"

#include <stdint.h>

#include "vm/global.h"

#include "threads/mutex.hpp"


#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------- interface description -----------------------------

The list management with this module works like this:
	
	- to be used in a list, a structure must have an element of type
	  'listnode'.
	  
	- there needs to be a structure of type 'list'.
	
	- the function list_init(l, nodeoffset) initializes the structure.
	  nodeoffset is the offset of the 'listnode' from the start of the
	  structure in bytes.
	  
	- The remaining functions provide inserting, removing and searching.
	  
This small example aims to demonstrate correct usage:



	void bsp() {
		struct node {
			listnode linkage;
			int value;
			} a,b,c, *el;
			
		list l;
		
		a.value = 7;
		b.value = 9;
		c.value = 11;
		
		list_init (&l, OFFSET(struct node,linkage) );
		list_addlast (&l, a);
		list_addlast (&l, b);
		list_addlast (&l, c);
		
		e = list_first (&l);
		while (e) {
			printf ("Element: %d\n", e->value);
			e = list_next (&l,e);
			}
	}
	
	
	The output from this program should be:
		7
		9
		11



The reason for the usage of 'nodeoffset' is that this way, the same node can
part of different lists (there must be one 'listnode' element for every
distinct list).

*/

/* listnode_t *****************************************************************/

typedef struct listnode_t listnode_t;

struct listnode_t {
	listnode_t *next;
	listnode_t *prev;
};


/* list_t *********************************************************************/

typedef struct list_t list_t;

struct list_t {
	Mutex*             mutex;           /* threads lock object                */
	listnode_t        *first;
	listnode_t        *last;
	int                nodeoffset;
	int                size;            /* number of elements in the list     */
};


/* function prototypes ********************************************************/

list_t *list_create(int nodeoffset);
list_t *list_create_dump(int nodeoffset);

void    list_free(list_t *l);

void    list_lock(list_t *l);
void    list_unlock(list_t *l);

void    list_add_first(list_t *l, void *element);
void    list_add_last(list_t *l, void *element);
void    list_add_before(list_t *l, void *element, void *newelement);

void    list_remove(list_t *l, void *element);

void   *list_first(list_t *l);
void   *list_last(list_t *l);

void   *list_next(list_t *l, void *element);
void   *list_prev(list_t *l, void *element);

#ifdef __cplusplus
}
#endif

#endif /* _TOOLBOX_LIST_H */


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
