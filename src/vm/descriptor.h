/* vm/descriptor.h - checking and parsing of field / method descriptors

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Edwin Steiner

   Changes:

   $Id: descriptor.h 2181 2005-04-01 16:53:33Z edwin $

*/


#ifndef _DESCRIPTOR_H
#define _DESCRIPTOR_H

/* forward typedefs ***********************************************************/

typedef struct descriptor_pool descriptor_pool;

#include "vm/global.h"
#include "vm/references.h"
#include "vm/tables.h"

/* data structures ************************************************************/ 

/*----------------------------------------------------------------------------*/
/* Descriptor Pools                                                           */
/*                                                                            */
/* A descriptor_pool is a temporary data structure used during loading of     */
/* a class. The descriptor_pool is used to allocate the table of              */
/* constant_classrefs the class uses, and for parsing the field and method    */
/* descriptors which occurr within the class. The inner workings of           */
/* descriptor_pool are not important for outside code.                        */
/*                                                                            */
/* You use a descriptor_pool as follows:                                      */
/*                                                                            */
/* 1. create one with descriptor_pool_new                                     */
/* 2. add all explicit class references with descriptor_pool_add_class        */
/* 3. add all field/method descriptors with descriptor_pool_add               */
/* 4. call descriptor_pool_create_classrefs                                   */
/*    You can now lookup classrefs with descriptor_pool_lookup_classref       */
/* 5. call descriptor_pool_alloc_parsed_descriptors                           */
/* 6. for each field descriptor call descriptor_pool_parse_field_descriptor   */
/*    for each method descriptor call descriptor_pool_parse_method_descriptor */
/* 7. call descriptor_pool_get_parsed_descriptors                             */
/*                                                                            */
/* IMPORTANT: The descriptor_pool functions use DNEW and DMNEW for allocating */
/*            memory which can be thrown away when the steps above have been  */
/*            done.                                                           */
/*----------------------------------------------------------------------------*/

struct descriptor_pool {
	classinfo         *referer;
	u4                 fieldcount;
	u4                 methodcount;
	u4                 paramcount;
	u4                 descriptorsize;
	u1                *descriptors;
	u1                *descriptors_next;
	hashtable          descriptorhash;
	constant_classref *classrefs;
	hashtable          classrefhash;
	u1                *descriptor_kind;       /* useful for debugging */
	u1                *descriptor_kind_next;  /* useful for debugging */
};


/* data structures for parsed field/method descriptors ************************/

struct typedesc {
	constant_classref *classref;   /* class reference for TYPE_ADR types      */
	u1                 type;       /* TYPE_??? constant                       */
	u1                 arraydim;   /* array dimension (0 if no array)         */
};

struct methoddesc {
	s2                 paramcount; /* number of parameters                    */
	s2                 paramslots; /* like above but LONG,DOUBLE count twice  */
	typedesc           returntype; /* parsed descriptor of the return type    */
	typedesc           paramtypes[1]; /* parameter types, variable length!    */
};


/* function prototypes ********************************************************/

/* descriptor_debug_print_typedesc *********************************************
 
   Print the given typedesc to the given stream

   IN:
	   file.............stream to print to
	   d................the parsed descriptor

*******************************************************************************/

void descriptor_debug_print_typedesc(FILE *file,typedesc *d);

/* descriptor_debug_print_methoddesc *******************************************
 
   Print the given methoddesc to the given stream

   IN:
	   file.............stream to print to
	   d................the parsed descriptor

*******************************************************************************/

void descriptor_debug_print_methoddesc(FILE *file,methoddesc *d);

/* descriptor_pool_new *********************************************************
 
   Allocate a new descriptor_pool

   IN:
       referer..........class for which to create the pool

   RETURN VALUE:
       a pointer to the new descriptor_pool

*******************************************************************************/

descriptor_pool * descriptor_pool_new(classinfo *referer);

/* descriptor_pool_add_class ***************************************************
 
   Add the given class reference to the pool

   IN:
       pool.............the descriptor_pool
	   name.............the class reference to add

   RETURN VALUE:
       true.............reference has been added
	   false............an exception has been thrown

*******************************************************************************/

bool descriptor_pool_add_class(descriptor_pool *pool,utf *name);

/* descriptor_pool_add *********************************************************
 
   Check the given descriptor and add it to the pool

   IN:
       pool.............the descriptor_pool
	   desc.............the descriptor to add. Maybe a field or method desc.

   RETURN VALUE:
       true.............descriptor has been added
	   false............an exception has been thrown

*******************************************************************************/

bool descriptor_pool_add(descriptor_pool *pool,utf *desc);

/* descriptor_pool_create_classrefs ********************************************
 
   Create a table containing all the classrefs which were added to the pool

   IN:
       pool.............the descriptor_pool

   OUT:
       *count...........if count is non-NULL, this is set to the number
	                    of classrefs in the table

   RETURN VALUE:
       a pointer to the constant_classref table

*******************************************************************************/

constant_classref * descriptor_pool_create_classrefs(descriptor_pool *pool,
													 s4 *count);

/* descriptor_pool_lookup_classref *********************************************
 
   Return the constant_classref for the given class name

   IN:
       pool.............the descriptor_pool
	   classname........name of the class to look up

   RETURN VALUE:
       a pointer to the constant_classref, or
	   NULL if an exception has been thrown

*******************************************************************************/

constant_classref * descriptor_pool_lookup_classref(descriptor_pool *pool,utf *classname);

/* descriptor_pool_alloc_parsed_descriptors ************************************
 
   Allocate space for the parsed descriptors

   IN:
       pool.............the descriptor_pool

   NOTE:
       This function must be called after all descriptors have been added
	   with descriptor_pool_add.

*******************************************************************************/

void descriptor_pool_alloc_parsed_descriptors(descriptor_pool *pool);

/* descriptor_pool_parse_field_descriptor **************************************
 
   Parse the given field descriptor

   IN:
       pool.............the descriptor_pool
	   desc.............the field descriptor

   RETURN VALUE:
       a pointer to the parsed field descriptor, or
	   NULL if an exception has been thrown

   NOTE:
       descriptor_pool_alloc_parsed_descriptors must be called (once) before this
	   function is used.

*******************************************************************************/

typedesc * descriptor_pool_parse_field_descriptor(descriptor_pool *pool,utf *desc);

/* descriptor_pool_parse_method_descriptor *************************************
 
   Parse the given method descriptor

   IN:
       pool.............the descriptor_pool
	   desc.............the method descriptor

   RETURN VALUE:
       a pointer to the parsed method descriptor, or
	   NULL if an exception has been thrown

   NOTE:
       descriptor_pool_alloc_parsed_descriptors must be called (once) before this
	   function is used.

*******************************************************************************/

methoddesc * descriptor_pool_parse_method_descriptor(descriptor_pool *pool,utf *desc);

/* descriptor_pool_get_parsed_descriptors **************************************
 
   Return a pointer to the block of parsed descriptors

   IN:
       pool.............the descriptor_pool

   OUT:
   	   *size............if size is non-NULL, this is set to the size of the
	                    parsed descriptor block (in u1)

   RETURN VALUE:
       a pointer to the block of parsed descriptors,
	   NULL if there are no descriptors in the pool

   NOTE:
       descriptor_pool_alloc_parsed_descriptors must be called (once) before this
	   function is used.

*******************************************************************************/

void * descriptor_pool_get_parsed_descriptors(descriptor_pool *pool,
											  s4 *size);

/* descriptor_pool_get_sizes ***************************************************
 
   Get the sizes of the class reference table and the parsed descriptors

   IN:
       pool.............the descriptor_pool

   OUT:
       *classrefsize....set to size of the class reference table
	   *descsize........set to size of the parsed descriptors

   NOTE:
       This function may only be called after both
	       descriptor_pool_create_classrefs, and
		   descriptor_pool_alloc_parsed_descriptors
	   have been called.

*******************************************************************************/

void descriptor_pool_get_sizes(descriptor_pool *pool,
							   u4 *classrefsize,u4 *descsize);

/* descriptor_debug_print_typedesc *********************************************
 
   Print the given typedesc to the given stream

   IN:
	   file.............stream to print to
	   d................the parsed descriptor

*******************************************************************************/

void descriptor_debug_print_typedesc(FILE *file,typedesc *d);

/* descriptor_debug_print_methoddesc *******************************************
 
   Print the given methoddesc to the given stream

   IN:
	   file.............stream to print to
	   d................the parsed descriptor

*******************************************************************************/

void descriptor_debug_print_methoddesc(FILE *file,methoddesc *d);

/* descriptor_pool_debug_dump **************************************************
 
   Print the state of the descriptor_pool to the given stream

   IN:
       pool.............the descriptor_pool
	   file.............stream to print to

*******************************************************************************/

void descriptor_pool_debug_dump(descriptor_pool *pool,FILE *file);

/* macros for descriptor parsing **********************************************/

/* XXX These should be moved to descriptor.c */

/* SKIP_FIELDDESCRIPTOR:
 * utf_ptr must point to the first character of a field descriptor.
 * After the macro call utf_ptr points to the first character after
 * the field descriptor.
 *
 * CAUTION: This macro does not check for an unexpected end of the
 * descriptor. Better use SKIP_FIELDDESCRIPTOR_SAFE.
 */
#define SKIP_FIELDDESCRIPTOR(utf_ptr)							\
	do { while (*(utf_ptr)=='[') (utf_ptr)++;					\
		if (*(utf_ptr)++=='L')									\
			while(*(utf_ptr)++ != ';') /* skip */; } while(0)

/* SKIP_FIELDDESCRIPTOR_SAFE:
 * utf_ptr must point to the first character of a field descriptor.
 * After the macro call utf_ptr points to the first character after
 * the field descriptor.
 *
 * Input:
 *     utf_ptr....points to first char of descriptor
 *     end_ptr....points to first char after the end of the string
 *     errorflag..must be initialized (to false) by the caller!
 * Output:
 *     utf_ptr....points to first char after the descriptor
 *     errorflag..set to true if the string ended unexpectedly
 */
#define SKIP_FIELDDESCRIPTOR_SAFE(utf_ptr,end_ptr,errorflag)			\
	do { while ((utf_ptr) != (end_ptr) && *(utf_ptr)=='[') (utf_ptr)++;	\
		if ((utf_ptr) == (end_ptr))										\
			(errorflag) = true;											\
		else															\
			if (*(utf_ptr)++=='L') {									\
				while((utf_ptr) != (end_ptr) && *(utf_ptr)++ != ';')	\
					/* skip */;											\
				if ((utf_ptr)[-1] != ';')								\
					(errorflag) = true; }} while(0)


#endif /* _DESCRIPTOR_H */


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
