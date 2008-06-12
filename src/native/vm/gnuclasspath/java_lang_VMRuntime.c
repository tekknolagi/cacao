/* src/native/vm/gnu/java_lang_VMRuntime.c

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


#include "config.h"

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>

#if defined(__DARWIN__)
# if defined(__POWERPC__)
#  define OS_INLINE    /* required for <libkern/ppc/OSByteOrder.h> */
# endif
# include <mach/mach.h>
#endif

#include "mm/dumpmemory.h"
#include "mm/gc-common.h"

#include "native/jni.h"
#include "native/native.h"

#include "native/include/java_io_File.h"
#include "native/include/java_lang_ClassLoader.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_Process.h"

#include "native/include/java_lang_VMRuntime.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"

#include "vmcore/system.h"
#include "vmcore/utf8.h"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ "exit",                   "(I)V",                                         (void *) (intptr_t) &Java_java_lang_VMRuntime_exit                   },
	{ "freeMemory",             "()J",                                          (void *) (intptr_t) &Java_java_lang_VMRuntime_freeMemory             },
	{ "totalMemory",            "()J",                                          (void *) (intptr_t) &Java_java_lang_VMRuntime_totalMemory            },
	{ "maxMemory",              "()J",                                          (void *) (intptr_t) &Java_java_lang_VMRuntime_maxMemory              },
	{ "gc",                     "()V",                                          (void *) (intptr_t) &Java_java_lang_VMRuntime_gc                     },
	{ "runFinalization",        "()V",                                          (void *) (intptr_t) &Java_java_lang_VMRuntime_runFinalization        },
	{ "runFinalizersOnExit",    "(Z)V",                                         (void *) (intptr_t) &Java_java_lang_VMRuntime_runFinalizersOnExit    },
	{ "runFinalizationForExit", "()V",                                          (void *) (intptr_t) &Java_java_lang_VMRuntime_runFinalizationForExit },
	{ "traceInstructions",      "(Z)V",                                         (void *) (intptr_t) &Java_java_lang_VMRuntime_traceInstructions      },
	{ "traceMethodCalls",       "(Z)V",                                         (void *) (intptr_t) &Java_java_lang_VMRuntime_traceMethodCalls       },
	{ "availableProcessors",    "()I",                                          (void *) (intptr_t) &Java_java_lang_VMRuntime_availableProcessors    },
	{ "nativeLoad",             "(Ljava/lang/String;Ljava/lang/ClassLoader;)I", (void *) (intptr_t) &Java_java_lang_VMRuntime_nativeLoad             },
	{ "mapLibraryName",         "(Ljava/lang/String;)Ljava/lang/String;",       (void *) (intptr_t) &Java_java_lang_VMRuntime_mapLibraryName         },
};


/* _Jv_java_lang_VMRuntime_init ************************************************

   Register native functions.

*******************************************************************************/

void _Jv_java_lang_VMRuntime_init(void)
{
	utf *u;

	u = utf_new_char("java/lang/VMRuntime");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


static bool finalizeOnExit = false;


/*
 * Class:     java/lang/VMRuntime
 * Method:    exit
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_exit(JNIEnv *env, jclass clazz, int32_t status)
{
	if (finalizeOnExit)
		gc_finalize_all();

	vm_shutdown(status);
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    freeMemory
 * Signature: ()J
 */
JNIEXPORT int64_t JNICALL Java_java_lang_VMRuntime_freeMemory(JNIEnv *env, jclass clazz)
{
	return gc_get_free_bytes();
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    totalMemory
 * Signature: ()J
 */
JNIEXPORT int64_t JNICALL Java_java_lang_VMRuntime_totalMemory(JNIEnv *env, jclass clazz)
{
	return gc_get_heap_size();
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    maxMemory
 * Signature: ()J
 */
JNIEXPORT int64_t JNICALL Java_java_lang_VMRuntime_maxMemory(JNIEnv *env, jclass clazz)
{
	return gc_get_max_heap_size();
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    gc
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_gc(JNIEnv *env, jclass clazz)
{
	gc_call();
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    runFinalization
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalization(JNIEnv *env, jclass clazz)
{
	gc_invoke_finalizers();
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    runFinalizersOnExit
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalizersOnExit(JNIEnv *env, jclass clazz, int32_t value)
{
	/* XXX threading */

	finalizeOnExit = value;
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    runFinalizationsForExit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_runFinalizationForExit(JNIEnv *env, jclass clazz)
{
/*  	if (finalizeOnExit) { */
/*  		gc_call(); */
	/* gc_finalize_all(); */
/*  	} */
/*  	log_text("Java_java_lang_VMRuntime_runFinalizationForExit called"); */
	/*gc_finalize_all();*/
	/*gc_invoke_finalizers();*/
	/*gc_call();*/
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    traceInstructions
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_traceInstructions(JNIEnv *env, jclass clazz, int32_t par1)
{
	/* not supported */
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    traceMethodCalls
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMRuntime_traceMethodCalls(JNIEnv *env, jclass clazz, int32_t par1)
{
	/* not supported */
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    availableProcessors
 * Signature: ()I
 */
JNIEXPORT int32_t JNICALL Java_java_lang_VMRuntime_availableProcessors(JNIEnv *env, jclass clazz)
{
	return system_processors_online();
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    nativeLoad
 * Signature: (Ljava/lang/String;Ljava/lang/ClassLoader;)I
 */
JNIEXPORT int32_t JNICALL Java_java_lang_VMRuntime_nativeLoad(JNIEnv *env, jclass clazz, java_lang_String *libname, java_lang_ClassLoader *loader)
{
	classloader_t *cl;
	utf           *name;

	cl = loader_hashtable_classloader_add((java_handle_t *) loader);

	/* REMOVEME When we use Java-strings internally. */

	if (libname == NULL) {
		exceptions_throw_nullpointerexception();
		return 0;
	}

	name = javastring_toutf((java_handle_t *) libname, false);

	return native_library_load(env, name, cl);
}


/*
 * Class:     java/lang/VMRuntime
 * Method:    mapLibraryName
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT java_lang_String* JNICALL Java_java_lang_VMRuntime_mapLibraryName(JNIEnv *env, jclass clazz, java_lang_String *libname)
{
	utf           *u;
	char          *buffer;
	int32_t        buffer_len;
	java_handle_t *o;
	int32_t        dumpmarker;

	if (libname == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	u = javastring_toutf((java_handle_t *) libname, false);

	/* calculate length of library name */

	buffer_len =
		strlen(NATIVE_LIBRARY_PREFIX) +
		utf_bytes(u) +
		strlen(NATIVE_LIBRARY_SUFFIX) +
		strlen("0");

	DMARKER;

	buffer = DMNEW(char, buffer_len);

	/* generate library name */

	strcpy(buffer, NATIVE_LIBRARY_PREFIX);
	utf_cat(buffer, u);
	strcat(buffer, NATIVE_LIBRARY_SUFFIX);

	o = javastring_new_from_utf_string(buffer);

	/* release memory */

	DRELEASE;

	return (java_lang_String *) o;
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
