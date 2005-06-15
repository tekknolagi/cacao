/* src/native/native.h - table of native functions

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

   Authors: Reinhard Grafl

   Changes: Christian Thalinger

   $Id: native.h 2707 2005-06-15 13:40:36Z twisti $

*/


#ifndef _NATIVE_H
#define _NATIVE_H

#if !defined(STATIC_CLASSPATH)
# include "libltdl/ltdl.h"
#endif

#include "vm/class.h"
#include "vm/global.h"
#include "vm/method.h"
#include "vm/utf8.h"
#include "native/jni.h"
#include "native/include/java_lang_String.h"
#include "native/include/java_lang_ClassLoader.h"
#include "native/include/java_lang_Throwable.h"


/* table for locating native methods */

typedef struct nativeref nativeref;
typedef struct nativecompref nativecompref;


#if !defined(STATIC_CLASSPATH)
typedef struct library_hash_loader_entry library_hash_loader_entry;
typedef struct library_hash_name_entry library_hash_name_entry;

/* library_hash_loader_entry **************************************************/

struct library_hash_loader_entry {
	java_objectheader         *loader;  /* class loader                       */
	library_hash_name_entry   *namelink;/* libraries loaded by this loader    */
	library_hash_loader_entry *hashlink;/* link for external chaining         */
};


/* library_hash_name_entry ****************************************************/

struct library_hash_name_entry {
	utf                     *name;      /* library name                       */
	lt_dlhandle              handle;    /* libtool library handle             */
	library_hash_name_entry *hashlink;  /* link for external chaining         */
};
#endif


struct nativeref {
	char       *classname;
	char       *methodname;
	char       *descriptor;
	bool        isstatic;
	functionptr func;
};

/* table for fast string comparison */

struct nativecompref {
	utf        *classname;
	utf        *methodname;
	utf        *descriptor;
	bool        isstatic;
	functionptr func;
};


void use_class_as_object(classinfo *c);

/* initialize native subsystem */
bool native_init(void);

/* find native function */
functionptr native_findfunction(utf *cname, utf *mname, 
								utf *desc, bool isstatic);

#if !defined(STATIC_CLASSPATH)
/* add a library to the library hash */
void native_library_hash_add(utf *filename, java_objectheader *loader,
							 lt_dlhandle handle);

/* resolve native function */
functionptr native_resolve_function(methodinfo *m);
#endif

/* create new object on the heap and call the initializer */
java_objectheader *native_new_and_init(classinfo *c);

/* create new object on the heap and call the initializer 
   mainly used for exceptions with a message */
java_objectheader *native_new_and_init_string(classinfo *c, java_lang_String *s);

/* create new object on the heap and call the initializer 
   mainly used for exceptions with an index */
java_objectheader *native_new_and_init_int(classinfo *c, s4 i);

/* create new object on the heap and call the initializer 
   mainly used for exceptions with cause */
java_objectheader *native_new_and_init_throwable(classinfo *c, java_lang_Throwable *t);

/* add property to temporary property list -- located in nat/VMRuntime.c */
void create_property(char *key, char *value);

/* correct vftbl-entries of javastring-hash */
void stringtable_update();


/* search 'classinfo'-structure for a field with the specified name */
fieldinfo *class_findfield_approx(classinfo *c, utf *name);
s4 class_findfield_index_approx(classinfo *c, utf *name);

void copy_vftbl(vftbl_t **dest, vftbl_t *src);

utf *create_methodsig(java_objectarray* types, char *retType);
classinfo *get_type(char **utf_ptr,char *desc_end, bool skip);
java_objectarray* get_parametertypes(methodinfo *m);
java_objectarray* get_exceptiontypes(methodinfo *m);
classinfo *get_returntype(methodinfo *m);


/*----- For Static Analysis of Natives by parseRT -----*/

/*---------- global variables ---------------------------*/
typedef struct classMeth classMeth;
typedef struct nativeCall   nativeCall;
typedef struct methodCall   methodCall;
typedef struct nativeMethod nativeMethod;

typedef struct nativeCompCall   nativeCompCall;
typedef struct methodCompCall   methodCompCall;
typedef struct nativeCompMethod nativeCompMethod;

/*---------- Define Constants ---------------------------*/
#define MAXCALLS 30 

struct classMeth {
	int i_class;
	int j_method;
	int methCnt;
};

struct  methodCall{
	char *classname;
	char *methodname;
	char *descriptor;
};

struct  nativeMethod  {
	char *methodname;
	char *descriptor;
	struct methodCall methodCalls[MAXCALLS];
};


struct nativeCall {
	char *classname;
	struct nativeMethod methods[MAXCALLS];
	int methCnt;
	int callCnt[MAXCALLS];
};


struct methodCompCall {
	utf *classname;
	utf *methodname;
	utf *descriptor;
};


struct nativeCompMethod {
	utf *methodname;
	utf *descriptor;
	struct methodCompCall methodCalls[MAXCALLS];
};


struct nativeCompCall {
	utf *classname;
	struct nativeCompMethod methods[MAXCALLS];
	int methCnt;
	int callCnt[MAXCALLS];
};


bool natcall2utf(bool);
void printNativeCall(nativeCall);
void markNativeMethodsRT(utf *, utf* , utf* ); 

#endif /* _NATIVE_H */


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
