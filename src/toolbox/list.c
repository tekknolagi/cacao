/* src/toolbox/list.c - double linked list

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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

   $Id: list.c 7601 2007-03-28 23:02:50Z michi $

*/


#include "config.h"

#include <stdlib.h>

#include "vm/types.h"

#include "mm/memory.h"

#if defined(ENABLE_THREADS)
# include "threads/native/lock.h"
#else
# include "threads/none/lock.h"
#endif

#include "toolbox/list.h"


/* list_create *****************************************************************

   Allocates a new list and initializes the lock object.

*******************************************************************************/

list *list_create(s4 nodeoffset)
{
	list *l;

	l = NEW(list);

#if defined(ENABLE_THREADS)
	lock_init_object_lock((java_objectheader *) l);
#endif

	l->first      = NULL;
	l->last       = NULL;
	l->nodeoffset = nodeoffset;

	return l;
}


/* list_create_dump ************************************************************

   Allocates a new list on the dump memory.

   ATTENTION: This list does NOT initialize the locking object!!!

*******************************************************************************/

list *list_create_dump(s4 nodeoffset)
{
	list *l;

	l = DNEW(list);

	l->first      = NULL;
	l->last       = NULL;
	l->nodeoffset = nodeoffset;

	return l;
}


void list_add_first(list *l, void *element)
{
	listnode *ln;

	ln = (listnode *) (((u1 *) element) + l->nodeoffset);

	LOCK_MONITOR_ENTER(l);

	if (l->first) {
		ln->prev       = NULL;
		ln->next       = l->first;
		l->first->prev = ln;
		l->first       = ln;
	}
	else {
		ln->prev = NULL;
		ln->next = NULL;
		l->last  = ln;
		l->first = ln;
	}

	LOCK_MONITOR_EXIT(l);
}


/* list_add_last ***************************************************************

   Adds the element as last element.

*******************************************************************************/

void list_add_last(list *l, void *element)
{
	LOCK_MONITOR_ENTER(l);

	list_add_last_unsynced(l, element);

	LOCK_MONITOR_EXIT(l);
}


/* list_add_last_unsynced ******************************************************

   Adds the element as last element but does NO locking!

   ATTENTION: Use this function with care!!!

*******************************************************************************/

void list_add_last_unsynced(list *l, void *element)
{
	listnode *ln;

	ln = (listnode *) (((u1 *) element) + l->nodeoffset);

	if (l->last) {
		ln->prev      = l->last;
		ln->next      = NULL;
		l->last->next = ln;
		l->last       = ln;
	}
	else {
		ln->prev = NULL;
		ln->next = NULL;
		l->last  = ln;
		l->first = ln;
	}
}


/* list_add_before *************************************************************

   Adds the element newelement to the list l before element.

   [ A ] <-> [ newn ] <-> [ n ] <-> [ B ]

*******************************************************************************/

void list_add_before(list *l, void *element, void *newelement)
{
	listnode *ln;
	listnode *newln;

	ln    = (listnode *) (((u1 *) element) + l->nodeoffset);
	newln = (listnode *) (((u1 *) newelement) + l->nodeoffset);

	LOCK_MONITOR_ENTER(l);

	/* set the new links */

	newln->prev = ln->prev;
	newln->next = ln;

	if (newln->prev)
		newln->prev->next = newln;

	ln->prev = newln;

	/* set list's first and last if necessary */

	if (l->first == ln)
		l->first = newln;

	if (l->last == ln)
		l->last = newln;

	LOCK_MONITOR_EXIT(l);
}


/* list_remove ***************************************************************

   Removes the element.

*******************************************************************************/

void list_remove(list *l, void *element)
{
	LOCK_MONITOR_ENTER(l);

	list_remove_unsynced(l, element);

	LOCK_MONITOR_EXIT(l);
}


/* list_remove_unsynced ********************************************************

   Removes the element but does NO locking!

   ATTENTION: Use this function with care!!!

*******************************************************************************/

void list_remove_unsynced(list *l, void *element)
{
	listnode *ln;

	ln = (listnode *) (((u1 *) element) + l->nodeoffset);
	
	if (ln->next)
		ln->next->prev = ln->prev;
	else
		l->last = ln->prev;

	if (ln->prev)
		ln->prev->next = ln->next;
	else
		l->first = ln->next;

	ln->next = NULL;
	ln->prev = NULL;
}

 
/* list_first ******************************************************************

   Returns the first element of the list.

*******************************************************************************/

void *list_first(list *l)
{
	void *el;

	LOCK_MONITOR_ENTER(l);

	el = list_first_unsynced(l);

	LOCK_MONITOR_EXIT(l);

	return el;
}


/* list_first_unsynced *********************************************************

   Returns the first element of the list, but does NO locking!

   ATTENTION: Use this function with care!!!

*******************************************************************************/

void *list_first_unsynced(list *l)
{
	void *el;

	if (l->first == NULL)
		el = NULL;
	else
		el = ((u1 *) l->first) - l->nodeoffset;

	return el;
}


/* list_last *******************************************************************

   Returns the last element of the list.

*******************************************************************************/

void *list_last(list *l)
{
	void *el;

	LOCK_MONITOR_ENTER(l);

	el = list_last_unsynced(l);

	LOCK_MONITOR_EXIT(l);

	return el;
}


/* list_last_unsynced **********************************************************

   Returns the last element of the list, but does NO locking!

   ATTENTION: Use this function with care!!!

*******************************************************************************/

void *list_last_unsynced(list *l)
{
	void *el;

	if (l->last == NULL)
		el = NULL;
	else
		el = ((u1 *) l->last) - l->nodeoffset;

	return el;
}


/* list_next *******************************************************************

   Returns the next element of element from the list.

*******************************************************************************/

void *list_next(list *l, void *element)
{
	void *el;

	LOCK_MONITOR_ENTER(l);

	el = list_next_unsynced(l, element);

	LOCK_MONITOR_EXIT(l);

	return el;
}


/* list_next_unsynced **********************************************************

   Returns the next element of element from the list, but does NO
   locking!

   ATTENTION: Use this function with care!!!

*******************************************************************************/

void *list_next_unsynced(list *l, void *element)
{
	listnode *ln;
	void     *el;

	ln = (listnode *) (((u1 *) element) + l->nodeoffset);

	if (ln->next == NULL)
		el = NULL;
	else
		el = ((u1 *) ln->next) - l->nodeoffset;

	return el;
}

	
/* list_prev *******************************************************************

   Returns the previous element of element from the list.

*******************************************************************************/

void *list_prev(list *l, void *element)
{
	void *el;

	LOCK_MONITOR_ENTER(l);

	el = list_prev_unsynced(l, element);

	LOCK_MONITOR_EXIT(l);

	return el;
}


/* list_prev_unsynced **********************************************************

   Returns the previous element of element from the list, but does NO
   locking!

   ATTENTION: Use this function with care!!!

*******************************************************************************/

void *list_prev_unsynced(list *l, void *element)
{
	listnode *ln;
	void     *el;

	ln = (listnode *) (((u1 *) element) + l->nodeoffset);

	if (ln->prev == NULL)
		el = NULL;
	else
		el = ((u1 *) ln->prev) - l->nodeoffset;

	return el;
}


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
