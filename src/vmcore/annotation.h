/* src/vmcore/annotation.h - class annotations

   Copyright (C) 2006, 2007 R. Grafl, A. Krall, C. Kruegel, C. Oates,
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


#ifndef _ANNOTATION_H
#define _ANNOTATION_H


#include "config.h"
#include "vm/types.h"

#include "vm/global.h"

#include "vmcore/class.h"
#include "vmcore/field.h"
#include "vmcore/method.h"
#include "vmcore/loader.h"
#include "vmcore/utf8.h"


/* function prototypes ********************************************************/

bool annotation_load_class_attribute_runtimevisibleannotations(
	classbuffer *cb);

bool annotation_load_class_attribute_runtimeinvisibleannotations(
	classbuffer *cb);

bool annotation_load_method_attribute_runtimevisibleannotations(
	classbuffer *cb, methodinfo *m);

bool annotation_load_method_attribute_runtimeinvisibleannotations(
	classbuffer *cb, methodinfo *m);

bool annotation_load_field_attribute_runtimevisibleannotations(
	classbuffer *cb, fieldinfo *f);

bool annotation_load_field_attribute_runtimeinvisibleannotations(
	classbuffer *cb, fieldinfo *f);

bool annotation_load_method_attribute_annotationdefault(
	classbuffer *cb, methodinfo *m);

bool annotation_load_method_attribute_runtimevisibleparameterannotations(
	classbuffer *cb, methodinfo *m);

bool annotation_load_method_attribute_runtimeinvisibleparameterannotations(
	classbuffer *cb, methodinfo *m);

#endif /* _ANNOTATION_H */


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
