/* src/toolbox/avl.h - AVL tree implementation

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

   $Id: avl.h 4086 2006-01-03 23:46:30Z twisti $

*/


#ifndef _AVL_H
#define _AVL_H

#include "config.h"

#include "vm/types.h"

#include "vm/global.h"


/* define direction in an AVL node ********************************************/

#define AVL_LEFT     0
#define AVL_RIGHT    1


/* tree comparator prototype **************************************************/

typedef s4 avl_comparator(const void *a, const void *b);


/* forward typedefs ***********************************************************/

typedef struct avl_tree avl_tree;
typedef struct avl_node avl_node;


/* avl_tree *******************************************************************/

struct avl_tree {
	avl_node          *root;            /* pointer to root node               */
	avl_comparator    *comparator;      /* pointer to comparison function     */
	s4                 entries;         /* contains number of entries         */
#if defined(USE_THREADS)
	java_objectheader *lock;            /* threads lock object                */
#endif
};


/* avl_node *******************************************************************/

struct avl_node {
	void     *data;                     /* pointer to data structure          */
	s4        balance;                  /* the range of the field is -2...2   */
	avl_node *childs[2];                /* pointers to the child nodes        */
};


/* function prototypes ********************************************************/

avl_tree *avl_create(avl_comparator *compar);
bool avl_insert(avl_tree *tree, void *data);
void *avl_find(avl_tree *tree, void *data);

#if !defined(NDEBUG)
void avl_dump(avl_node* node, s4 indent);
#endif

#endif /* _AVL_H */


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
