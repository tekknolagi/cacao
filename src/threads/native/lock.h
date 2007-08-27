/* src/threads/native/lock.h - lock implementation

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

*/


#ifndef _LOCK_H
#define _LOCK_H

#include "config.h"

#include <pthread.h>

#include "vm/types.h"

#include "vm/global.h"



/* typedefs *******************************************************************/

typedef struct lock_record_t             lock_record_t;
typedef struct lock_waiter_t             lock_waiter_t;
typedef struct lock_hashtable_t          lock_hashtable_t;


/* lock_waiter_t ***************************************************************

   List node for storing a waiting thread.

*******************************************************************************/

struct lock_waiter_t {
	struct threadobject *waiter;         /* the waiting thread                */
	lock_waiter_t       *next;           /* next in list                      */
};


/* lock_record_t ***************************************************************

   Lock record struct representing an inflated ("fat") lock.

*******************************************************************************/

struct lock_record_t {
	java_object_t       *object;             /* object for which this lock is */
	struct threadobject *owner;              /* current owner of this monitor */
	s4                   count;              /* recursive lock count          */
	pthread_mutex_t      mutex;              /* mutex for synchronizing       */
	lock_waiter_t       *waiters;            /* list of threads waiting       */
	lock_record_t       *hashlink;           /* next record in hash chain     */
};


/* lock_hashtable_t ************************************************************
 
   The global hashtable mapping objects to lock records.

*******************************************************************************/

struct lock_hashtable_t {
	pthread_mutex_t      mutex;       /* mutex for synch. access to the table */
	u4                   size;        /* number of slots                      */
	u4                   entries;     /* current number of entries            */
	lock_record_t      **ptr;         /* the table of slots, uses ext. chain. */
};


/* defines ********************************************************************/

#define LOCK_INIT_OBJECT_LOCK(o) lock_init_object_lock((java_object_t *) (o))

#define LOCK_MONITOR_ENTER(o)    lock_monitor_enter((java_object_t *) (o))
#define LOCK_MONITOR_EXIT(o)     lock_monitor_exit((java_object_t *) (o))

#endif /* _LOCK_H */


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
 * vim:noexpandtab:sw=4:ts=4:
 */
