/* typeinfo.h - type system used by the type checker

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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

   $Id: typeinfo.h 868 2004-01-10 20:12:10Z edwin $

*/


#ifndef _TYPEINFO_H
#define _TYPEINFO_H

#include "global.h"

/* resolve typedef cycles *****************************************************/

typedef struct typeinfo typeinfo;
typedef struct typeinfo_mergedlist typeinfo_mergedlist;
typedef struct typedescriptor typedescriptor;
typedef struct typevector typevector;
typedef struct typeinfo_retaddr_set typeinfo_retaddr_set;

/* global variables ***********************************************************/

/* XXX move this documentation to global.h */
/* The following classinfo pointers are used internally by the type system.
 * Please do not use them directly, use the TYPEINFO_ macros instead.
 */

/*
 * pseudo_class_Arraystub
 *     (extends Object implements Cloneable, java.io.Serializable)
 *
 *     If two arrays of incompatible component types are merged,
 *     the resulting reference has no accessible components.
 *     The result does, however, implement the interfaces Cloneable
 *     and java.io.Serializable. This pseudo class is used internally
 *     to represent such results. (They are *not* considered arrays!)
 *
 * pseudo_class_Null
 *
 *     This pseudo class is used internally to represent the
 *     null type.
 */

/* data structures for the type system ****************************************/

/* The typeinfo structure stores detailed information on reference types.
 * (stack elements, variables, etc. with type == TYPE_ADR.)
 * XXX: exclude ReturnAddresses?
 *
 * For primitive types either there is no typeinfo allocated or the
 * typeclass pointer in the typeinfo struct is NULL.
 *
 * CAUTION: The typeinfo structure should be considered opaque outside of
 *          typeinfo.[ch]. Please use the macros and functions defined here to
 *          access typeinfo structures!
 */

/* At all times *exactly one* of the following conditions is true for
 * a particular typeinfo struct:
 *
 * A) typeclass == NULL
 *
 *        In this case the other fields of the structure
 *        are INVALID.
 *
 * B) typeclass == pseudo_class_Null
 *
 *        XXX
 *
 * C) typeclass is an array class
 *
 *        XXX
 *
 * D) typeclass == pseudo_class_Arraystub
 *
 *        XXX
 *
 * E) typeclass is an interface
 *
 *        XXX
 *
 * F) typeclass is a (non-pseudo-)class != java.lang.Object
 *
 *        XXX
 *        All classinfos in u.merged.list (if any) are
 *        subclasses of typeclass.
 *
 * G) typeclass is java.lang.Object
 *
 *        XXX
 *        In this case u.merged.count and u.merged.list
 *        are valid and may be non-zero.
 *        The classinfos in u.merged.list (if any) can be
 *        classes, interfaces or pseudo classes.
 */

/* The following algorithm is used to determine if the type described
 * by this typeinfo struct supports the interface X:
 *
 *     1) If typeclass is X or a subinterface of X the answer is "yes".
 *     2) If typeclass is a (pseudo) class implementing X the answer is "yes".
 *     3) XXX If typeclass is not an array and u.merged.count>0
 *        and all classes/interfaces in u.merged.list implement X
 *        the answer is "yes".
 *     4) If none of the above is true the answer is "no".
 */

/*
 * CAUTION: The typeinfo structure should be considered opaque outside of
 *          typeinfo.[ch]. Please use the macros and functions defined here to
 *          access typeinfo structures!
 */
struct typeinfo {
	classinfo           *typeclass;
	classinfo           *elementclass; /* valid if dimension>0 */ /* XXX various uses */
	typeinfo_mergedlist *merged;
	u1                   dimension;
	u1                   elementtype;  /* valid if dimension>0 */
};

struct typeinfo_mergedlist {
	s4         count;
	classinfo *list[1];       /* variable length!                        */
};

struct typeinfo_retaddr_set {
	typeinfo_retaddr_set *alt;  /* next alternative in set               */
	void                 *addr; /* return address                        */
};

struct typedescriptor {
	typeinfo        info;     /* valid if type == TYPE_ADR               */
	u1              type;     /* basic type (TYPE_INT, ...)              */
};

/* typevectors are used to store the types of local variables */

struct typevector {
	typevector      *alt;     /* next alternative in typevector set */
	int              k;       /* for lining up with the stack set   */
	typedescriptor   td[1];   /* variable length!                   */
};

/****************************************************************************/
/* MACROS                                                                   */
/****************************************************************************/

/* NOTE: The TYPEINFO macros take typeinfo *structs*, not pointers as
 *       arguments.  You have to dereference any pointers.
 */

/* typevectors **************************************************************/

#define TYPEVECTOR_SIZE(size)						\
    ((sizeof(typevector) - sizeof(typedescriptor))	\
     + (size)*sizeof(typedescriptor))

#define DNEW_TYPEVECTOR(size)						\
    ((typevector*)dump_alloc(TYPEVECTOR_SIZE(size)))

#define DMNEW_TYPEVECTOR(num,size)						\
    ((void*)dump_alloc((num) * TYPEVECTOR_SIZE(size)))

#define MGET_TYPEVECTOR(array,index,size) \
    ((typevector*) (((u1*)(array)) + TYPEVECTOR_SIZE(size) * (index)))

#define COPY_TYPEVECTORSET(src,dst,size)						\
    do {memcpy(dst,src,TYPEVECTOR_SIZE(size));					\
        dst->k = 0;                                             \
        if ((src)->alt) {										\
	        (dst)->alt = typevectorset_copy((src)->alt,1,size);	\
        }} while(0)

/* internally used macros ***************************************************/

/* internal, don't use this explicitly! */
#define TYPEINFO_ALLOCMERGED(mergedlist,count)                  \
    do {(mergedlist) = (typeinfo_mergedlist*)dump_alloc(        \
            sizeof(typeinfo_mergedlist)                         \
            + ((count)-1)*sizeof(classinfo*));} while(0)

/* internal, don't use this explicitly! */
#define TYPEINFO_FREEMERGED(mergedlist)

/* internal, don't use this explicitly! */
#define TYPEINFO_FREEMERGED_IF_ANY(mergedlist)

/* macros for type queries **************************************************/

#define TYPEINFO_IS_PRIMITIVE(info)                             \
            ((info).typeclass == NULL)

#define TYPEINFO_IS_REFERENCE(info)                             \
            ((info).typeclass != NULL)

#define TYPEINFO_IS_NULLTYPE(info)                              \
            ((info).typeclass == pseudo_class_Null)

#define TYPEINFO_IS_NEWOBJECT(info)                             \
            ((info).typeclass == pseudo_class_New)

/* only use this if TYPEINFO_IS_PRIMITIVE returned true! */
#define TYPEINFO_RETURNADDRESS(info)                            \
            ((void *)(info).elementclass)

/* only use this if TYPEINFO_IS_NEWOBJECT returned true! */
#define TYPEINFO_NEWOBJECT_INSTRUCTION(info)                    \
            ((void *)(info).elementclass)

/* macros for array type queries ********************************************/

#define TYPEINFO_IS_ARRAY(info)                                 \
            ( TYPEINFO_IS_REFERENCE(info)                       \
              && ((info).dimension != 0) )

#define TYPEINFO_IS_SIMPLE_ARRAY(info)                          \
            ( ((info).dimension == 1) )

#define TYPEINFO_IS_ARRAY_ARRAY(info)                           \
            ( ((info).dimension >= 2) )

#define TYPEINFO_IS_PRIMITIVE_ARRAY(info,arraytype)             \
            ( TYPEINFO_IS_SIMPLE_ARRAY(info)                    \
              && ((info).elementtype == (arraytype)) )

#define TYPEINFO_IS_OBJECT_ARRAY(info)                          \
            ( TYPEINFO_IS_SIMPLE_ARRAY(info)                    \
              && ((info).elementclass != NULL) )

/* assumes that info describes an array type */
#define TYPEINFO_IS_ARRAY_OF_REFS_NOCHECK(info)                 \
            ( ((info).elementclass != NULL)                     \
              || ((info).dimension >= 2) )

#define TYPEINFO_IS_ARRAY_OF_REFS(info)                         \
            ( TYPEINFO_IS_ARRAY(info)                           \
              && TYPEINFO_IS_ARRAY_OF_REFS_NOCHECK(info) )

#define TYPE_IS_RETURNADDRESS(type,info)                        \
            ( ((type)==TYPE_ADDRESS)                            \
              && TYPEINFO_IS_PRIMITIVE(info) )

#define TYPE_IS_REFERENCE(type,info)                            \
            ( ((type)==TYPE_ADDRESS)                            \
              && !TYPEINFO_IS_PRIMITIVE(info) )

#define TYPEDESC_IS_RETURNADDRESS(td)                           \
            TYPE_IS_RETURNADDRESS((td).type,(td).info)

#define TYPEDESC_IS_REFERENCE(td)                               \
            TYPE_IS_REFERENCE((td).type,(td).info)

/* queries allowing the null type ********************************************/

#define TYPEINFO_MAYBE_ARRAY(info)                              \
    (TYPEINFO_IS_ARRAY(info) || TYPEINFO_IS_NULLTYPE(info))

#define TYPEINFO_MAYBE_PRIMITIVE_ARRAY(info,at)                 \
    (TYPEINFO_IS_PRIMITIVE_ARRAY(info,at) || TYPEINFO_IS_NULLTYPE(info))

#define TYPEINFO_MAYBE_ARRAY_OF_REFS(info)                      \
    (TYPEINFO_IS_ARRAY_OF_REFS(info) || TYPEINFO_IS_NULLTYPE(info))

/* macros for initializing typeinfo structures ******************************/

#define TYPEINFO_INIT_PRIMITIVE(info)                           \
         do {(info).typeclass = NULL;                           \
             (info).elementclass = NULL;                        \
             (info).merged = NULL;                              \
             (info).dimension = 0;                              \
             (info).elementtype = 0;} while(0)

#define TYPEINFO_INIT_RETURNADDRESS(info,adr)                   \
         do {(info).typeclass = NULL;                           \
             (info).elementclass = (classinfo*) (adr);          \
             (info).merged = NULL;                              \
             (info).dimension = 0;                              \
             (info).elementtype = 0;} while(0)

#define TYPEINFO_INIT_NON_ARRAY_CLASSINFO(info,cinfo)   \
         do {(info).typeclass = (cinfo);                \
             (info).elementclass = NULL;                \
             (info).merged = NULL;                      \
             (info).dimension = 0;                      \
             (info).elementtype = 0;} while(0)

#define TYPEINFO_INIT_NULLTYPE(info)                            \
            TYPEINFO_INIT_CLASSINFO(info,pseudo_class_Null)

#define TYPEINFO_INIT_NEWOBJECT(info,instr)             \
         do {(info).typeclass = pseudo_class_New;       \
             (info).elementclass = (classinfo*) (instr);\
             (info).merged = NULL;                      \
             (info).dimension = 0;                      \
             (info).elementtype = 0;} while(0)

#define TYPEINFO_INIT_PRIMITIVE_ARRAY(info,arraytype)                   \
    TYPEINFO_INIT_CLASSINFO(info,primitivetype_table[arraytype].arrayclass);

#define TYPEINFO_INIT_CLASSINFO(info,cls)                               \
        do {if (((info).typeclass = (cls))->vftbl->arraydesc) {         \
                if ((cls)->vftbl->arraydesc->elementvftbl)              \
                    (info).elementclass = (cls)->vftbl->arraydesc->elementvftbl->class; \
                else                                                    \
                    (info).elementclass = NULL;                         \
                (info).dimension = (cls)->vftbl->arraydesc->dimension;  \
                (info).elementtype = (cls)->vftbl->arraydesc->elementtype; \
            }                                                           \
            else {                                                      \
                (info).elementclass = NULL;                             \
                (info).dimension = 0;                                   \
                (info).elementtype = 0;                                 \
            }                                                           \
            (info).merged = NULL;} while(0)

#define TYPEINFO_INIT_FROM_FIELDINFO(info,fi)                   \
            typeinfo_init_from_descriptor(&(info),              \
                (fi)->descriptor->text,utf_end((fi)->descriptor));

/* macros for writing types (destination must have been initialized) ********/
/* XXX delete them? */
#if 0

#define TYPEINFO_PUT_NULLTYPE(info)                             \
    do {(info).typeclass = pseudo_class_Null;} while(0)

#define TYPEINFO_PUT_NON_ARRAY_CLASSINFO(info,cinfo)            \
    do {(info).typeclass = (cinfo);} while(0)

#define TYPEINFO_PUT_CLASSINFO(info,cls)                                \
    do {if (((info).typeclass = (cls))->vftbl->arraydesc) {             \
                if ((cls)->vftbl->arraydesc->elementvftbl)                \
                    (info).elementclass = (cls)->vftbl->arraydesc->elementvftbl->class; \
                (info).dimension = (cls)->vftbl->arraydesc->dimension;    \
                (info).elementtype = (cls)->vftbl->arraydesc->elementtype; \
        }} while(0)

/* srcarray must be an array (not checked) */
#define TYPEINFO_PUT_COMPONENT(srcarray,dst)                    \
    do {typeinfo_put_component(&(srcarray),&(dst));} while(0)

#endif

/* macros for copying types (destinition is not checked or freed) ***********/

/* TYPEINFO_COPY makes a shallow copy, the merged pointer is simply copied. */
#define TYPEINFO_COPY(src,dst)                                  \
    do {(dst) = (src);} while(0)

/* TYPEINFO_CLONE makes a deep copy, the merged list (if any) is duplicated
 * into a newly allocated array.
 */
#define TYPEINFO_CLONE(src,dst)                                 \
    do {(dst) = (src);                                          \
        if ((dst).merged) typeinfo_clone(&(src),&(dst));} while(0)

/****************************************************************************/
/* FUNCTIONS                                                                */
/****************************************************************************/

/* typevector functions *****************************************************/

/* element read-only access */
bool typevectorset_checktype(typevector *set,int index,int type);
bool typevectorset_checkreference(typevector *set,int index);
bool typevectorset_checkretaddr(typevector *set,int index);
int typevectorset_copymergedtype(typevector *set,int index,typeinfo *dst);
typeinfo *typevectorset_mergedtypeinfo(typevector *set,int index,typeinfo *temp);
int typevectorset_mergedtype(typevector *set,int index,typeinfo *temp,typeinfo **result);

/* element write access */
void typevectorset_store(typevector *set,int index,int type,typeinfo *info);
void typevectorset_store_retaddr(typevector *set,int index,typeinfo *info);
void typevectorset_store_twoword(typevector *set,int index,int type);
void typevectorset_init_object(typevector *set,void *ins,classinfo *initclass,int size);

/* vector functions */
bool typevector_separable_from(typevector *a,typevector *b,int size);
bool typevector_merge(typevector *dst,typevector *y,int size);

/* vector set functions */
typevector *typevectorset_copy(typevector *src,int k,int size);
/* typevector *typevectorset_copy_select(typevector *src,
   int retindex,void *retaddr,int size);
   void typevectorset_copy_select_to(typevector *src,typevector *dst,
   int retindex,void *retaddr,int size); */
/* bool typevectorset_separable(typevector *set,int size); */
bool typevectorset_separable_with(typevector *set,typevector *add,int size);
bool typevectorset_collapse(typevector *dst,int size);
void typevectorset_add(typevector *dst,typevector *v,int size);
/* void typevectorset_union(typevector *dst,typevector *v,int size); */
typevector *typevectorset_select(typevector **set,int retindex,void *retaddr);

/* inquiry functions (read-only) ********************************************/

bool typeinfo_is_array(typeinfo *info);
bool typeinfo_is_primitive_array(typeinfo *info,int arraytype);
bool typeinfo_is_array_of_refs(typeinfo *info);

bool typeinfo_implements_interface(typeinfo *info,classinfo *interf);
bool typeinfo_is_assignable(typeinfo *value,typeinfo *dest);

/* initialization functions *************************************************/

void typeinfo_init_from_descriptor(typeinfo *info,char *utf_ptr,char *end_ptr);
void typeinfo_init_component(typeinfo *srcarray,typeinfo *dst);

int  typeinfo_count_method_args(utf *d,bool twoword); /* this not included */
void typeinfo_init_from_method_args(utf *desc,u1 *typebuf,
                                    typeinfo *infobuf,
                                    int buflen,bool twoword,
                                    int *returntype,typeinfo *returntypeinfo);
int  typedescriptors_init_from_method_args(typedescriptor *td,
										   utf *desc,
										   int buflen,bool twoword,
										   typedescriptor *returntype);

void typeinfo_clone(typeinfo *src,typeinfo *dest);

/* functions for the type system ********************************************/

void typeinfo_free(typeinfo *info);

bool typeinfo_merge(typeinfo *dest,typeinfo* y);

/* debugging helpers ********************************************************/

#ifdef TYPEINFO_DEBUG

#include <stdio.h>

void typeinfo_test();
void typeinfo_init_from_fielddescriptor(typeinfo *info,char *desc);
void typeinfo_print(FILE *file,typeinfo *info,int indent);
void typeinfo_print_short(FILE *file,typeinfo *info);
void typeinfo_print_type(FILE *file,int type,typeinfo *info);
void typeinfo_print_stacktype(FILE *file,int type,typeinfo *info);
void typedescriptor_print(FILE *file,typedescriptor *td);
void typevector_print(FILE *file,typevector *vec,int size);
void typevectorset_print(FILE *file,typevector *set,int size);

#endif /* TYPEINFO_DEBUG */

#endif /* _TYPEINFO_H */


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
