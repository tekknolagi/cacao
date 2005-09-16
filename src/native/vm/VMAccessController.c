/* src/native/vm/VMAccessController.c - java/security/VMAccessController

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

   Authors: Joseph Wenninger

   Changes: Christian Thalinger

   $Id: VMAccessController.c 3189 2005-09-16 11:53:40Z twisti $

*/


#include "native/jni.h"
#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/jit/stacktrace.h"


/*
 * Class:     java/security/VMAccessController
 * Method:    getStack
 * Signature: ()[[Ljava/lang/Object;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_security_VMAccessController_getStack(JNIEnv *env, jclass clazz) {
#if defined(__ALPHA__) || defined(__ARM__) || defined(__I386__) || defined(__MIPS__) || defined(__POWERPC__) || defined (__X86_64__)
	/* these JITs support stacktraces */

	return cacao_getStackForVMAccessController();

#else
# if defined(ENABLE_INTRP)
	/* the interpreter supports stacktraces, even if the JIT does not */

	if (opt_intrp) {
		return cacao_getStackForVMAccessController();

	} else
# endif
		{
			java_objectarray *result;
			java_objectarray *classes;
			java_objectarray *methodnames;

			if (!(result = builtin_anewarray(2, arrayclass_java_lang_Object)))
				return NULL;

			if (!(classes = builtin_anewarray(0, class_java_lang_Class)))
				return NULL;

			if (!(methodnames = builtin_anewarray(0, class_java_lang_String)))
				return NULL;

			result->data[0] = (java_objectheader *) classes;
			result->data[1] = (java_objectheader *) methodnames;

			return result;
		}
#endif
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
 */
