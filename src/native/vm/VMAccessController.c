/* native/vm/VMAccessController.c - java/security/VMAccessController

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
   $Id: VMAccessController.c 2485 2005-05-20 12:02:25Z jowenn $

*/

#include "native/jni.h"
#include "vm/global.h"
#include "native/include/java_security_VMAccessController.h"
#include "vm/class.h"
#include "vm/builtin.h"
#include "toolbox/logging.h"
#include "vm/jit/stacktrace.h"
#include "vm/loader.h"

/*
 * Class:     java/security/VMAccessController
 * Method:    getStack
 * Signature: ()[[Ljava/lang/Object;
 */
JNIEXPORT java_objectarray* JNICALL Java_java_security_VMAccessController_getStack(JNIEnv *env, jclass clazz) {
#if defined(__I386__) || defined(__ALPHA__) || defined (__x86_64__)
	return cacao_getStackForVMAccessController();
#else
	java_objectarray *result=builtin_anewarray(2,arrayclass_java_lang_Object);
        if (result==0) panic("getStackCollector (very out of memory)");
        result->data[0]=(java_objectheader*)builtin_anewarray(0,class_java_lang_Class);
        result->data[1]=(java_objectheader*)builtin_anewarray(0,class_java_lang_String);
	return result;
#endif
}

