/* src/vm/string.cpp - java.lang.String related functions

   Copyright (C) 1996-2013
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

#include "vm/string.hpp"

#include "vm/array.hpp"
#include "vm/exceptions.hpp"
#include "vm/globals.hpp"
#include "vm/javaobjects.hpp"
#include "vm/options.hpp"
#include "vm/statistics.hpp"

#include "toolbox/intern_table.hpp"
#include "toolbox/logging.hpp"
#include "toolbox/OStream.hpp"
#include "toolbox/utf_utils.hpp"

using namespace cacao;

STAT_DECLARE_VAR(int,size_string,0)

//****************************************************************************//
//*****          GLOBAL JAVA/LANG/STRING INTERN TABLE                    *****//
//****************************************************************************//

struct JavaStringHash {
	uint32_t operator()(JavaString str) const {
		const u2 *cs = str.begin();
		size_t    sz = str.size();

		return utf_hashkey((const char*) cs, sz);
	}

	/// The hashkey is computed from the utf-text by using up to 8
	/// characters.  For utf-symbols longer than 15 characters 3 characters
	/// are taken from the beginning and the end, 2 characters are taken
	/// from the middle.
	static u4 utf_hashkey(const char *text, u4 length) {
#define nbs(val) ((u4) *(++text) << val) // get next byte, left shift by val
#define fbs(val) ((u4) *(  text) << val) // get first byte, left shift by val

		const char *start_pos = text;  // pointer to utf text
		u4          a;

	switch (length) {
	case 0: // empty string */
		return 0;

	case 1:
		return fbs(0);
	case 2:
		// fbs(0) ^ nbs(3);
		a = fbs(0);
		a ^= nbs(3);
		return a;
	case 3:
		// fbs(0) ^ nbs(3) ^ nbs(5);
		a = fbs(0);
		a ^= nbs(3);
		a ^= nbs(5);
		return a;
	case 4:
		// fbs(0) ^ nbs(2) ^ nbs(4) ^ nbs(6);
		a = fbs(0);
		a ^= nbs(2);
		a ^= nbs(4);
		a ^= nbs(6);
		return a;
	case 5:
		// fbs(0) ^ nbs(2) ^ nbs(3) ^ nbs(4) ^ nbs(6);
		a = fbs(0);
		a ^= nbs(2);
		a ^= nbs(3);
		a ^= nbs(4);
		a ^= nbs(6);
		return a;
	case 6:
		// fbs(0) ^ nbs(1) ^ nbs(2) ^ nbs(3) ^ nbs(5) ^ nbs(6);
		a = fbs(0);
		a ^= nbs(1);
		a ^= nbs(2);
		a ^= nbs(3);
		a ^= nbs(5);
		a ^= nbs(6);
		return a;
	case 7:
		// fbs(0) ^ nbs(1) ^ nbs(2) ^ nbs(3) ^ nbs(4) ^ nbs(5) ^ nbs(6);
		a = fbs(0);
		a ^= nbs(1);
		a ^= nbs(2);
		a ^= nbs(3);
		a ^= nbs(4);
		a ^= nbs(5);
		a ^= nbs(6);
		return a;
	case 8:
		// fbs(0) ^ nbs(1) ^ nbs(2) ^ nbs(3) ^ nbs(4) ^ nbs(5) ^ nbs(6) ^ nbs(7);
		a = fbs(0);
		a ^= nbs(1);
		a ^= nbs(2);
		a ^= nbs(3);
		a ^= nbs(4);
		a ^= nbs(5);
		a ^= nbs(6);
		a ^= nbs(7);
		return a;

	case 9:
		a = fbs(0);
		a ^= nbs(1);
		a ^= nbs(2);
		text++;
		// a ^ nbs(4) ^ nbs(5) ^ nbs(6) ^ nbs(7) ^ nbs(8);
		a ^= nbs(4);
		a ^= nbs(5);
		a ^= nbs(6);
		a ^= nbs(7);
		a ^= nbs(8);
		return a;

	case 10:
		a = fbs(0);
		text++;
		a ^= nbs(2);
		a ^= nbs(3);
		a ^= nbs(4);
		text++;
		// a ^ nbs(6) ^ nbs(7) ^ nbs(8) ^ nbs(9);
		a ^= nbs(6);
		a ^= nbs(7);
		a ^= nbs(8);
		a ^= nbs(9);
		return a;

	case 11:
		a = fbs(0);
		text++;
		a ^= nbs(2);
		a ^= nbs(3);
		a ^= nbs(4);
		text++;
		// a ^ nbs(6) ^ nbs(7) ^ nbs(8) ^ nbs(9) ^ nbs(10);
		a ^= nbs(6);
		a ^= nbs(7);
		a ^= nbs(8);
		a ^= nbs(9);
		a ^= nbs(10);
		return a;

	case 12:
		a = fbs(0);
		text += 2;
		a ^= nbs(2);
		a ^= nbs(3);
		text++;
		a ^= nbs(5);
		a ^= nbs(6);
		a ^= nbs(7);
		text++;
		// a ^ nbs(9) ^ nbs(10);
		a ^= nbs(9);
		a ^= nbs(10);
		return a;

	case 13:
		a = fbs(0);
		a ^= nbs(1);
		text++;
		a ^= nbs(3);
		a ^= nbs(4);
		text += 2;
		a ^= nbs(7);
		a ^= nbs(8);
		text += 2;
		// a ^ nbs(9) ^ nbs(10);
		a ^= nbs(9);
		a ^= nbs(10);
		return a;

	case 14:
		a = fbs(0);
		text += 2;
		a ^= nbs(3);
		a ^= nbs(4);
		text += 2;
		a ^= nbs(7);
		a ^= nbs(8);
		text += 2;
		// a ^ nbs(9) ^ nbs(10) ^ nbs(11);
		a ^= nbs(9);
		a ^= nbs(10);
		a ^= nbs(11);
		return a;

	case 15:
		a = fbs(0);
		text += 2;
		a ^= nbs(3);
		a ^= nbs(4);
		text += 2;
		a ^= nbs(7);
		a ^= nbs(8);
		text += 2;
		// a ^ nbs(9) ^ nbs(10) ^ nbs(11);
		a ^= nbs(9);
		a ^= nbs(10);
		a ^= nbs(11);
		return a;

	default:
       	// 3 characters from beginning
		a = fbs(0);
		text += 2;
		a ^= nbs(3);
		a ^= nbs(4);

		// 2 characters from middle
		text = start_pos + (length / 2);
		a ^= fbs(5);
		text += 2;
		a ^= nbs(6);

		// 3 characters from end
		text = start_pos + length - 4;

		a ^= fbs(7);
		text++;

		// a ^ nbs(10) ^ nbs(11);
		a ^= nbs(10);
		a ^= nbs(11);
		return a;
#undef fbs
#undef nbs
		}
	}
};

struct JavaStringEq {
	bool operator()(JavaString a, JavaString b) const {
		size_t a_sz = a.size();
		size_t b_sz = b.size();

		if (a_sz != b_sz) return false;

		const char *a_cs = (const char*) a.begin();
		const char *b_cs = (const char*) b.begin();

		return memcmp(a_cs, b_cs, a_sz * sizeof(u2)) == 0;
	}
};

typedef InternTable<JavaString, JavaStringHash, JavaStringEq, 1>
        JavaStringInternTable;

static JavaStringInternTable* intern_table = NULL;

//****************************************************************************//
//*****          JAVA STRING SUBSYSTEM INITIALIZATION                    *****//
//****************************************************************************//

/* JavaString::initialize ******************************************************

	Initialize string subsystem

*******************************************************************************/

void JavaString::initialize() {
	assert(!JavaString::is_initialized());

	TRACESUBSYSTEMINITIALIZATION("string_init");

	intern_table = new JavaStringInternTable(4096);
}

/* JavaString::is_initialized **************************************************

	Check is string subsystem is initialized

*******************************************************************************/

bool JavaString::is_initialized() {
	return intern_table != NULL;
}

//****************************************************************************//
//*****          JAVA STRING CONSTRUCTORS                                *****//
//****************************************************************************//

/* makeJavaString **************************************************************

	Allocate a new java/lang/String object, fill it with string content
	and set it's fields.

	If input chars is NULL a NullPointerException is raised.

	PARAMETERS:
		src ........ content of new string is copied from this pointer
		src_size ... number of chars in src
		dst_size ... numbers of chars in new string
	TEMPLATE PARAMETERS:
		Src ........... The char type used to initialize the String (u1 or u2)
		Allocator ..... An allocation function, it allocats and initializes a new
		                java/lang/String with a given size. The contents of the
		                strings char[] can be undefined.
		Initializer ... A function that copies the given chars into the strings
		                private char[].

*******************************************************************************/

template<typename Src, typename Allocator, typename Initializer>
static inline java_handle_t* makeJavaString(const Src *src, size_t src_size, size_t dst_size,
                                            Allocator alloc, Initializer init) {
	if (src == NULL) {
		exceptions_throw_nullpointerexception();
		return NULL;
	}

	assert(src_size >= 0);
	assert(dst_size >= 0);

	// allocate new java/lang/String

	JavaString str = alloc(dst_size);

	if (str == NULL) return NULL;

	// copy text

	u2 *dst = (u2*) str.begin();

	bool success = init(src, src_size, dst);

	if (!success) return NULL;

	return str;
}

//***** ALLOCATORS

static inline JavaString allocate_with_GC(size_t size) {
	java_handle_t *h = builtin_new(class_java_lang_String);

	if (h == NULL) return NULL;

	CharArray ca(size);

	if (ca.is_null()) return NULL;

	java_lang_String::set_fields(h, ca.get_handle());

	return h;
}

static inline JavaString allocate_on_system_heap(size_t size) {
	// allocate string
	java_handle_t *h = (java_object_t*) MNEW(uint8_t, class_java_lang_String->instancesize);
	if (h == NULL) return NULL;

	// set string VTABLE and lockword
	Lockword(h->lockword).init();
	h->vftbl = class_java_lang_String->vftbl;

	// allocate array
	java_chararray_t *a = (java_chararray_t*) MNEW(uint8_t, sizeof(java_chararray_t) + sizeof(u2) * size);
	if (a == NULL) return NULL;

	// set array VTABLE, lockword and length
	a->header.objheader.vftbl = Primitive::get_arrayclass_by_type(ARRAYTYPE_CHAR)->vftbl;
	Lockword(a->header.objheader.lockword).init();
	a->header.size            = size;

	java_lang_String::set_fields(h, (java_handle_chararray_t*) a);

	STATISTICS(size_string += sizeof(class_java_lang_String->instancesize));

	return h;
}

static inline void free_from_system_heap(JavaString str) {
	MFREE(java_lang_String::get_value(str), uint8_t, sizeof(java_chararray_t) + sizeof(u2) * str.size());
	MFREE(str,                              uint8_t, class_java_lang_String->instancesize);
}

// ***** COPY CONTENTS INTO STRING

template<typename Iterator>
static inline bool init_from_utf8(const char *src, size_t src_size, u2 *dst) {
	return utf8::decode(Iterator(src), Iterator(src + src_size), dst);
}

static inline bool init_from_utf16(const u2 *src, size_t src_size, u2 *dst) {
	memcpy(dst, src, sizeof(u2) * src_size);
	return true;
}


/* JavaString::from_utf8 *******************************************************

	Create a new java/lang/String filled with text decoded from an UTF-8 string.
	Returns NULL on error.

*******************************************************************************/

JavaString JavaString::from_utf8(Utf8String u) {
	return makeJavaString(u.begin(), u.size(), u.utf16_size(),
	                      allocate_with_GC, init_from_utf8<const char*>);
}

JavaString JavaString::from_utf8(const char *cs, size_t sz) {
	return makeJavaString(cs, sz, utf8::num_codepoints(cs, sz),
	                      allocate_with_GC, init_from_utf8<const char*>);
}

/* JavaString::from_utf8_slash_to_dot ******************************************

	Create a new java/lang/String filled with text decoded from an UTF-8 string.
	Replaces '/' with '.'.

	NOTE:
		If the input is not valid UTF-8 the process aborts!

*******************************************************************************/

JavaString JavaString::from_utf8_slash_to_dot(Utf8String u) {
	return makeJavaString(u.begin(), u.size(), u.utf16_size(),
	                      allocate_with_GC, init_from_utf8<utf8::SlashToDot>);
}

/* JavaString::from_utf8_dot_to_slash ******************************************

	Create a new java/lang/String filled with text decoded from an UTF-8 string.
	Replaces '.' with '/'.

	NOTE:
		If the input is not valid UTF-8 the process aborts!

*******************************************************************************/

JavaString JavaString::from_utf8_dot_to_slash(Utf8String u) {
	return makeJavaString(u.begin(), u.size(), u.utf16_size(),
	                      allocate_with_GC, init_from_utf8<utf8::DotToSlash>);
}

/* JavaString::literal *********************************************************

	Create and intern a java/lang/String filled with text decoded from an UTF-8
	string.

	NOTE:
		because the intern table is allocated on the system heap the GC
		can't see it and thus interned strings must also be allocated on
		the system heap.

*******************************************************************************/

JavaString JavaString::literal(Utf8String u) {
	JavaString str = makeJavaString(u.begin(), u.size(), u.utf16_size(),
	                                allocate_on_system_heap, init_from_utf8<const char*>);


	JavaString intern_str = intern_table->intern(str);

	if (intern_str != str) {
		// str was already present, free it.
		free_from_system_heap(str);
	}

	return intern_str;
}

/* JavaString:from_utf16 *******************************************************

	Create a new java/lang/String filled with text copied from an UTF-16 string.
	Returns NULL on error.

*******************************************************************************/

JavaString JavaString::from_utf16(const u2 *cs, size_t sz) {
	return makeJavaString(cs, sz, sz,
                          allocate_with_GC, init_from_utf16);
}

/* JavaString:from_utf16 *******************************************************

	Creates a new java/lang/String with a given char[]

	WARNING: the char[] is not copied or validated,
	         you must make sure it is never changed.

*******************************************************************************/

#ifdef WITH_JAVA_RUNTIME_LIBRARY_GNU_CLASSPATH

JavaString JavaString::from_array(java_handle_t *array, int32_t count, int32_t offset) {
	java_handle_t *str = builtin_new(class_java_lang_String);
	if (!str)
		return NULL;

	java_lang_String jstr(str);

	jstr.set_value((java_handle_chararray_t*) array);
	jstr.set_count (count);
	jstr.set_offset(offset);

	return str;
}

#endif

/* JavaString::intern **********************************************************

	intern string in global intern table

	NOTE:
		because the intern table is allocated on the system heap the GC
		can't see it and thus interned strings must also be allocated on
		the system heap.

*******************************************************************************/

struct LazyStringCopy {
	LazyStringCopy(JavaString src) : src(src) {}

	operator JavaString() const {
		return makeJavaString(src.begin(), src.size(), src.size(),
	                          allocate_on_system_heap, init_from_utf16);
	}

	JavaString src;
};

JavaString JavaString::intern() const {
	return intern_table->intern(*this, LazyStringCopy(*this));
}

//****************************************************************************//
//*****          JAVA STRING ACCESSORS                                   *****//
//****************************************************************************//

/* JavaString::begin ***********************************************************

	Get the utf-16 contents of string

*******************************************************************************/

const u2* JavaString::begin() const {
	assert(str);

	java_handle_chararray_t *array = java_lang_String::get_value(str);

	if (array == NULL) {
		// this can only happen if the string has been allocated by java code
		// and <init> has not been called on it yet
		return NULL;
	}

	CharArray ca(array);

	int32_t   offset = java_lang_String::get_offset(str);
	uint16_t* ptr    = ca.get_raw_data_ptr();

	return ptr + offset;
}

const u2* JavaString::end() const {
	const u2 *ptr = begin();

	return ptr ? ptr + size() : NULL;
}


/* JavaString::size ************************************************************

	Get the number of utf-16 characters in string

*******************************************************************************/

size_t JavaString::size() const {
	assert(str);

	return java_lang_String::get_count(str);
}

/* JavaString::utf8_size *******************************************************

	Get the number of bytes this string would need in utf-8 encoding

*******************************************************************************/

size_t JavaString::utf8_size() const {
	assert(str);

	return utf8::num_bytes(begin(), size());
}

//****************************************************************************//
//*****          JAVA STRING CONVERSIONS                                 *****//
//****************************************************************************//

/* JavaString::to_chars ********************************************************

	Decodes java/lang/String into newly allocated string (debugging)

	NOTE:
		You must free the string allocated yourself with MFREE

*******************************************************************************/

char *JavaString::to_chars() const {
	if (str == NULL) return MNEW(char, 1); // memory is zero initialized

	size_t sz = size();

	const uint16_t *src = begin();
	const uint16_t *end = src + sz;

	char *buf = MNEW(char, sz + 1);
	char *dst = buf;

	while (src != end) *dst++ = *src++;

	*dst = '\0';

	return buf;
}

/* JavaString::to_utf8() *******************************************************

	make utf symbol from java.lang.String

*******************************************************************************/

Utf8String JavaString::to_utf8() const {
	if (str == NULL) return utf8::empty;

	return Utf8String::from_utf16(begin(), size());
}

/* JavaString::to_utf8_dot_to_slash() ******************************************

	make utf symbol from java.lang.String
	replace '/' with '.'

*******************************************************************************/

Utf8String JavaString::to_utf8_dot_to_slash() const {
	if (str == NULL) return utf8::empty;

	return Utf8String::from_utf16_dot_to_slash(begin(), size());
}

//****************************************************************************//
//*****          JAVA STRING IO                                          *****//
//****************************************************************************//

/* JavaString::fprint **********************************************************

   Print the given Java string to the given stream.

*******************************************************************************/

void JavaString::fprint(FILE *stream) const
{
	const uint16_t* cs = begin();
	size_t          sz = size();

	for (size_t i = 0; i < sz; i++) {
		char c = cs[i];

		fputc(c, stream);
	}
}

void JavaString::fprint_printable_ascii(FILE *stream) const
{
	const uint16_t* cs = begin();
	size_t          sz = size();

	for (size_t i = 0; i < sz; i++) {
		char c = cs[i];

		c = (c >= 32 && (unsigned char)c <= 127) ? c : '?';

		fputc(c, stream);
	}
}

OStream& operator<<(OStream& os, JavaString js) {
	const u2 *cs = js.begin();

	if (cs == NULL) {
		// string has been allocated by java code
		// but <init> has not been called on it yet.
		return os << "<uninitialized string>";
	} else {
		os << '"';

		for (const u2 *end = js.end(); cs != end; ++cs) {
			os << ((char) *cs);
		}

		os << '"';

		return os;
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
