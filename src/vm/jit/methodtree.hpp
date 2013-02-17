/* src/vm/jit/methodtree.hpp - AVL tree of methods

   Copyright (C) 2008
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


#ifndef METHODTREE_HPP_
#define METHODTREE_HPP_ 1

#include "config.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* function prototypes ********************************************************/

void  methodtree_init(void);
void  methodtree_insert(void *startpc, void *endpc);
void *methodtree_find(void *pc);
void *methodtree_find_nocheck(void *pc);

#ifdef __cplusplus
}
#endif

#endif // METHODTREE_HPP_


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
 * vim:noexpandtab:sw=4:ts=4:
 */
