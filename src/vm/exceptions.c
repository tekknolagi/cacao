/* src/vm/exceptions.c - exception related functions

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

   Authors: Christian Thalinger

   Changes: Edwin Steiner

   $Id: exceptions.c 4357 2006-01-22 23:33:38Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "vm/types.h"

#include "mm/memory.h"
#include "native/native.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_Throwable.h"
#include "toolbox/logging.h"
#include "toolbox/util.h"
#include "vm/class.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/jit.h"
#include "vm/jit/methodheader.h"


/* for raising exceptions from native methods *********************************/

#if !defined(USE_THREADS) || !defined(NATIVE_THREADS)
java_objectheader *_no_threads_exceptionptr = NULL;
#endif


/* init_system_exceptions ******************************************************

   Load and link exceptions used in the system.

*******************************************************************************/

bool exceptions_init(void)
{
	/* java/lang/Throwable */

	if (!(class_java_lang_Throwable =
		  load_class_bootstrap(utf_java_lang_Throwable)) ||
		!link_class(class_java_lang_Throwable))
		return false;


	/* java/lang/VMThrowable */

	if (!(class_java_lang_VMThrowable =
		  load_class_bootstrap(utf_java_lang_VMThrowable)) ||
		!link_class(class_java_lang_VMThrowable))
		return false;


	/* java/lang/Error */

	if (!(class_java_lang_Error = load_class_bootstrap(utf_java_lang_Error)) ||
		!link_class(class_java_lang_Error))
		return false;

	/* java/lang/NoClassDefFoundError */

	if (!(class_java_lang_NoClassDefFoundError =
		  load_class_bootstrap(utf_java_lang_NoClassDefFoundError)) ||
		!link_class(class_java_lang_NoClassDefFoundError))
		return false;

	/* java/lang/LinkageError */

	if (!(class_java_lang_LinkageError =
		  load_class_bootstrap(utf_java_lang_LinkageError)) ||
		!link_class(class_java_lang_LinkageError))
		return false;

	/* java/lang/NoSuchMethodError */

	if (!(class_java_lang_NoSuchMethodError =
		  load_class_bootstrap(utf_java_lang_NoSuchMethodError)) ||
		!link_class(class_java_lang_NoSuchMethodError))
		return false;

	/* java/lang/OutOfMemoryError */

	if (!(class_java_lang_OutOfMemoryError =
		  load_class_bootstrap(utf_java_lang_OutOfMemoryError)) ||
		!link_class(class_java_lang_OutOfMemoryError))
		return false;


	/* java/lang/Exception */

	if (!(class_java_lang_Exception =
		  load_class_bootstrap(utf_java_lang_Exception)) ||
		!link_class(class_java_lang_Exception))
		return false;

	/* java/lang/ClassNotFoundException */

	if (!(class_java_lang_ClassNotFoundException =
		  load_class_bootstrap(utf_java_lang_ClassNotFoundException)) ||
		!link_class(class_java_lang_ClassNotFoundException))
		return false;

	/* java/lang/IllegalArgumentException */

	if (!(class_java_lang_IllegalArgumentException =
		  load_class_bootstrap(utf_java_lang_IllegalArgumentException)) ||
		!link_class(class_java_lang_IllegalArgumentException))
		return false;

	/* java/lang/IllegalMonitorStateException */

	if (!(class_java_lang_IllegalMonitorStateException =
		  load_class_bootstrap(utf_java_lang_IllegalMonitorStateException)) ||
		!link_class(class_java_lang_IllegalMonitorStateException))
		return false;

	/* java/lang/NullPointerException */

	if (!(class_java_lang_NullPointerException =
		  load_class_bootstrap(utf_java_lang_NullPointerException)) ||
		!link_class(class_java_lang_NullPointerException))
		return false;


	return true;
}


static void throw_exception_exit_intern(bool doexit)
{
	java_objectheader *xptr;
	classinfo *c;
	methodinfo *pss;

	xptr = *exceptionptr;

	if (xptr) {
		/* clear exception, because we are calling jit code again */
		*exceptionptr = NULL;

		c = xptr->vftbl->class;

		pss = class_resolveclassmethod(c,
									   utf_printStackTrace,
									   utf_void__void,
									   class_java_lang_Object,
									   false);

		/* print the stacktrace */

		if (pss) {
			ASM_CALLJAVAFUNCTION(pss, xptr, NULL, NULL, NULL);

			/* This normally means, we are EXTREMLY out of memory or have a   */
			/* serious problem while printStackTrace. But may be another      */
			/* exception, so print it.                                        */

			if (*exceptionptr) {
				java_lang_Throwable *t;

				t = (java_lang_Throwable *) *exceptionptr;

				fprintf(stderr, "Exception while printStackTrace(): ");
				utf_fprint_classname(stderr, t->header.vftbl->class->name);

				if (t->detailMessage) {
					char *buf;

					buf = javastring_tochar((java_objectheader *) t->detailMessage);
					fprintf(stderr, ": %s", buf);
					MFREE(buf, char, strlen(buf));
				}
					
				fprintf(stderr, "\n");
			}

		} else {
			utf_fprint_classname(stderr, c->name);
			fprintf(stderr, ": printStackTrace()V not found!\n");
		}

		fflush(stderr);

		/* good bye! */

		if (doexit)
			exit(1);
	}
}


void throw_exception(void)
{
	throw_exception_exit_intern(false);
}


void throw_exception_exit(void)
{
	throw_exception_exit_intern(true);
}


void throw_main_exception(void)
{
	fprintf(stderr, "Exception in thread \"main\" ");
	fflush(stderr);

	throw_exception_exit_intern(false);
}


void throw_main_exception_exit(void)
{
	fprintf(stderr, "Exception in thread \"main\" ");
	fflush(stderr);

	throw_exception_exit_intern(true);
}


void throw_cacao_exception_exit(const char *exception, const char *message, ...)
{
	s4 i;
	char *tmp;
	s4 len;
	va_list ap;

	len = strlen(exception);
	tmp = MNEW(char, len + 1);
	strncpy(tmp, exception, len);
	tmp[len] = '\0';

	/* convert to classname */

   	for (i = len - 1; i >= 0; i--)
 	 	if (tmp[i] == '/') tmp[i] = '.';

	fprintf(stderr, "Exception in thread \"main\" %s", tmp);

	MFREE(tmp, char, len);

	if (strlen(message) > 0) {
		fprintf(stderr, ": ");

		va_start(ap, message);
		vfprintf(stderr, message, ap);
		va_end(ap);
	}

	fprintf(stderr, "\n");
	fflush(stderr);

	/* good bye! */

	exit(1);
}


/* exceptions_throw_outofmemory_exit *******************************************

   Just print an: java.lang.InternalError: Out of memory

*******************************************************************************/

void exceptions_throw_outofmemory_exit(void)
{
	throw_cacao_exception_exit(string_java_lang_InternalError,
							   "Out of memory");
}


/* new_exception ***************************************************************

   Creates an exception object with the given name and initalizes it.

*******************************************************************************/

java_objectheader *new_exception(const char *classname)
{
	java_objectheader *o;
	classinfo         *c;

	if (!(c = load_class_bootstrap(utf_new_char(classname))))
		return *exceptionptr;

	o = native_new_and_init(c);

	if (!o)
		return *exceptionptr;

	return o;
}


/* new_exception_message *******************************************************

   Creates an exception object with the given name and initalizes it
   with the given char message.

*******************************************************************************/

java_objectheader *new_exception_message(const char *classname,
										 const char *message)
{
	java_objectheader *o;
	classinfo         *c;
   
	if (!(c = load_class_bootstrap(utf_new_char(classname))))
		return *exceptionptr;

	o = native_new_and_init_string(c, javastring_new_char(message));

	if (!o)
		return *exceptionptr;

	return o;
}


/* new_exception_throwable *****************************************************

   Creates an exception object with the given name and initalizes it
   with the given java/lang/Throwable exception.

*******************************************************************************/

java_objectheader *new_exception_throwable(const char *classname,
										   java_lang_Throwable *throwable)
{
	java_objectheader *o;
	classinfo         *c;
   
	if (!(c = load_class_bootstrap(utf_new_char(classname))))
		return *exceptionptr;

	o = native_new_and_init_throwable(c, throwable);

	if (!o)
		return *exceptionptr;

	return o;
}


/* new_exception_utfmessage ****************************************************

   Creates an exception object with the given name and initalizes it
   with the given utf message.

*******************************************************************************/

java_objectheader *new_exception_utfmessage(const char *classname, utf *message)
{
	java_objectheader *o;
	classinfo         *c;
   
	if (!(c = load_class_bootstrap(utf_new_char(classname))))
		return *exceptionptr;

	o = native_new_and_init_string(c, javastring_new(message));

	if (!o)
		return *exceptionptr;

	return o;
}


/* new_exception_javastring ****************************************************

   Creates an exception object with the given name and initalizes it
   with the given java/lang/String message.

*******************************************************************************/

java_objectheader *new_exception_javastring(const char *classname,
											java_lang_String *message)
{
	java_objectheader *o;
	classinfo         *c;
   
	if (!(c = load_class_bootstrap(utf_new_char(classname))))
		return *exceptionptr;

	o = native_new_and_init_string(c, message);

	if (!o)
		return *exceptionptr;

	return o;
}


/* new_exception_int ***********************************************************

   Creates an exception object with the given name and initalizes it
   with the given int value.

*******************************************************************************/

java_objectheader *new_exception_int(const char *classname, s4 i)
{
	java_objectheader *o;
	classinfo         *c;
   
	if (!(c = load_class_bootstrap(utf_new_char(classname))))
		return *exceptionptr;

	o = native_new_and_init_int(c, i);

	if (!o)
		return *exceptionptr;

	return o;
}


/* new_classformaterror ********************************************************

   generates a java.lang.ClassFormatError for the classloader

*******************************************************************************/

java_objectheader *new_classformaterror(classinfo *c, const char *message, ...)
{
	java_objectheader *o;
	char              *msg;
	s4                 msglen;
	va_list            ap;

	/* calculate message length */

	msglen = 0;

	if (c)
		msglen += utf_strlen(c->name) + strlen(" (");

	va_start(ap, message);
	msglen += get_variable_message_length(message, ap);
	va_end(ap);

	if (c)
		msglen += strlen(")");

	msglen += strlen("0");

	/* allocate a buffer */

	msg = MNEW(char, msglen);

	/* print message into allocated buffer */

	if (c) {
		utf_sprint_classname(msg, c->name);
		strcat(msg, " (");
	}

	va_start(ap, message);
	vsprintf(msg + strlen(msg), message, ap);
	va_end(ap);

	if (c)
		strcat(msg, ")");

	o = new_exception_message(string_java_lang_ClassFormatError, msg);

	MFREE(msg, char, msglen);

	return o;
}


/* exceptions_throw_classformaterror *******************************************

   Generates a java.lang.ClassFormatError for the VM system throw it
   in the VM system.

*******************************************************************************/

void exceptions_throw_classformaterror(classinfo *c, const char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	*exceptionptr = new_classformaterror(c, message, ap);
	va_end(ap);
}


/* new_classnotfoundexception **************************************************

   Generates a java.lang.ClassNotFoundException for the classloader.

*******************************************************************************/

java_objectheader *new_classnotfoundexception(utf *name)
{
	java_objectheader *o;

	o = native_new_and_init_string(class_java_lang_ClassNotFoundException,
								   javastring_new(name));

	if (!o)
		return *exceptionptr;

	return o;
}


/* new_noclassdeffounderror ****************************************************

   Generates a java.lang.NoClassDefFoundError

*******************************************************************************/

java_objectheader *new_noclassdeffounderror(utf *name)
{
	java_objectheader *o;

	o = native_new_and_init_string(class_java_lang_NoClassDefFoundError,
								   javastring_new(name));

	if (!o)
		return *exceptionptr;

	return o;
}


/* classnotfoundexception_to_noclassdeffounderror ******************************

   Check the *exceptionptr for a ClassNotFoundException. If it is one,
   convert it to a NoClassDefFoundError.

*******************************************************************************/

void classnotfoundexception_to_noclassdeffounderror(void)
{
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
			return;

		/* set new exception */

		*exceptionptr = xptr;
	}
}


/* new_internalerror ***********************************************************

   Generates a java.lang.InternalError for the VM.

*******************************************************************************/

java_objectheader *new_internalerror(const char *message, ...)
{
	java_objectheader *o;
	va_list            ap;
	char              *msg;
	s4                 msglen;

	/* calculate exception message length */

	va_start(ap, message);
	msglen = get_variable_message_length(message, ap);
	va_end(ap);

	/* allocate memory */

	msg = MNEW(char, msglen);

	/* generate message */

	va_start(ap, message);
	vsprintf(msg, message, ap);
	va_end(ap);

	/* create exception object */

	o = new_exception_message(string_java_lang_InternalError, msg);

	/* free memory */

	MFREE(msg, char, msglen);

	return o;
}


/* exceptions_new_linkageerror *************************************************

   Generates a java.lang.LinkageError with an error message.
   If c != NULL, the name of c is appended to the error message.

*******************************************************************************/

java_objectheader *exceptions_new_linkageerror(const char *message,
											   classinfo *c)
{
	java_objectheader *o;
	char              *msg;
	s4                 msglen;

	/* calculate exception message length */

	msglen = strlen(message) + 1;
	if (c) {
		msglen += utf_strlen(c->name);
	}
		
	/* allocate memory */

	msg = MNEW(char, msglen);

	/* generate message */

	strcpy(msg,message);
	if (c) {
		utf_strcat(msg, c->name);
	}

	o = native_new_and_init_string(class_java_lang_LinkageError,
								   javastring_new_char(msg));

	/* free memory */

	MFREE(msg, char, msglen);

	return o;
}


/* exceptions_new_nosuchmethoderror ********************************************

   Generates a java.lang.NoSuchMethodError with an error message.

*******************************************************************************/

java_objectheader *exceptions_new_nosuchmethoderror(classinfo *c,
													utf *name, utf *desc)
{
	java_objectheader *o;
	char              *msg;
	s4                 msglen;

	/* calculate exception message length */

	msglen = utf_strlen(c->name) + strlen(".") + utf_strlen(name) +
		utf_strlen(desc) + strlen("0");

	/* allocate memory */

	msg = MNEW(char, msglen);

	/* generate message */

	utf_sprint(msg, c->name);
	strcat(msg, ".");
	utf_strcat(msg, name);
	utf_strcat(msg, desc);

	o = native_new_and_init_string(class_java_lang_NoSuchMethodError,
								   javastring_new_char(msg));

	/* free memory */

	MFREE(msg, char, msglen);

	return o;
}


/* new_unsupportedclassversionerror ********************************************

   generates a java.lang.UnsupportedClassVersionError for the classloader

*******************************************************************************/

java_objectheader *new_unsupportedclassversionerror(classinfo *c, const char *message, ...)
{
	java_objectheader *o;
	va_list            ap;
	char              *msg;
    s4                 msglen;

	/* calculate exception message length */

	msglen = utf_strlen(c->name) + strlen(" (") + strlen(")") + strlen("0");

	va_start(ap, message);
	msglen += get_variable_message_length(message, ap);
	va_end(ap);

	/* allocate memory */

	msg = MNEW(char, msglen);

	/* generate message */

	utf_sprint_classname(msg, c->name);
	strcat(msg, " (");

	va_start(ap, message);
	vsprintf(msg + strlen(msg), message, ap);
	va_end(ap);

	strcat(msg, ")");

	/* create exception object */

	o = new_exception_message(string_java_lang_UnsupportedClassVersionError,
							  msg);

	/* free memory */

	MFREE(msg, char, msglen);

	return o;
}


/* new_verifyerror *************************************************************

   generates a java.lang.VerifyError for the jit compiler

*******************************************************************************/

java_objectheader *new_verifyerror(methodinfo *m, const char *message, ...)
{
	java_objectheader *o;
	va_list            ap;
	char              *msg;
	s4                 msglen;

	useinlining = false; /* at least until sure inlining works with exceptions*/

	/* calculate exception message length */

	msglen = 0;

	if (m)
		msglen = strlen("(class: ") + utf_strlen(m->class->name) +
			strlen(", method: ") + utf_strlen(m->name) +
			strlen(" signature: ") + utf_strlen(m->descriptor) +
			strlen(") ") + strlen("0");

	va_start(ap, message);
	msglen += get_variable_message_length(message, ap);
	va_end(ap);

	/* allocate memory */

	msg = MNEW(char, msglen);

	/* generate message */

	if (m) {
		strcpy(msg, "(class: ");
		utf_strcat(msg, m->class->name);
		strcat(msg, ", method: ");
		utf_strcat(msg, m->name);
		strcat(msg, " signature: ");
		utf_strcat(msg, m->descriptor);
		strcat(msg, ") ");
	}

	va_start(ap, message);
	vsprintf(msg + strlen(msg), message, ap);
	va_end(ap);

	/* create exception object */

	o = new_exception_message(string_java_lang_VerifyError, msg);

	/* free memory */

	MFREE(msg, char, msglen);

	return o;
}


/* new_arithmeticexception *****************************************************

   Generates a java.lang.ArithmeticException for the jit compiler.

*******************************************************************************/

java_objectheader *new_arithmeticexception(void)
{
	java_objectheader *e;

	e = new_exception_message(string_java_lang_ArithmeticException,
							  string_java_lang_ArithmeticException_message);

	if (!e)
		return *exceptionptr;

	return e;
}


/* exceptions_new_arrayindexoutofboundsexception *******************************

   Generates a java.lang.ArrayIndexOutOfBoundsException for the VM
   system.

*******************************************************************************/

java_objectheader *new_arrayindexoutofboundsexception(s4 index)
{
	java_objectheader *e;
	methodinfo        *m;
	java_objectheader *o;
	java_lang_String  *s;

	/* convert the index into a String, like Sun does */

	m = class_resolveclassmethod(class_java_lang_String,
								 utf_new_char("valueOf"),
								 utf_new_char("(I)Ljava/lang/String;"),
								 class_java_lang_Object,
								 true);

	if (!m)
		return *exceptionptr;

	ASM_CALLJAVAFUNCTION_ADR(o, m, (void *) (ptrint) index, NULL, NULL, NULL);

	s = (java_lang_String *) o;

	if (!s)
		return *exceptionptr;

	e = new_exception_javastring(string_java_lang_ArrayIndexOutOfBoundsException,
								 s);

	if (!e)
		return *exceptionptr;

	return e;
}


/* exceptions_throw_arrayindexoutofboundsexception *****************************

   Generates a java.lang.ArrayIndexOutOfBoundsException for the VM
   system.

*******************************************************************************/

void exceptions_throw_arrayindexoutofboundsexception(void)
{
	java_objectheader *e;

	e = new_exception(string_java_lang_ArrayIndexOutOfBoundsException);

	if (!e)
		return;

	*exceptionptr = e;
}


/* new_arraystoreexception *****************************************************

   generates a java.lang.ArrayStoreException for the jit compiler

*******************************************************************************/

java_objectheader *new_arraystoreexception(void)
{
	java_objectheader *e;

	e = new_exception(string_java_lang_ArrayStoreException);
/*  	e = native_new_and_init(class_java_lang_ArrayStoreException); */

	if (!e)
		return *exceptionptr;

	return e;
}


/* new_classcastexception ******************************************************

   generates a java.lang.ClassCastException for the jit compiler

*******************************************************************************/

java_objectheader *new_classcastexception(void)
{
	java_objectheader *e;

	e = new_exception(string_java_lang_ClassCastException);

	if (!e)
		return *exceptionptr;

	return e;
}


/* exceptions_new_illegalargumentexception *************************************

   Generates a java.lang.IllegalArgumentException for the VM system.

*******************************************************************************/

java_objectheader *new_illegalargumentexception(void)
{
	java_objectheader *e;

	e = native_new_and_init(class_java_lang_IllegalArgumentException);

	if (!e)
		return *exceptionptr;

	return e;
}


/* exceptions_throw_illegalargumentexception ***********************************

   Generates a java.lang.IllegalArgumentException for the VM system
   and throw it in the VM system.

*******************************************************************************/

void exceptions_throw_illegalargumentexception(void)
{
	*exceptionptr = new_illegalargumentexception();
}


/* new_illegalmonitorstateexception ********************************************

   Generates a java.lang.IllegalMonitorStateException for the VM
   thread system.

*******************************************************************************/

java_objectheader *new_illegalmonitorstateexception(void)
{
	java_objectheader *e;

	e = native_new_and_init(class_java_lang_IllegalMonitorStateException);

	if (!e)
		return *exceptionptr;

	return e;
}


/* exceptions_new_negativearraysizeexception ***********************************

   Generates a java.lang.NegativeArraySizeException for the VM system.

*******************************************************************************/

java_objectheader *new_negativearraysizeexception(void)
{
	java_objectheader *e;

	e = new_exception(string_java_lang_NegativeArraySizeException);

	if (!e)
		return *exceptionptr;

	return e;
}


/* exceptions_throw_negativearraysizeexception *********************************

   Generates a java.lang.NegativeArraySizeException for the VM system.

*******************************************************************************/

void exceptions_throw_negativearraysizeexception(void)
{
	*exceptionptr = new_negativearraysizeexception();
}


/* new_nullpointerexception ****************************************************

   generates a java.lang.NullPointerException for the jit compiler

*******************************************************************************/

java_objectheader *new_nullpointerexception(void)
{
	java_objectheader *e;

	e = native_new_and_init(class_java_lang_NullPointerException);

	if (!e)
		return *exceptionptr;

	return e;
}


/* exceptions_throw_nullpointerexception ***************************************

   Generates a java.lang.NullPointerException for the VM system and
   throw it in the VM system.

*******************************************************************************/

void exceptions_throw_nullpointerexception(void)
{
	*exceptionptr = new_nullpointerexception();
}


/* exceptions_handle_exception *************************************************

   XXX

*******************************************************************************/

u1 *exceptions_handle_exception(java_objectheader *xptr, u1 *xpc, u1 *pv, u1 *sp)
{
	methodinfo            *m;
	s4                     framesize;
	s4                     issync;
	exceptionentry        *ex;
	s4                     exceptiontablelength;
	s4                     i;
	classref_or_classinfo  cr;
	classinfo             *c;
	java_objectheader     *o;

	/* get methodinfo pointer from method header */

	m                    = *((methodinfo **)    (pv + MethodPointer));
	framesize            = *((s4 *)             (pv + FrameSize));
	issync               = *((s4 *)             (pv + IsSync));
	ex                   =   (exceptionentry *) (pv + ExTableStart);
	exceptiontablelength = *((s4 *)             (pv + ExTableSize));

#if 0
 	if (m != NULL) {
 		printf("exceptions_handle_exception(%p, %p, %p, %p): ", xptr, xpc, pv, sp);
 		utf_display(m->class->name);
 		printf(".");
 		utf_display(m->name);
 		utf_display(m->descriptor);
 		printf(", %d\n", exceptiontablelength);
 	}
#endif

	/* print exception trace */

	if (opt_verbose || runverbose || opt_verboseexception)
		builtin_trace_exception(xptr, m, xpc, 1);

	for (i = 0; i < exceptiontablelength; i++) {
		/* ATTENTION: keep this here, as we need to decrement the
           pointer before the loop executes! */

		ex--;

		/* is the xpc is the current catch range */

		if (ex->startpc <= xpc && xpc < ex->endpc) {
			cr = ex->catchtype;

			/* NULL catches everything */

			if (cr.any == NULL)
				return ex->handlerpc;

			/* resolve or load/link the exception class */

			if (IS_CLASSREF(cr)) {
				c = resolve_classref_eager(cr.ref);

			} else {
				c = cr.cls;

				if (!(c->state & CLASS_LOADED))
					/* use the methods' classloader */
					if (!load_class_from_classloader(c->name,
													 m->class->classloader))
						return NULL;

				if (!(c->state & CLASS_LINKED))
					if (!link_class(c))
						return NULL;
			}

			/* is the thrown exception an instance of the catch class? */

			if (builtin_instanceof(xptr, c))
				return ex->handlerpc;
		}
	}

#if defined(USE_THREADS)
	/* is this method synchronized? */

	if (issync) {
		/* get synchronization object */

# if defined(__MIPS__) && (SIZEOF_VOID_P == 4)
		/* XXX change this if we ever want to use 4-byte stackslots */
		o = *((java_objectheader **) (sp + issync - 8));
# else
		o = *((java_objectheader **) (sp + issync - SIZEOF_VOID_P));
#endif

		assert(o != NULL);

		builtin_monitorexit(o);
	}
#endif

	/* none of the exceptions catch this one */

	return NULL;
}


/* exceptions_print_exception **************************************************

   Prints an exception, the detail message and the cause, if
   available, with CACAO internal functions to stdout.

*******************************************************************************/

#if !defined(NDEBUG)
void exceptions_print_exception(java_objectheader *xptr)
{
	java_lang_Throwable   *t;
	java_lang_Throwable   *cause;
	utf                   *u;

	t = (java_lang_Throwable *) xptr;
	cause = t->cause;

	/* print the root exception */

	utf_display_classname(t->header.vftbl->class->name);

	if (t->detailMessage) {
		u = javastring_toutf(t->detailMessage, false);

		printf(": ");
		utf_display(u);
	}

	putc('\n', stdout);

	/* print the cause if available */

	if (cause && (cause != t)) {
		printf("Caused by: ");
		utf_display_classname(cause->header.vftbl->class->name);

		if (cause->detailMessage) {
			u = javastring_toutf(cause->detailMessage, false);

			printf(": ");
			utf_display(u);
		}

		putc('\n', stdout);
	}
}
#endif /* !defined(NDEBUG) */


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
