/* src/native/vm/VMSystem.c - java/lang/VMSystem

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
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

   Contact: cacao@cacaojvm.org

   Authors: Roman Obermaiser

   Changes: Joseph Wenninger
            Christian Thalinger

   $Id: java_lang_VMSystem.c 5153 2006-07-18 08:19:24Z twisti $

*/


#include <string.h>

#include "config.h"
#include "vm/types.h"

#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Object.h"
#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/stringlocal.h"


/*
 * Class:     java/lang/VMSystem
 * Method:    arraycopy
 * Signature: (Ljava/lang/Object;ILjava/lang/Object;II)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMSystem_arraycopy(JNIEnv *env, jclass clazz, java_lang_Object *source, s4 sp, java_lang_Object *dest, s4 dp, s4 len)
{
	java_arrayheader *s;
	java_arrayheader *d;
	arraydescriptor  *sdesc;
	arraydescriptor  *ddesc;
	s4                i;

	s = (java_arrayheader *) source;
	d = (java_arrayheader *) dest;

	if (!s || !d) { 
		exceptions_throw_nullpointerexception();
		return; 
	}

	sdesc = s->objheader.vftbl->arraydesc;
	ddesc = d->objheader.vftbl->arraydesc;

	if (!sdesc || !ddesc || (sdesc->arraytype != ddesc->arraytype)) {
		*exceptionptr = new_arraystoreexception();
		return; 
	}

	/* we try to throw exception with the same message as SUN does */

	if ((len < 0) || (sp < 0) || (dp < 0) ||
		(sp + len < 0) || (sp + len > s->size) ||
		(dp + len < 0) || (dp + len > d->size)) {
		exceptions_throw_arrayindexoutofboundsexception();
		return; 
	}

	if (sdesc->componentvftbl == ddesc->componentvftbl) {
		/* We copy primitive values or references of exactly the same type */

		s4 dataoffset = sdesc->dataoffset;
		s4 componentsize = sdesc->componentsize;

		memmove(((u1 *) d) + dataoffset + componentsize * dp,
				((u1 *) s) + dataoffset + componentsize * sp,
				(size_t) len * componentsize);

	} else {
		/* We copy references of different type */

		java_objectarray *oas = (java_objectarray *) s;
		java_objectarray *oad = (java_objectarray *) d;
                
		if (dp <= sp) {
			for (i = 0; i < len; i++) {
				java_objectheader *o = oas->data[sp + i];
				if (!builtin_canstore(oad, o)) {
					*exceptionptr = new_arraystoreexception();
					return;
				}
				oad->data[dp + i] = o;
			}

		} else {
			/* XXX this does not completely obey the specification!
			   If an exception is thrown only the elements above the
			   current index have been copied. The specification
			   requires that only the elements *below* the current
			   index have been copied before the throw. */

			for (i = len - 1; i >= 0; i--) {
				java_objectheader *o = oas->data[sp + i];

				if (!builtin_canstore(oad, o)) {
					*exceptionptr = new_arraystoreexception();
					return;
				}

				oad->data[dp + i] = o;
			}
		}
	}
}


/*
 * Class:     java/lang/VMSystem
 * Method:    identityHashCode
 * Signature: (Ljava/lang/Object;)I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMSystem_identityHashCode(JNIEnv *env, jclass clazz, java_lang_Object *o)
{
	return (s4) ((ptrint) o);
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