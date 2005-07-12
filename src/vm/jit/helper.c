/* src/vm/jit/asmhelper.c - code patching helper functions

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

   $Id: helper.c 3007 2005-07-12 20:58:01Z twisti $

*/


#include <assert.h>

#include "vm/class.h"
#include "vm/exceptions.h"
#include "vm/initialize.h"
#include "vm/linker.h"
#include "vm/method.h"
#include "vm/references.h"
#include "vm/resolve.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"

/* XXX class_resolveclassmethod */
#include "vm/loader.h"


/* helper_resolve_classinfo ****************************************************

   This function returns the loaded and resolved class.

*******************************************************************************/

classinfo *helper_resolve_classinfo(constant_classref *cr)
{
	classinfo *c;

	/* resolve and load the class */

	if (!resolve_classref(NULL, cr, resolveEager, true, true, &c))
		return NULL;

	/* return the classinfo pointer */

	return c;
}


/* helper_resolve_methodinfo ***************************************************

   This function returns the loaded and resolved methodinfo of the
   passed method.

*******************************************************************************/

methodinfo *helper_resolve_methodinfo(unresolved_method *um)
{
	methodinfo *m;

	/* resolve the method */

	if (!resolve_method(um, resolveEager, &m)) {
		java_objectheader *xptr;
		java_objectheader *cause;

		/* get the cause */

		cause = *exceptionptr;

		/* convert ClassNotFoundException's to NoClassDefFoundError's */

		if (builtin_instanceof(cause, class_java_lang_ClassNotFoundException)) {
			/* clear exception, because we are calling jit code again */

			*exceptionptr = NULL;

			/* create new error */

			xptr =
				new_exception_javastring(string_java_lang_NoClassDefFoundError,
										 ((java_lang_Throwable *) cause)->detailMessage);

			/* we had an exception while creating the error */

			if (*exceptionptr)
				return NULL;

			/* set new exception */

			*exceptionptr = xptr;
		}

		return NULL;
	}

	/* return the methodinfo pointer */

	return m;
}


/* helper_resolve_fieldinfo ****************************************************

   This function returns the fieldinfo pointer of the passed field.

*******************************************************************************/

void *helper_resolve_fieldinfo(unresolved_field *uf)
{
	fieldinfo *fi;

	/* resolve the field */

	if (!resolve_field(uf, resolveEager, &fi)) {
		java_objectheader *xptr;
		java_objectheader *cause;

		/* get the cause */

		cause = *exceptionptr;

		/* convert ClassNotFoundException's to NoClassDefFoundError's */

		if (builtin_instanceof(cause, class_java_lang_ClassNotFoundException)) {
			/* clear exception, because we are calling jit code again */

			*exceptionptr = NULL;

			/* create new error */

			xptr =
				new_exception_javastring(string_java_lang_NoClassDefFoundError,
										 ((java_lang_Throwable *) cause)->detailMessage);

			/* we had an exception while creating the error */

			if (*exceptionptr)
				return NULL;

			/* set new exception */

			*exceptionptr = xptr;
		}

		return NULL;
	}

	/* return the fieldinfo pointer */

	return fi;
}


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
