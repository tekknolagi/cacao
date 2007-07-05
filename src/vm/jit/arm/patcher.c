/* src/vm/jit/arm/patcher.c - ARM code patching functions

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

   $Id: patcher.c 8160 2007-06-28 01:52:19Z michi $

*/


#include "config.h"

#include <assert.h>

#include "vm/types.h"

#include "mm/memory.h"

#include "native/native.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/initialize.h"

#include "vm/jit/asmpart.h"
#include "vm/jit/md.h"
#include "vm/jit/patcher-common.h"

#include "vmcore/field.h"
#include "vmcore/options.h"
#include "vmcore/references.h"
#include "vm/resolve.h"


#define PATCH_BACK_ORIGINAL_MCODE \
    *((u4 *) pr->mpc) = (u4) pr->mcode; \
    md_icacheflush((u1 *) pr->mpc, 1 * 4);

#define gen_resolveload(inst,offset) \
	assert((offset) >= -0x0fff && (offset) <= 0x0fff); \
	assert(!((inst) & 0x0fff)); \
	if ((offset) <  0) { \
		(inst) = ((inst) & 0xff7ff000) | ((-(offset)) & 0x0fff); \
		/*(inst) &= ~(1 << 23);*/ \
	} else { \
		(inst) = ((inst) & 0xfffff000) | ((offset) & 0x0fff); \
		/*(inst) |= (1 << 23);*/ \
	}


/* patcher_get_putstatic *******************************************************

   Machine code:

   <patched call position>
   e51c103c    ldr   r1, [ip, #-60]

*******************************************************************************/

bool patcher_get_putstatic(patchref_t *pr)
{
	unresolved_field *uf;
	u1               *datap;
	fieldinfo        *fi;

	/* get stuff from the stack */

	uf    = (unresolved_field *) pr->ref;
	datap = (u1 *)               pr->datap;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf)))
		return false;

	/* check if the field's class is initialized */

	if (!(fi->class->state & CLASS_INITIALIZED))
		if (!initialize_class(fi->class))
			return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch the field value's address */

	*((ptrint *) datap) = (ptrint) &(fi->value);

	return true;
}


/* patcher_get_putfield ********************************************************

   Machine code:

   <patched call position>
   e58a8000    str   r8, [sl, #__]

*******************************************************************************/

bool patcher_get_putfield(patchref_t *pr)
{
	u1                *ra;
	u4                 mcode;
	unresolved_field  *uf;
	fieldinfo         *fi;

	/* get stuff from the stack */
	ra    = (u1*)                 pr->mpc;
	mcode =                       pr->mcode;
	uf    = (unresolved_field*)   pr->ref;

	/* get the fieldinfo */

	if (!(fi = resolve_field_eager(uf)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* if we show disassembly, we have to skip the nop */

	if (opt_showdisassemble)
		ra = ra + 1 * 4;

	/* patch the field's offset into the instruction */

	switch(fi->type) {
	case TYPE_ADR:
	case TYPE_INT:
#if defined(ENABLE_SOFTFLOAT)
	case TYPE_FLT:
#endif
		assert(fi->offset <= 0x0fff);
		*((u4 *) (ra + 0 * 4)) |= (fi->offset & 0x0fff);
		break;

	case TYPE_LNG:
#if defined(ENABLE_SOFTFLOAT)
	case TYPE_DBL:
#endif
		assert((fi->offset + 4) <= 0x0fff);
		*((u4 *) (ra + 0 * 4)) |= ((fi->offset + 0) & 0x0fff);
		*((u4 *) (ra + 1 * 4)) &= 0xfffff000;
		*((u4 *) (ra + 1 * 4)) |= ((fi->offset + 4) & 0x0fff);
		break;

#if !defined(ENABLE_SOFTFLOAT)
	case TYPE_FLT:
	case TYPE_DBL:
		assert(fi->offset <= 0x03ff);
		*((u4 *) (ra + 0 * 4)) |= ((fi->offset >> 2) & 0x00ff);
		break;
#endif
	}

	/* synchronize instruction cache */

	md_icacheflush(ra, 2 * 4);

	return true;
}


/* patcher_resolve_classref_to_classinfo ***************************************

   ACONST - Machine code:

   <patched call postition>
   e51cc030    ldr   r0, [ip, #-48]

   MULTIANEWARRAY - Machine code:
    
   <patched call position>
   e3a00002    mov   r0, #2  ; 0x2
   e51c1064    ldr   r1, [ip, #-100]
   e1a0200d    mov   r2, sp
   e1a0e00f    mov   lr, pc
   e51cf068    ldr   pc, [ip, #-104]

   ARRAYCHECKCAST - Machine code:

   <patched call position>
   e51c1120    ldr   r1, [ip, #-288]
   e1a0e00f    mov   lr, pc
   e51cf124    ldr   pc, [ip, #-292]

*******************************************************************************/

bool patcher_resolve_classref_to_classinfo(patchref_t *pr)
{
	constant_classref *cr;
	u1                *datap;
	classinfo         *c;

	/* get stuff from the stack */

	cr    = (constant_classref *) pr->ref;
	datap = (u1 *)                pr->datap;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch the classinfo pointer */

	*((ptrint *) datap) = (ptrint) c;

	return true;
}


/* patcher_invokestatic_special ************************************************

   Machine code:

   <patched call position>
   e51cc02c    ldr   ip, [ip, #-44]
   e1a0e00f    mov   lr, pc
   e1a0f00c    mov   pc, ip

******************************************************************************/

bool patcher_invokestatic_special(patchref_t *pr)
{
	unresolved_method *um;
	u1                *datap;
	methodinfo        *m;

	/* get stuff from the stack */

	um    = (unresolved_method*) pr->ref;
	datap = (u1 *)               pr->datap;

	/* get the methodinfo */

	if (!(m = resolve_method_eager(um)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch stubroutine */

	*((ptrint *) datap) = (ptrint) m->stubroutine;

	return true;
}


/* patcher_invokevirtual *******************************************************

   Machine code:

   <patched call position>
   e590b000    ldr   fp, [r0]
   e59bc000    ldr   ip, [fp, #__]
   e1a0e00f    mov   lr, pc
   e1a0f00c    mov   pc, ip

*******************************************************************************/

bool patcher_invokevirtual(patchref_t *pr)
{
	u1                *ra;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra = (u1 *)                pr->mpc;
	um = (unresolved_method *) pr->ref;

	/* get the methodinfo */

	if (!(m = resolve_method_eager(um)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* if we show disassembly, we have to skip the nop */

	if (opt_showdisassemble)
		ra = ra + 1 * 4;

	/* patch vftbl index */

	gen_resolveload(*((s4 *) (ra + 1 * 4)), (s4) (OFFSET(vftbl_t, table[0]) + sizeof(methodptr) * m->vftblindex));

	/* synchronize instruction cache */

	md_icacheflush(ra + 1 * 4, 1 * 4);

	return true;
}


/* patcher_invokeinterface *****************************************************

   Machine code:

   <patched call position>
   e590b000    ldr   fp, [r0]
   e59bb000    ldr   fp, [fp, #__]
   e59bc000    ldr   ip, [fp, #__]
   e1a0e00f    mov   lr, pc
   e1a0f00c    mov   pc, ip


*******************************************************************************/

bool patcher_invokeinterface(patchref_t *pr)
{
	u1                *ra;
	unresolved_method *um;
	methodinfo        *m;

	/* get stuff from the stack */

	ra = (u1 *)                pr->mpc;
	um = (unresolved_method *) pr->ref;

	/* get the methodinfo */

	if (!(m = resolve_method_eager(um)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* if we show disassembly, we have to skip the nop */

	if (opt_showdisassemble)
		ra = ra + 1 * 4;

	/* patch interfacetable index */

	gen_resolveload(*((s4 *) (ra + 1 * 4)), (s4) (OFFSET(vftbl_t, interfacetable[0]) - sizeof(methodptr*) * m->class->index));

	/* patch method offset */

	gen_resolveload(*((s4 *) (ra + 2 * 4)), (s4) (sizeof(methodptr) * (m - m->class->methods)));

	/* synchronize instruction cache */

	md_icacheflush(ra + 1 * 4, 2 * 4);

	return true;
}


/* patcher_checkcast_instanceof_flags ******************************************

   Machine code:

   <patched call position>

*******************************************************************************/

bool patcher_resolve_classref_to_flags(patchref_t *pr)
{
	constant_classref *cr;
	u1                *datap;
	classinfo         *c;

	/* get stuff from the stack */

	cr    = (constant_classref *) pr->ref;
	datap = (u1 *)                pr->datap;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch class flags */

	*((s4 *) datap) = (s4) c->flags;

	return true;
}


/* patcher_resolve_classref_to_index *******************************************

   Machine code:

   <patched call position>

*******************************************************************************/

bool patcher_resolve_classref_to_index(patchref_t *pr)
{
	constant_classref *cr;
	u1                *datap;
	classinfo         *c;

	/* get stuff from the stack */

	cr    = (constant_classref *) pr->ref;
	datap = (u1 *)                pr->datap;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch super class index */

	*((s4 *) datap) = (s4) c->index;

	return true;
}


/* patcher_resolve_classref_to_vftbl *******************************************

   Machine code:

   <patched call position>

*******************************************************************************/

bool patcher_resolve_classref_to_vftbl(patchref_t *pr)
{
	constant_classref *cr;
	u1                *datap;
	classinfo         *c;

	/* get stuff from the stack */

	cr    = (constant_classref *) pr->ref;
	datap = (u1 *)                pr->datap;

	/* get the classinfo */

	if (!(c = resolve_classref_eager(cr)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch super class' vftbl */

	*((ptrint *) datap) = (ptrint) c->vftbl;

	return true;
}


/* patcher_initialize_class ****************************************************

   XXX

*******************************************************************************/

bool patcher_initialize_class(patchref_t *pr)
{
	classinfo *c;

	/* get stuff from the stack */

	c = (classinfo *) pr->ref;

	/* check if the class is initialized */

	if (!(c->state & CLASS_INITIALIZED))
		if (!initialize_class(c))
			return false;

	PATCH_BACK_ORIGINAL_MCODE;

	return true;
}


/* patcher_resolve_class *******************************************************

   Machine code:

   <patched call position>

*******************************************************************************/

#ifdef ENABLE_VERIFIER
bool patcher_resolve_class(patchref_t *pr)
{
	unresolved_class *uc;

	/* get stuff from the stack */

	uc = (unresolved_class *) pr->ref;

	/* resolve the class and check subtype constraints */

	if (!resolve_class_eager_no_access_check(uc))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	return true;
}
#endif /* ENABLE_VERIFIER */


/* patcher_resolve_native_function *********************************************

   XXX

*******************************************************************************/

#if !defined(WITH_STATIC_CLASSPATH)
bool patcher_resolve_native_function(patchref_t *pr)
{
	methodinfo  *m;
	u1          *datap;
	functionptr  f;

	/* get stuff from the stack */

	m     = (methodinfo *) pr->ref;
	datap = (u1 *)         pr->datap;

	/* resolve native function */

	if (!(f = native_resolve_function(m)))
		return false;

	PATCH_BACK_ORIGINAL_MCODE;

	/* patch native function pointer */

	*((ptrint *) datap) = (ptrint) f;

	return true;
}
#endif /* !defined(WITH_STATIC_CLASSPATH) */


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
