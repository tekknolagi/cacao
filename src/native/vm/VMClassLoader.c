/* nat/VMClassLoader.c - java/lang/ClassLoader

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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

   Authors: Roman Obermaiser

   Changes: Joseph Wenninger

   $Id: VMClassLoader.c 1344 2004-07-21 17:12:53Z twisti $

*/


#include "exceptions.h"
#include "jni.h"
#include "loader.h"
#include "native.h"
#include "builtin.h"
#include "tables.h"
#include "toolbox/logging.h"
#include "nat/java_lang_Class.h"
#include "nat/java_lang_String.h"
#include "nat/java_lang_ClassLoader.h"
#include "nat/java_net_URL.h"


/*
 * Class:     java/lang/ClassLoader
 * Method:    defineClass
 * Signature: (Ljava/lang/String;[BII)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClassLoader_defineClass(JNIEnv *env, jclass clazz, java_lang_ClassLoader *this, java_lang_String *name, java_bytearray *buf, s4 off, s4 len)
{
	classinfo *c;

	log_text("Java_java_lang_VMClassLoader_defineClass called");

	/* call JNI-function to load the class */
	c = (*env)->DefineClass(env,
							javastring_tochar((java_objectheader*) name),
							(jobject) this,
							(const jbyte *) &buf[off],
							len);

	use_class_as_object(c);

	return (java_lang_Class *) c;
}


/*
 * Class:     java/lang/Class
 * Method:    getPrimitiveClass
 * Signature: (Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClassLoader_getPrimitiveClass(JNIEnv *env, jclass clazz, java_lang_String *name)
{
	classinfo *c;
	utf *u = javastring_toutf(name, false);

	if (u) {
		/* get primitive class */
		c = class_new(u);
		class_load(c);
		class_init(c);
		use_class_as_object(c);

		return (java_lang_Class *) c;
	}

	/* illegal primitive classname specified */
	*exceptionptr = new_exception(string_java_lang_ClassNotFoundException);

	return NULL;
}


/*
 * Class:     java/lang/ClassLoader
 * Method:    resolveClass
 * Signature: (Ljava/lang/Class;)V
 */
JNIEXPORT void JNICALL Java_java_lang_VMClassLoader_resolveClass(JNIEnv *env, jclass clazz, java_lang_Class *c)
{
	classinfo *ci;

	ci = (classinfo *) c;

	if (!ci) {
		*exceptionptr = new_exception(string_java_lang_NullPointerException);
		return;
	}

	/* link the class */
	if (!ci->linked)
		class_link(ci);

	return;
}


/*
 * Class:     java/lang/VMClassLoader
 * Method:    loadClass
 * Signature: (Ljava/lang/String;Z)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_java_lang_VMClassLoader_loadClass(JNIEnv *env, jclass clazz, java_lang_String *name, jboolean resolve)
{
	classinfo *c;
	utf *u;

	if (!name) {
		*exceptionptr = new_exception(string_java_lang_NullPointerException);
		return NULL;
	}

	/* create utf string in which '.' is replaced by '/' */
	u = javastring_toutf(name, true);

	/* create class */
	c = class_new(u);

	/* load class */
	if (!class_load(c)) {
		classinfo *xclass;

		xclass = (*exceptionptr)->vftbl->class;

		/* if the exception is a NoClassDefFoundError, we replace it with a
		   ClassNotFoundException, otherwise return the exception */

		if (xclass == class_get(utf_new_char(string_java_lang_NoClassDefFoundError))) {
			/* clear exceptionptr, because builtin_new checks for 
			   ExceptionInInitializerError */
			*exceptionptr = NULL;

			*exceptionptr =
				new_exception_javastring(string_java_lang_ClassNotFoundException, name);
		}

		return NULL;
	}

	/* resolve class -- if requested */
	/* XXX TWISTI: we do not support REAL (at runtime) lazy linking */
/*  	if (resolve) */
		if (!class_link(c))
			return NULL;

	use_class_as_object(c);

	return (java_lang_Class *) c;
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
