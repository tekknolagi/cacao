/* src/native/vm/cldc1.1/com_sun_cldchi_io_ConsoleOutputStream.cpp

   Copyright (C) 2006, 2007, 2008
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

#include <stdint.h>
#include <stdio.h>

#include "vm/types.h"

#include "native/jni.hpp"
#include "native/native.h"

#if defined(ENABLE_JNI_HEADERS)
# include "native/include/com_sun_cldchi_io_ConsoleOutputStream.h"
#endif


// Native functions are exported as C functions.
extern "C" {

/*
 * Class:     com/sun/cldchi/io/ConsoleOutputStream
 * Method:    write
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_sun_cldchi_io_ConsoleOutputStream_write(JNIEnv *env, jobject _this, jint c)
{
	(void) fputc(c, stdout);
}

} // extern "C"


/* native methods implemented by this file ************************************/
 
static JNINativeMethod methods[] = {
	{ (char*) "write", (char*) "(I)V", (void*) (uintptr_t) &Java_com_sun_cldchi_io_ConsoleOutputStream_write },
};

 
/* _Jv_com_sun_cldchi_io_ConsoleOutputStream_init ******************************
 
   Register native functions.
 
*******************************************************************************/
 
// FIXME
extern "C" { 
void _Jv_com_sun_cldchi_io_ConsoleOutputStream_init(void)
{
	utf *u;
 
	u = utf_new_char("com/sun/cldchi/io/ConsoleOutputStream");
 
	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
