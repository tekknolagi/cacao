/* src/native/vm/nativevm.h - register the native functions

   Copyright (C) 2007 R. Grafl, A. Krall, C. Kruegel,
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

   $Id: native.c 7906 2007-05-14 17:25:33Z twisti $

*/


#ifndef _NATIVEVM_H
#define _NATIVEVM_H

#include "config.h"
#include "vm/types.h"

/* function prototypes ********************************************************/

void nativevm_init(void);

#if defined(ENABLE_JAVASE)
# if defined(WITH_CLASSPATH_GNU)
void _Jv_gnu_classpath_VMStackWalker_init();
void _Jv_gnu_classpath_VMSystemProperties_init();
void _Jv_gnu_java_lang_management_VMClassLoadingMXBeanImpl_init();
void _Jv_gnu_java_lang_management_VMMemoryMXBeanImpl_init();
void _Jv_gnu_java_lang_management_VMRuntimeMXBeanImpl_init();
void _Jv_gnu_java_lang_management_VMThreadMXBeanImpl_init();
void _Jv_java_lang_VMClass_init();
void _Jv_java_lang_VMClassLoader_init();
void _Jv_java_lang_VMObject_init();
void _Jv_java_lang_VMRuntime_init();
void _Jv_java_lang_VMString_init();
void _Jv_java_lang_VMSystem_init();
void _Jv_java_lang_VMThread_init();
void _Jv_java_lang_VMThrowable_init();
void _Jv_java_lang_management_VMManagementFactory_init();
void _Jv_java_lang_reflect_Constructor_init();
void _Jv_java_lang_reflect_Field_init();
void _Jv_java_lang_reflect_Method_init();
void _Jv_java_lang_reflect_VMProxy_init();
void _Jv_java_security_VMAccessController_init();
void _Jv_sun_misc_Unsafe_init();
# else
#  error unknown classpath configuration
# endif
#elif defined(ENABLE_JAVAME_CLDC1_1)
void _Jv_com_sun_cldc_io_ResourceInputStream_init();
void _Jv_com_sun_cldc_io_j2me_socket_Protocol_init();
void _Jv_com_sun_cldchi_io_ConsoleOutputStream_init();
void _Jv_com_sun_cldchi_jvm_JVM_init();
void _Jv_java_lang_Class_init();
void _Jv_java_lang_Double_init();
void _Jv_java_lang_Float_init();
void _Jv_java_lang_Math_init();
void _Jv_java_lang_Object_init();
void _Jv_java_lang_Runtime_init();
void _Jv_java_lang_String_init();
void _Jv_java_lang_System_init();
void _Jv_java_lang_Thread_init();
void _Jv_java_lang_Throwable_init();
#else
# error unknown Java configuration
#endif

#endif /* _NATIVEVM_H */

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