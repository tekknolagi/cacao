/* src/vm/exceptions.hpp - exception related functions prototypes

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


#ifndef _EXCEPTIONS_HPP
#define _EXCEPTIONS_HPP

#include "config.h"
#include "vm/types.h"

#include "vm/global.h"
#include "vm/references.h"
#include "vm/method.hpp"


/* function prototypes ********************************************************/

#ifdef __cplusplus
extern "C" {
#endif

java_handle_t *exceptions_get_exception(void);
void           exceptions_set_exception(java_handle_t *o);
void           exceptions_clear_exception(void);
java_handle_t *exceptions_get_and_clear_exception(void);


/* functions to generate compiler exceptions */

java_handle_t *exceptions_new_abstractmethoderror(void);
java_object_t *exceptions_asm_new_abstractmethoderror(u1 *sp, u1 *ra);
java_handle_t *exceptions_new_arraystoreexception(void);

void exceptions_throw_abstractmethoderror(void);
void exceptions_throw_classcircularityerror(classinfo *c);
void exceptions_throw_classformaterror(classinfo *c, const char *message, ...);
void exceptions_throw_classnotfoundexception(utf *name);
void exceptions_throw_noclassdeffounderror(utf *name);
void exceptions_throw_noclassdeffounderror_cause(java_handle_t *cause);
void exceptions_throw_noclassdeffounderror_wrong_name(classinfo *c, utf *name);
void exceptions_throw_linkageerror(const char *message, classinfo *c);
void exceptions_throw_nosuchfielderror(classinfo *c, utf *name);
void exceptions_throw_nosuchmethoderror(classinfo *c, utf *name, utf *desc);
void exceptions_throw_exceptionininitializererror(java_handle_t *cause);
void exceptions_throw_incompatibleclasschangeerror(classinfo *c,
												   const char *message);
void exceptions_throw_instantiationerror(classinfo *c);
void exceptions_throw_internalerror(const char *message, ...);
void exceptions_throw_outofmemoryerror(void);
void exceptions_throw_verifyerror(methodinfo *m, const char *message, ...);
void exceptions_throw_verifyerror_for_stack(methodinfo *m, int type);
void exceptions_throw_unsatisfiedlinkerror(utf *name);
void exceptions_throw_unsupportedclassversionerror(classinfo *c, u4 ma, u4 mi);

java_handle_t *exceptions_new_arithmeticexception(void);

java_handle_t *exceptions_new_arrayindexoutofboundsexception(s4 index);
void exceptions_throw_arrayindexoutofboundsexception(void);
void exceptions_throw_arraystoreexception(void);

java_handle_t *exceptions_new_classcastexception(java_handle_t *o);

void exceptions_throw_clonenotsupportedexception(void);
void exceptions_throw_illegalaccessexception(utf *message);
void exceptions_throw_illegalargumentexception(void);
void exceptions_throw_illegalmonitorstateexception(void);
void exceptions_throw_interruptedexception(void);
void exceptions_throw_instantiationexception(classinfo *c);
void exceptions_throw_invocationtargetexception(java_handle_t *cause);
void exceptions_throw_negativearraysizeexception(void);

java_handle_t *exceptions_new_nullpointerexception(void);
void exceptions_throw_nullpointerexception(void);
void exceptions_throw_privilegedactionexception(java_handle_t *cause);
void exceptions_throw_stringindexoutofboundsexception(void);

java_handle_t *exceptions_fillinstacktrace(void);

void exceptions_print_exception(java_handle_t *xptr);
void exceptions_print_current_exception(void);
void exceptions_print_stacktrace(void);

#ifdef __cplusplus
}
#endif

#endif // _EXCEPTIONS_HPP


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