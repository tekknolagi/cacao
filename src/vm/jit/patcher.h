/* src/vm/jit/patcher.h - code patching functions

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

   Authors: Christian Thalinger

   Changes:

   $Id: patcher.h 2320 2005-04-21 20:19:06Z twisti $

*/

#ifndef _PATCHER_H
#define _PATCHER_H

#include "types.h"
#include "vm/global.h"


/* function prototypes ********************************************************/

bool patcher_get_putstatic(u1 *sp);
#define PATCHER_get_putstatic (functionptr) patcher_get_putstatic

#if defined(__I386__)

bool patcher_getfield(u1 *sp);
#define PATCHER_getfield (functionptr) patcher_getfield

bool patcher_putfield(u1 *sp);
#define PATCHER_putfield (functionptr) patcher_putfield

#else

bool patcher_get_putfield(u1 *sp);
#define PATCHER_get_putfield (functionptr) patcher_get_putfield

#endif /* defined(__I386__) */

bool patcher_builtin_new(u1 *sp);

bool patcher_invokestatic_special(u1 *sp);
#define PATCHER_invokestatic_special (functionptr) patcher_invokestatic_special

bool patcher_invokevirtual(u1 *sp);
#define PATCHER_invokevirtual (functionptr) patcher_invokevirtual

bool patcher_invokeinterface(u1 *sp);
#define PATCHER_invokeinterface (functionptr) patcher_invokeinterface

bool patcher_clinit(u1 *sp);
#define PATCHER_clinit (functionptr) patcher_clinit

#endif /* _PATCHER_H */


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

