/* src/native/vm/VMThread.c - java/lang/VMThread

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

   $Id: java_lang_VMThread.c 5806 2006-10-19 10:10:23Z twisti $

*/


#include "config.h"
#include "vm/types.h"

#include "native/jni.h"
#include "native/native.h"
#include "native/include/java_lang_ThreadGroup.h"
#include "native/include/java_lang_Object.h"            /* java_lang_Thread.h */
#include "native/include/java_lang_Throwable.h"         /* java_lang_Thread.h */
#include "native/include/java_lang_VMThread.h"
#include "native/include/java_lang_Thread.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#endif

#include "toolbox/logging.h"
#include "vm/exceptions.h"
#include "vm/options.h"


/*
 * Class:     java/lang/VMThread
 * Method:    countStackFrames
 * Signature: ()I
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_countStackFrames(JNIEnv *env, java_lang_VMThread *this)
{
    log_text("java_lang_VMThread_countStackFrames called");

    return 0;
}


/*
 * Class:     java/lang/VMThread
 * Method:    start
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_start(JNIEnv *env, java_lang_VMThread *this, s8 stacksize)
{
#if defined(ENABLE_THREADS)
	this->thread->vmThread = this;

	/* don't pass a function pointer (NULL) since we want Thread.run()V here */

	threads_start_thread((java_lang_Thread *) this->thread, NULL);
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    interrupt
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_interrupt(JNIEnv *env, java_lang_VMThread *this)
{
#if defined(ENABLE_THREADS)
	threads_thread_interrupt(this);
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    isInterrupted
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_isInterrupted(JNIEnv *env, java_lang_VMThread *this)
{
#if defined(ENABLE_THREADS)
	return threads_thread_has_been_interrupted(this);
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    suspend
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_suspend(JNIEnv *env, java_lang_VMThread *this)
{
#if defined(ENABLE_THREADS)
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    resume
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_resume(JNIEnv *env, java_lang_VMThread *this)
{
#if defined(ENABLE_THREADS)
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    nativeSetPriority
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_nativeSetPriority(JNIEnv *env, java_lang_VMThread *this, s4 priority)
{
#if defined(ENABLE_THREADS)
	threads_java_lang_Thread_set_priority((java_lang_Thread *) this->thread,
										  priority);
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    nativeStop
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_nativeStop(JNIEnv *env, java_lang_VMThread *this, java_lang_Throwable *t)
{
#if defined(ENABLE_THREADS)
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    currentThread
 * Signature: ()Ljava/lang/Thread;
 */
JNIEXPORT java_lang_Thread* JNICALL Java_java_lang_VMThread_currentThread(JNIEnv *env, jclass clazz)
{
	java_lang_Thread *t;

#if defined(ENABLE_THREADS)
	t = ((threadobject*) THREADOBJECT)->o.thread;

	if (t == NULL)
		log_text("t ptr is NULL\n");
  
	if (!t->group) {
		/* ThreadGroup of currentThread is not initialized */

		t->group = (java_lang_ThreadGroup *)
			native_new_and_init(class_java_lang_ThreadGroup);

		if (t->group == NULL)
			log_text("unable to create ThreadGroup");
  	}
#else
	/* we just return a fake java.lang.Thread object, otherwise we get
       NullPointerException's in GNU classpath */

	t = (java_lang_Thread *) builtin_new(class_java_lang_Thread);
#endif

	return t;
}


/*
 * Class:     java/lang/VMThread
 * Method:    yield
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMThread_yield(JNIEnv *env, jclass clazz)
{
#if defined(ENABLE_THREADS)
	threads_yield();
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    interrupted
 * Signature: ()Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_interrupted(JNIEnv *env, jclass clazz)
{
#if defined(ENABLE_THREADS)
	return threads_check_if_interrupted_and_reset();
#endif
}


/*
 * Class:     java/lang/VMThread
 * Method:    holdsLock
 * Signature: (Ljava/lang/Object;)Z
 */
JNIEXPORT s4 JNICALL Java_java_lang_VMThread_holdsLock(JNIEnv *env, jclass clazz, java_lang_Object* o)
{
#if defined(ENABLE_THREADS)
	return lock_is_held_by_current_thread((java_objectheader *) o);
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
