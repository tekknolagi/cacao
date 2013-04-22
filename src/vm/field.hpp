/* src/vm/field.hpp - field functions header

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


#ifndef _FIELD_HPP
#define _FIELD_HPP

/* forward typedefs ***********************************************************/

typedef struct fieldinfo fieldinfo; 


#include "config.h"
#include "vm/types.hpp"

#include "vm/descriptor.hpp"
#include "vm/global.hpp"
#include "vm/utf8.hpp"

struct classinfo;
struct classbuffer;
struct constant_FMIref;

/* fieldinfo ******************************************************************/

struct fieldinfo {	      /* field of a class                                 */

	/* CAUTION: The first field must be a pointer that is never the same      */
	/*          value as CLASSREF_PSEUDO_VFTBL! This is used to check whether */
	/*          a constant_FMIref has been resolved.                          */

	classinfo *clazz;     /* needed by typechecker. Could be optimized        */
	                      /* away by using constant_FMIref instead of         */
	                      /* fieldinfo throughout the compiler.               */

	s4         flags;     /* ACC flags                                        */
	s4         type;      /* basic data type                                  */
	Utf8String name;      /* name of field                                    */
	Utf8String descriptor;/* JavaVM descriptor string of field                */
	Utf8String signature; /* Signature attribute string                       */
	typedesc  *parseddesc;/* parsed descriptor                                */

	int32_t    offset;    /* offset from start of object (instance variables) */
	imm_union *value;     /* storage for static values (class variables)      */
};


/* function prototypes ********************************************************/

#ifdef __cplusplus
extern "C" {
#endif

bool       field_load(classbuffer *cb, fieldinfo *f, descriptor_pool *descpool);
classinfo *field_get_type(fieldinfo *f);
void       field_free(fieldinfo *f);

java_handle_bytearray_t *field_get_annotations(fieldinfo *f);

#if !defined(NDEBUG)
void field_printflags(fieldinfo *f);
void field_print(fieldinfo *f);
void field_println(fieldinfo *f);
void field_fieldref_print(constant_FMIref *fr);
void field_fieldref_println(constant_FMIref *fr);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _FIELD_HPP */


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
