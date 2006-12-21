/* src/native/vm/java_lang_Object.c - java/lang/Object functions

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
            Joseph Wenninger
            Christian Thalinger

   $Id: java_lang_VMObject.c 6213 2006-12-18 17:36:06Z twisti $

*/


#include "config.h"

#include <stdlib.h>

#include "vm/types.h"

#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_Cloneable.h"
#include "native/include/java_lang_Object.h"

#if defined(ENABLE_THREADS)
# include "threads/native/lock.h"
#endif

#include "vm/builtin.h"
#include "vm/options.h"

#if defined(ENABLE_JVMTI)
#include "native/jvmti/cacaodbg.h"
#endif


/*
 * Class:     java/lang/Object
 * Method:    getClass
 * Signature: (Ljava/lang/Object;)Ljava/lang/Class;
 */
java_lang_Class *_Jv_java_lang_Object_getClass(java_lang_Object *obj)
{
	classinfo *c;

	if (obj == NULL)
		return NULL;

	c = ((java_objectheader *) obj)->vftbl->class;

	return (java_lang_Class *) c;
}


/*
 * Class:     java/lang/Object
 * Method:    clone
 * Signature: (Ljava/lang/Cloneable;)Ljava/lang/Object;
 */
java_lang_Object *_Jv_java_lang_Object_clone(java_lang_Cloneable *this)
{
	java_objectheader *o;
	java_objectheader *co;

	o = (java_objectheader *) this;

	co = builtin_clone(NULL, o);

	return (java_lang_Object *) co;
}


/*
 * Class:     java/lang/Object
 * Method:    notify
 * Signature: ()V
 */
void _Jv_java_lang_Object_notify(java_lang_Object *this)
{
#if defined(ENABLE_THREADS)
	lock_notify_object(&this->header);
#endif
}


/*
 * Class:     java/lang/Object
 * Method:    notifyAll
 * Signature: ()V
 */
void _Jv_java_lang_Object_notifyAll(java_lang_Object *this)
{
#if defined(ENABLE_THREADS)
	lock_notify_all_object(&this->header);
#endif
}


/*
 * Class:     java/lang/Object
 * Method:    wait
 * Signature: (Ljava/lang/Object;JI)V
 */
void _Jv_java_lang_Object_wait(java_lang_Object *o, s8 ms, s4 ns)
{
#if defined(ENABLE_JVMTI)
	/* Monitor Wait */
	if (jvmti) jvmti_MonitorWaiting(true, o, ms);
#endif

#if defined(ENABLE_THREADS)
	printf("_Jv_java_lang_Object_wait: ms=%lld ns=%d\n", ms, ns);
	lock_wait_for_object(&o->header, ms, ns);
#endif

#if defined(ENABLE_JVMTI)
	/* Monitor Waited */
	/* XXX: How do you know if wait timed out ?*/
	if (jvmti) jvmti_MonitorWaiting(false, o, 0);
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
