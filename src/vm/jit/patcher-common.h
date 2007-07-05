/* src/vm/jit/patcher-common.h - architecture independent code patching stuff

   Copyright (C) 2007 R. Grafl, A. Krall, C. Kruegel,
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

   $Id$

*/


#ifndef _PATCHER_COMMON_H
#define _PATCHER_COMMON_H

/* forward typedefs ***********************************************************/

#include "config.h"
#include "vm/types.h"

#include "toolbox/list.h"

#include "vm/global.h"

#include "vm/jit/jit.h"


/* patchref_t ******************************************************************

   A patcher reference contains information about a code position
   which needs additional code patching during runtime.

*******************************************************************************/

typedef struct patchref_t {
	ptrint       mpc;           /* absolute position in code segment          */
	ptrint       datap;         /* absolute position in data segment          */
	s4           disp;          /* displacement of ref in the data segment    */
	functionptr  patcher;       /* patcher function to call                   */
	voidptr      ref;           /* reference passed                           */
	u8           mcode;         /* machine code to be patched back in         */
	bool         done;          /* XXX preliminary: patch already applied?    */
	listnode_t   linkage;
} patchref_t;


/* macros *********************************************************************/


/* function prototypes ********************************************************/

void patcher_list_create(codeinfo *code);
void patcher_list_free(codeinfo *code);

void patcher_add_patch_ref(jitdata *jd, functionptr patcher, voidptr ref,
                           s4 disp);

java_objectheader *patcher_handler(u1 *pc);


/* patcher prototypes and macros **********************************************/

#if defined(__ALPHA__) || defined(__ARM__)

/* new patcher functions */

bool patcher_resolve_class(patchref_t *pr);
#define PATCHER_resolve_class (functionptr) patcher_resolve_class

bool patcher_initialize_class(patchref_t *pr);
#define PATCHER_initialize_class (functionptr) patcher_initialize_class

bool patcher_resolve_classref_to_classinfo(patchref_t *pr);
#define PATCHER_resolve_classref_to_classinfo (functionptr) patcher_resolve_classref_to_classinfo

bool patcher_resolve_classref_to_vftbl(patchref_t *pr);
#define PATCHER_resolve_classref_to_vftbl (functionptr) patcher_resolve_classref_to_vftbl

bool patcher_resolve_classref_to_index(patchref_t *pr);
#define PATCHER_resolve_classref_to_index (functionptr) patcher_resolve_classref_to_index

bool patcher_resolve_classref_to_flags(patchref_t *pr);
#define PATCHER_resolve_classref_to_flags (functionptr) patcher_resolve_classref_to_flags

#if !defined(WITH_STATIC_CLASSPATH)
bool patcher_resolve_native_function(patchref_t *pr);
#define PATCHER_resolve_native_function (functionptr) patcher_resolve_native_function
#endif

/* old patcher functions */

bool patcher_get_putstatic(patchref_t *pr);
#define PATCHER_get_putstatic (functionptr) patcher_get_putstatic

#if defined(__I386__)

bool patcher_getfield(patchref_t *pr);
#define PATCHER_getfield (functionptr) patcher_getfield

bool patcher_putfield(patchref_t *pr);
#define PATCHER_putfield (functionptr) patcher_putfield

#else

bool patcher_get_putfield(patchref_t *pr);
#define PATCHER_get_putfield (functionptr) patcher_get_putfield

#endif /* defined(__I386__) */

#if defined(__I386__) || defined(__X86_64__)

bool patcher_putfieldconst(patchref_t *pr);
#define PATCHER_putfieldconst (functionptr) patcher_putfieldconst

#endif /* defined(__I386__) || defined(__X86_64__) */

bool patcher_invokestatic_special(patchref_t *pr);
#define PATCHER_invokestatic_special (functionptr) patcher_invokestatic_special

bool patcher_invokevirtual(patchref_t *pr);
#define PATCHER_invokevirtual (functionptr) patcher_invokevirtual

bool patcher_invokeinterface(patchref_t *pr);
#define PATCHER_invokeinterface (functionptr) patcher_invokeinterface

#if defined(__ALPHA__)

bool patcher_checkcast_interface(patchref_t *pr);
#define PATCHER_checkcast_interface (functionptr) patcher_checkcast_interface

bool patcher_instanceof_interface(patchref_t *pr);
#define PATCHER_instanceof_interface (functionptr) patcher_instanceof_interface

#endif /* defined(__ALPHA__) */



#endif /* architecture list */


#endif /* _PATCHER_COMMON_H */


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
