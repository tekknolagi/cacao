/* src/native/native.hpp - native library support

   Copyright (C) 1996-2005, 2006, 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
   Copyright (C) 2008 Theobroma Systems Ltd.

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


#ifndef _NATIVE_HPP
#define _NATIVE_HPP

#include <stdint.h>

#ifdef __cplusplus
#include <map>
#include <set>
#endif

#include "native/jni.hpp"

#include "vm/class.h"
#include "vm/global.h"
#include "vm/loader.hpp"
#include "vm/method.h"
#include "vm/os.hpp"
#include "vm/utf8.h"


/* defines ********************************************************************/

#define NATIVE_METHODS_COUNT    sizeof(methods) / sizeof(JNINativeMethod)


#define NATIVE_LIBRARY_PREFIX     "lib"

#if defined(__DARWIN__)
# define NATIVE_LIBRARY_SUFFIX    ".dylib"
#else
# define NATIVE_LIBRARY_SUFFIX    ".so"
#endif


#ifdef __cplusplus

#if defined(ENABLE_DL)
/**
 * Represents a native library.
 */
class NativeLibrary {
private:
	utf*           _filename;    ///< Name of the native library.
	classloader_t* _classloader; ///< Defining classloader.
	void*          _handle;      ///< Filesystem handle.

public:
	NativeLibrary(utf* filename, classloader_t* classloader = 0, void* handle = 0) : _filename(filename), _classloader(classloader), _handle(handle) {}
	NativeLibrary(void* handle) : _filename(0), _classloader(0), _handle(handle) {}

	inline classloader_t* get_classloader() const { return _classloader; }
	inline utf*           get_filename   () const { return _filename; }
	inline void*          get_handle     () const { return _handle; }

	void* open();
	void  close();
	bool  load(JNIEnv* env);
	bool  is_loaded();
	void* resolve_symbol(utf* symbolname) const;
};


/**
 * Table containing all loaded native libraries.
 */
class NativeLibraries {
private:
	Mutex _mutex; ///< Mutex to make the container thread-safe.
	typedef std::multimap<classloader_t*, NativeLibrary> MAP;
	MAP _libraries;

private:
	// Comparator class.
	class comparator : public std::binary_function<std::pair<classloader_t*, NativeLibrary>, utf*, bool> {
	public:
		bool operator() (std::pair<classloader_t*, NativeLibrary> args, const utf* filename) const
		{
			return (args.second.get_filename() == filename);
		}
	};

public:
	void  add(NativeLibrary& library);
	bool  is_loaded(NativeLibrary& library);
	void* resolve_symbol(utf* symbolname, classloader_t* classloader);
};
#endif


/**
 * Represents a native method.
 */
class NativeMethod {
private:
	utf*  _classname;  ///< Class name.
	utf*  _name;       ///< Method name.
	utf*  _descriptor; ///< Method signature.
	void* _function;   ///< Pointer to the native function.

	friend bool operator< (const NativeMethod& first, const NativeMethod& second);

public:
	NativeMethod(utf* classname, utf* name, utf* signature, void* function) : _classname(classname), _name(name), _descriptor(signature), _function(function) {}
	NativeMethod(methodinfo* m) : _classname(m->clazz->name), _name(m->name), _descriptor(m->descriptor), _function(0) {}

	inline void* get_function() const { return _function; }
};


/**
 * Table containing all native methods registered with the VM.
 */
class NativeMethods {
private:
	Mutex _mutex;
	std::set<NativeMethod> _methods;

private:
	// Comparator class.
	class comparator : public std::binary_function<std::pair<classloader_t*, NativeLibrary>, utf*, bool> {
	public:
		bool operator() (std::pair<classloader_t*, NativeLibrary> args, const utf* filename) const
		{
			return (args.second.get_filename() == filename);
		}
	};
	
public:
	void  register_methods(utf* classname, const JNINativeMethod* methods, size_t count);
	void* resolve_method(methodinfo* m);
	void* find_registered_method(methodinfo* m);
};

#endif

/* function prototypes ********************************************************/

#ifdef __cplusplus
extern "C" {
#endif

java_handle_t *native_new_and_init(classinfo *c);
java_handle_t *native_new_and_init_string(classinfo *c, java_handle_t *s);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _NATIVE_HPP


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
