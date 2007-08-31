/* src/native/vm/sun_misc_Unsafe.c - sun/misc/Unsafe

   Copyright (C) 2006, 2007 R. Grafl, A. Krall, C. Kruegel, C. Oates,
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/


#include "config.h"

#include <stdint.h>
#include <unistd.h>

#include "mm/memory.h"

#include "native/jni.h"
#include "native/llni.h"
#include "native/native.h"

#include "native/include/java_lang_Object.h"                  /* before c.l.C */
#include "native/include/java_lang_String.h"            /* required by j.l.CL */

#if defined(WITH_CLASSPATH_SUN)
# include "native/include/java_nio_ByteBuffer.h"        /* required by j.l.CL */
#endif

#include "native/include/java_lang_ClassLoader.h"        /* required by j.l.C */
#include "native/include/java_lang_Class.h"
#include "native/include/java_lang_reflect_Field.h"
#include "native/include/java_lang_Thread.h"             /* required by s.m.U */
#include "native/include/java_lang_Throwable.h"

#include "native/include/java_security_ProtectionDomain.h" /* required by smU */

#include "native/include/sun_misc_Unsafe.h"

#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/initialize.h"
#include "vm/stringlocal.h"

#include "vmcore/utf8.h"


/* native methods implemented by this file ************************************/

static JNINativeMethod methods[] = {
	{ "registerNatives",        "()V",                                                        (void *) (intptr_t) &Java_sun_misc_Unsafe_registerNatives                },
	{ "getInt",                 "(Ljava/lang/Object;J)I",                                     (void *) (intptr_t) &Java_sun_misc_Unsafe_getInt__Ljava_lang_Object_2J   },
	{ "putInt",                 "(Ljava/lang/Object;JI)V",                                    (void *) (intptr_t) &Java_sun_misc_Unsafe_putInt__Ljava_lang_Object_2JI  },
	{ "getObject",              "(Ljava/lang/Object;J)Ljava/lang/Object;",                    (void *) (intptr_t) &Java_sun_misc_Unsafe_getObject                      },
	{ "putObject",              "(Ljava/lang/Object;JLjava/lang/Object;)V",                   (void *) (intptr_t) &Java_sun_misc_Unsafe_putObject                      },
	{ "getBoolean",             "(Ljava/lang/Object;J)Z",                                     (void *) (intptr_t) &Java_sun_misc_Unsafe_getBoolean                     },
	{ "putBoolean",             "(Ljava/lang/Object;JZ)V",                                    (void *) (intptr_t) &Java_sun_misc_Unsafe_putBoolean                     },
	{ "getByte",                "(Ljava/lang/Object;J)B",                                     (void *) (intptr_t) &Java_sun_misc_Unsafe_getByte__Ljava_lang_Object_2J  },
	{ "putByte",                "(Ljava/lang/Object;JB)V",                                    (void *) (intptr_t) &Java_sun_misc_Unsafe_putByte__Ljava_lang_Object_2JB },
	{ "putShort",               "(Ljava/lang/Object;JS)V",                                    (void *) (intptr_t) &Java_sun_misc_Unsafe_putShort__Ljava_lang_Object_2JS },
	{ "getChar",                "(Ljava/lang/Object;J)C",                                     (void *) (intptr_t) &Java_sun_misc_Unsafe_getChar__Ljava_lang_Object_2J  },
	{ "putChar",                "(Ljava/lang/Object;JC)V",                                    (void *) (intptr_t) &Java_sun_misc_Unsafe_putChar__Ljava_lang_Object_2JC },
	{ "getLong",                "(Ljava/lang/Object;J)J",                                     (void *) (intptr_t) &Java_sun_misc_Unsafe_getLong__Ljava_lang_Object_2J  },
	{ "getFloat",               "(Ljava/lang/Object;J)F",                                     (void *) (intptr_t) &Java_sun_misc_Unsafe_getFloat__Ljava_lang_Object_2J },
	{ "putFloat",               "(Ljava/lang/Object;JF)V",                                    (void *) (intptr_t) &Java_sun_misc_Unsafe_putFloat__Ljava_lang_Object_2JF },
	{ "getByte",                "(J)B",                                                       (void *) (intptr_t) &Java_sun_misc_Unsafe_getByte__J                     },
	{ "putByte",                "(JB)V",                                                      (void *) (intptr_t) &Java_sun_misc_Unsafe_putByte__JB                    },
	{ "getShort",               "(J)S",                                                       (void *) (intptr_t) &Java_sun_misc_Unsafe_getShort__J                    },
	{ "putShort",               "(JS)V",                                                      (void *) (intptr_t) &Java_sun_misc_Unsafe_putShort__JS                   },
	{ "getInt",                 "(J)I",                                                       (void *) (intptr_t) &Java_sun_misc_Unsafe_getInt__J                      },
	{ "putInt",                 "(JI)V",                                                      (void *) (intptr_t) &Java_sun_misc_Unsafe_putInt__JI                     },
	{ "getLong",                "(J)J",                                                       (void *) (intptr_t) &Java_sun_misc_Unsafe_getLong__J                     },
	{ "putLong",                "(JJ)V",                                                      (void *) (intptr_t) &Java_sun_misc_Unsafe_putLong__JJ                    },
	{ "getFloat",               "(J)F",                                                       (void *) (intptr_t) &Java_sun_misc_Unsafe_getFloat__J                    },
	{ "objectFieldOffset",      "(Ljava/lang/reflect/Field;)J",                               (void *) (intptr_t) &Java_sun_misc_Unsafe_objectFieldOffset              },
	{ "allocateMemory",         "(J)J",                                                       (void *) (intptr_t) &Java_sun_misc_Unsafe_allocateMemory                 },
	{ "setMemory",              "(Ljava/lang/Object;JJB)V",                                   (void *) (intptr_t) &Java_sun_misc_Unsafe_setMemory                      },
	{ "freeMemory",             "(J)V",                                                       (void *) (intptr_t) &Java_sun_misc_Unsafe_freeMemory                     },
	{ "staticFieldOffset",      "(Ljava/lang/reflect/Field;)J",                               (void *) (intptr_t) &Java_sun_misc_Unsafe_staticFieldOffset              },
	{ "staticFieldBase",        "(Ljava/lang/reflect/Field;)Ljava/lang/Object;",              (void *) (intptr_t) &Java_sun_misc_Unsafe_staticFieldBase                },
	{ "ensureClassInitialized", "(Ljava/lang/Class;)V",                                       (void *) (intptr_t) &Java_sun_misc_Unsafe_ensureClassInitialized         },
	{ "arrayBaseOffset",        "(Ljava/lang/Class;)I",                                       (void *) (intptr_t) &Java_sun_misc_Unsafe_arrayBaseOffset                },
	{ "arrayIndexScale",        "(Ljava/lang/Class;)I",                                       (void *) (intptr_t) &Java_sun_misc_Unsafe_arrayIndexScale                },
	{ "addressSize",            "()I",                                                        (void *) (intptr_t) &Java_sun_misc_Unsafe_addressSize                    },
	{ "pageSize",               "()I",                                                        (void *) (intptr_t) &Java_sun_misc_Unsafe_pageSize                       },
	{ "defineClass",            "(Ljava/lang/String;[BIILjava/lang/ClassLoader;Ljava/security/ProtectionDomain;)Ljava/lang/Class;", (void *) (intptr_t) &Java_sun_misc_Unsafe_defineClass__Ljava_lang_String_2_3BIILjava_lang_ClassLoader_2Ljava_security_ProtectionDomain_2 },
	{ "allocateInstance",       "(Ljava/lang/Class;)Ljava/lang/Object;",                      (void *) (intptr_t) &Java_sun_misc_Unsafe_allocateInstance               },
	{ "throwException",         "(Ljava/lang/Throwable;)V",                                   (void *) (intptr_t) &Java_sun_misc_Unsafe_throwException                 },
	{ "compareAndSwapObject",   "(Ljava/lang/Object;JLjava/lang/Object;Ljava/lang/Object;)Z", (void *) (intptr_t) &Java_sun_misc_Unsafe_compareAndSwapObject           },
	{ "compareAndSwapInt",      "(Ljava/lang/Object;JII)Z",                                   (void *) (intptr_t) &Java_sun_misc_Unsafe_compareAndSwapInt              },
	{ "compareAndSwapLong",     "(Ljava/lang/Object;JJJ)Z",                                   (void *) (intptr_t) &Java_sun_misc_Unsafe_compareAndSwapLong             },
	{ "getObjectVolatile",      "(Ljava/lang/Object;J)Ljava/lang/Object;",                    (void *) (intptr_t) &Java_sun_misc_Unsafe_getObjectVolatile              },
	{ "getIntVolatile",         "(Ljava/lang/Object;J)I",                                     (void *) (intptr_t) &Java_sun_misc_Unsafe_getIntVolatile                 },
	{ "getLongVolatile",        "(Ljava/lang/Object;J)J",                                     (void *) (intptr_t) &Java_sun_misc_Unsafe_getLongVolatile                },
	{ "unpark",                 "(Ljava/lang/Object;)V",                                      (void *) (intptr_t) &Java_sun_misc_Unsafe_unpark                         },
	{ "park",                   "(ZJ)V",                                                      (void *) (intptr_t) &Java_sun_misc_Unsafe_park                           },
};


/* _Jv_sun_misc_Unsafe_init ****************************************************

   Register native functions.

*******************************************************************************/

void _Jv_sun_misc_Unsafe_init(void)
{
	utf *u;

	u = utf_new_char("sun/misc/Unsafe");

	native_method_register(u, methods, NATIVE_METHODS_COUNT);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    registerNatives
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_registerNatives(JNIEnv *env, jclass clazz)
{
	/* The native methods of this function are already registered in
	   _Jv_sun_misc_Unsafe_init() which is called during VM
	   startup. */
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getInt
 * Signature: (Ljava/lang/Object;J)I
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getInt__Ljava_lang_Object_2J(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset)
{
	int32_t *p;
	int32_t  value;

	p = (int32_t *) (((uint8_t *) o) + offset);

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putInt
 * Signature: (Ljava/lang/Object;JI)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putInt__Ljava_lang_Object_2JI(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset, int32_t x)
{
	int32_t *p;

	p = (int32_t *) (((uint8_t *) o) + offset);

	*p = x;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getObject
 * Signature: (Ljava/lang/Object;J)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_sun_misc_Unsafe_getObject(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset)
{
	void **p;
	void  *value;

	p = (void **) (((uint8_t *) o) + offset);

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putObject
 * Signature: (Ljava/lang/Object;JLjava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putObject(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset, java_lang_Object *x)
{
	void **p;

	p = (void **) (((uint8_t *) o) + offset);

	*p = (void *) x;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getBoolean
 * Signature: (Ljava/lang/Object;J)Z
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getBoolean(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset)
{
	int32_t *p;
	int32_t  value;

	p = (int32_t *) (((uint8_t *) o) + offset);

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putBoolean
 * Signature: (Ljava/lang/Object;JZ)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putBoolean(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset, int32_t x)
{
	int32_t *p;

	p = (int32_t *) (((uint8_t *) o) + offset);

	*p = x;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getByte
 * Signature: (Ljava/lang/Object;J)B
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getByte__Ljava_lang_Object_2J(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset)
{
	int32_t *p;
	int32_t  value;

	p = (int32_t *) (((uint8_t *) o) + offset);

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putByte
 * Signature: (Ljava/lang/Object;JB)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putByte__Ljava_lang_Object_2JB(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset, int32_t x)
{
	int32_t *p;

	p = (int32_t *) (((uint8_t *) o) + offset);

	*p = x;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putShort
 * Signature: (Ljava/lang/Object;JS)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putShort__Ljava_lang_Object_2JS(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset, int32_t x)
{
	int32_t *p;

	p = (int32_t *) (((uint8_t *) o) + offset);

	*p = x;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getChar
 * Signature: (Ljava/lang/Object;J)C
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getChar__Ljava_lang_Object_2J(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset)
{
	int32_t *p;
	int32_t  value;

	p = (int32_t *) (((uint8_t *) o) + offset);

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putChar
 * Signature: (Ljava/lang/Object;JC)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putChar__Ljava_lang_Object_2JC(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset, int32_t x)
{
	int32_t *p;

	p = (int32_t *) (((uint8_t *) o) + offset);

	*p = x;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getLong
 * Signature: (Ljava/lang/Object;J)J
 */
JNIEXPORT int64_t JNICALL Java_sun_misc_Unsafe_getLong__Ljava_lang_Object_2J(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset)
{
	int64_t *p;
	int64_t  value;

	p = (int64_t *) (((uint8_t *) o) + offset);

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getFloat
 * Signature: (Ljava/lang/Object;J)F
 */
JNIEXPORT float JNICALL Java_sun_misc_Unsafe_getFloat__Ljava_lang_Object_2J(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset)
{
	float *p;
	float  value;

	p = (float *) (((uint8_t *) o) + offset);

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putFloat
 * Signature: (Ljava/lang/Object;JF)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putFloat__Ljava_lang_Object_2JF(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset, float x)
{
	float *p;

	p = (float *) (((uint8_t *) o) + offset);

	*p = x;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getByte
 * Signature: (J)B
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getByte__J(JNIEnv *env, sun_misc_Unsafe *this, int64_t address)
{
	int8_t *p;
	int8_t  value;

	p = (int8_t *) (intptr_t) address;

	value = *p;

	return (int32_t) value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putByte
 * Signature: (JB)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putByte__JB(JNIEnv *env, sun_misc_Unsafe *this, int64_t address, int32_t value)
{
	int8_t *p;

	p = (int8_t *) (intptr_t) address;

	*p = (int8_t) value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getShort
 * Signature: (J)S
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getShort__J(JNIEnv *env, sun_misc_Unsafe *this, int64_t address)
{
	int16_t *p;
	int16_t  value;

	p = (int16_t *) (intptr_t) address;

	value = *p;

	return (int32_t) value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putShort
 * Signature: (JS)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putShort__JS(JNIEnv *env, sun_misc_Unsafe *this, int64_t address, int32_t value)
{
	int16_t *p;

	p = (int16_t *) (intptr_t) address;

	*p = (int16_t) value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getInt
 * Signature: (J)I
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getInt__J(JNIEnv *env, sun_misc_Unsafe *this, int64_t address)
{
	int32_t *p;
	int32_t  value;

	p = (int32_t *) (intptr_t) address;

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putInt
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putInt__JI(JNIEnv *env, struct sun_misc_Unsafe* this, int64_t address, int32_t value)
{
	int32_t *p;

	p = (int32_t *) (intptr_t) address;

	*p = value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getLong
 * Signature: (J)J
 */
JNIEXPORT int64_t JNICALL Java_sun_misc_Unsafe_getLong__J(JNIEnv *env, sun_misc_Unsafe *this, int64_t address)
{
	int64_t *p;
	int64_t  value;

	p = (int64_t *) (intptr_t) address;

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    putLong
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_putLong__JJ(JNIEnv *env, sun_misc_Unsafe *this, int64_t address, int64_t value)
{
	int64_t *p;

	p = (int64_t *) (intptr_t) address;

	*p = value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getFloat
 * Signature: (J)F
 */
JNIEXPORT float JNICALL Java_sun_misc_Unsafe_getFloat__J(JNIEnv *env, sun_misc_Unsafe *this, int64_t address)
{
	float *p;
	float  value;

	p = (float *) (intptr_t) address;

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    objectFieldOffset
 * Signature: (Ljava/lang/reflect/Field;)J
 */
JNIEXPORT int64_t JNICALL Java_sun_misc_Unsafe_objectFieldOffset(JNIEnv *env, sun_misc_Unsafe *this, java_lang_reflect_Field *field)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	LLNI_field_get_cls(field, clazz, c);
	LLNI_field_get_val(field, slot , slot);

	f = &c->fields[slot];

	return (int64_t) f->offset;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    allocateMemory
 * Signature: (J)J
 */
JNIEXPORT int64_t JNICALL Java_sun_misc_Unsafe_allocateMemory(JNIEnv *env, sun_misc_Unsafe *this, int64_t bytes)
{
	size_t  length;
	void   *p;

	length = (size_t) bytes;

	if ((length != (uint64_t) bytes) || (bytes < 0)) {
		exceptions_throw_illegalargumentexception();
		return 0;
	}

	p = MNEW(uint8_t, length);

	return (int64_t) (intptr_t) p;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    setMemory
 * Signature: (Ljava/lang/Object;JJB)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_setMemory(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset, int64_t bytes, int32_t value)
{
	size_t  length;
	void   *p;

	length = (size_t) bytes;

	if ((length != (uint64_t) bytes) || (bytes < 0)) {
		exceptions_throw_illegalargumentexception();
		return;
	}

	/* XXX Missing LLNI: we need to unwrap this object. */

	p = (void *) (((uint8_t *) o) + offset);

	/* XXX Not sure this is correct. */

	MSET(p, value, uint8_t, length);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    freeMemory
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_freeMemory(JNIEnv *env, sun_misc_Unsafe *this, int64_t address)
{
	void *p;

	p = (void *) (intptr_t) address;

	if (p == NULL)
		return;

	/* we pass length 1 to trick the free function */

	MFREE(p, uint8_t, 1);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    staticFieldOffset
 * Signature: (Ljava/lang/reflect/Field;)J
 */
JNIEXPORT int64_t JNICALL Java_sun_misc_Unsafe_staticFieldOffset(JNIEnv *env, sun_misc_Unsafe *this, java_lang_reflect_Field *field)
{
	classinfo *c;
	fieldinfo *f;
	int32_t    slot;

	LLNI_field_get_cls(field, clazz, c);
	LLNI_field_get_val(field, slot , slot);
	f = &(c->fields[slot]);

	return (int64_t) (intptr_t) f->value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    staticFieldBase
 * Signature: (Ljava/lang/reflect/Field;)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_sun_misc_Unsafe_staticFieldBase(JNIEnv *env, sun_misc_Unsafe *this, java_lang_reflect_Field *f)
{
	/* In CACAO we return the absolute address in staticFieldOffset. */

	return NULL;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    ensureClassInitialized
 * Signature: (Ljava/lang/Class;)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_ensureClassInitialized(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Class *class)
{
	classinfo *c;

	c = LLNI_classinfo_unwrap(class);

	if (!(c->state & CLASS_INITIALIZED))
		initialize_class(c);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    arrayBaseOffset
 * Signature: (Ljava/lang/Class;)I
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_arrayBaseOffset(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Class *arrayClass)
{
	classinfo       *c;
	arraydescriptor *ad;

	c  = LLNI_classinfo_unwrap(arrayClass);
	ad = c->vftbl->arraydesc;

	if (ad == NULL) {
		/* XXX does that exception exist? */
		exceptions_throw_internalerror("java/lang/InvalidClassException");
		return 0;
	}

	return ad->dataoffset;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    arrayIndexScale
 * Signature: (Ljava/lang/Class;)I
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_arrayIndexScale(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Class *arrayClass)
{
	classinfo       *c;
	arraydescriptor *ad;

	c  = LLNI_classinfo_unwrap(arrayClass);
	ad = c->vftbl->arraydesc;

	if (ad == NULL) {
		/* XXX does that exception exist? */
		exceptions_throw_internalerror("java/lang/InvalidClassException");
		return 0;
	}

	return ad->componentsize;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    addressSize
 * Signature: ()I
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_addressSize(JNIEnv *env, sun_misc_Unsafe *this)
{
	return SIZEOF_VOID_P;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    pageSize
 * Signature: ()I
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_pageSize(JNIEnv *env, sun_misc_Unsafe *this)
{
	int sz;

	sz = getpagesize();

	return sz;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    defineClass
 * Signature: (Ljava/lang/String;[BIILjava/lang/ClassLoader;Ljava/security/ProtectionDomain;)Ljava/lang/Class;
 */
JNIEXPORT java_lang_Class* JNICALL Java_sun_misc_Unsafe_defineClass__Ljava_lang_String_2_3BIILjava_lang_ClassLoader_2Ljava_security_ProtectionDomain_2(JNIEnv *env, sun_misc_Unsafe *this, java_lang_String *name, java_handle_bytearray_t *b, int32_t off, int32_t len, java_lang_ClassLoader *loader, java_security_ProtectionDomain *protectionDomain)
{
	classloader     *cl;
	utf             *utfname;
	classinfo       *c;
	java_lang_Class *o;

	cl = loader_hashtable_classloader_add((java_handle_t *) loader);

	/* check if data was passed */

	if (b == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	/* check the indexes passed */

	if ((off < 0) || (len < 0) || ((off + len) > LLNI_array_size(b))) {
		exceptions_throw_arrayindexoutofboundsexception();
		return NULL;
	}

	if (name != NULL) {
		/* convert '.' to '/' in java string */

		utfname = javastring_toutf((java_handle_t *) name, true);
	} 
	else {
		utfname = NULL;
	}

	/* define the class */

	c = class_define(utfname, cl, len, (const uint8_t *) &b->data[off],
					 protectionDomain);

	if (c == NULL)
		return NULL;

	/* for convenience */

	o = LLNI_classinfo_wrap(c);

#if defined(WITH_CLASSPATH_GNU)
	/* set ProtectionDomain */

	LLNI_field_set_ref(o, pd, protectionDomain);
#endif

	return o;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    allocateInstance
 * Signature: (Ljava/lang/Class;)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_sun_misc_Unsafe_allocateInstance(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Class *cls)
{
	classinfo     *c;
	java_object_t *o;

	c = LLNI_classinfo_unwrap(cls);

	o = builtin_new(c);

	return (java_lang_Object *) o;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    throwException
 * Signature: (Ljava/lang/Throwable;)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_throwException(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Throwable *ee)
{
	java_handle_t *o;

	o = (java_handle_t *) ee;

	exceptions_set_exception(o);
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    compareAndSwapObject
 * Signature: (Ljava/lang/Object;JLjava/lang/Object;Ljava/lang/Object;)Z
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_compareAndSwapObject(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset, java_lang_Object *expected, java_lang_Object *x)
{
	void **p;
	void  *value;

	p = (void **) (((uint8_t *) o) + offset);

	/* XXX this should be atomic */

	value = *p;

	if (value == expected) {
		*p = x;

		return true;
	}

	return false;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    compareAndSwapInt
 * Signature: (Ljava/lang/Object;JII)Z
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_compareAndSwapInt(JNIEnv *env, sun_misc_Unsafe* this, java_lang_Object* obj, int64_t offset, int32_t expect, int32_t update)
{
	int32_t *p;
	int32_t  value;

	p = (int32_t *) (((uint8_t *) obj) + offset);

	/* XXX this should be atomic */

	value = *p;

	if (value == expect) {
		*p = update;

		return true;
	}

	return false;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    compareAndSwapLong
 * Signature: (Ljava/lang/Object;JJJ)Z
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_compareAndSwapLong(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset, int64_t expected, int64_t x)
{
	int64_t *p;
	int64_t  value;

	p = (int64_t *) (((uint8_t *) o) + offset);

	/* XXX this should be atomic */

	value = *p;

	if (value == expected) {
		*p = x;

		return true;
	}

	return false;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getObjectVolatile
 * Signature: (Ljava/lang/Object;J)Ljava/lang/Object;
 */
JNIEXPORT java_lang_Object* JNICALL Java_sun_misc_Unsafe_getObjectVolatile(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset)
{
	volatile void **p;
	volatile void  *value;

	p = (volatile void **) (((uint8_t *) o) + offset);

	value = *p;

	return (java_lang_Object *) value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getIntVolatile
 * Signature: (Ljava/lang/Object;J)I
 */
JNIEXPORT int32_t JNICALL Java_sun_misc_Unsafe_getIntVolatile(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset)
{
	volatile int32_t *p;
	volatile int32_t  value;

	p = (volatile int32_t *) (((uint8_t *) o) + offset);

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    getLongVolatile
 * Signature: (Ljava/lang/Object;J)J
 */
JNIEXPORT int64_t JNICALL Java_sun_misc_Unsafe_getLongVolatile(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *o, int64_t offset)
{
	volatile int64_t *p;
	volatile int64_t  value;

	p = (volatile int64_t *) (((uint8_t *) o) + offset);

	value = *p;

	return value;
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    unpark
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_unpark(JNIEnv *env, sun_misc_Unsafe *this, java_lang_Object *thread)
{
	/* XXX IMPLEMENT ME */
}


/*
 * Class:     sun/misc/Unsafe
 * Method:    park
 * Signature: (ZJ)V
 */
JNIEXPORT void JNICALL Java_sun_misc_Unsafe_park(JNIEnv *env, sun_misc_Unsafe *this, int32_t isAbsolute, int64_t time)
{
	/* XXX IMPLEMENT ME */
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
