# Copyright (c) 1999-2001 by Red Hat, Inc. All rights reserved.
#
# THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
# OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
#
# Permission is hereby granted to use or copy this program
# for any purpose,  provided the above notices are retained on all copies.
# Permission to modify the code and to distribute modified code is granted,
# provided the above notices are retained, and a notice that the code was
# modified is included with the above copyright notice.

dnl Process this file with autoconf to produce configure.

# Initialization
AC_INIT(gc,7.4.2,bdwgc@lists.opendylan.org)
    ## version must conform to [0-9]+[.][0-9]+[.][0-9]+
AC_CONFIG_SRCDIR(gcj_mlc.c)
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AC_PREREQ(2.61)
GC_SET_VERSION
AM_INIT_AUTOMAKE([foreign dist-bzip2 nostdinc])
AC_CONFIG_HEADERS([include/config.h])
AM_MAINTAINER_MODE

AC_SUBST(PACKAGE)
AC_SUBST(GC_VERSION)

AM_PROG_CC_C_O
AC_PROG_CXX
AM_PROG_AS
AC_PROG_INSTALL
LT_INIT
# Note: If Autoconf reports that LIBTOOL (or AC_ENABLE_SHARED, or
# AC_PROG_LIBTOOL) is undefined, Libtool installation should be checked.

# Special CFLAGS to use when building
gc_cflags=""

# gc_use_mmap   Set to "yes" on platforms where mmap should be used instead
#               of sbrk. This will define USE_MMAP.
gc_use_mmap=""

# We should set -fexceptions if we are using gcc and might be used
# inside something like gcj.  This is the zeroth approximation:
if test :"$GCC": = :yes: ; then
    gc_cflags="${gc_cflags} -fexceptions"
else
    case "$host" in
        hppa*-*-hpux* )
        if test :$GCC: != :"yes": ; then
            gc_cflags="${gc_flags} +ESdbgasm"
        fi
        # :TODO: actaully we should check using Autoconf if
        #     the compiler supports this option.
        ;;
    esac
fi

case "${host}" in
  *-linux*)
    # FIXME: This seems to be no longer needed as configured in gcconfig.h
    #gc_use_mmap=yes
    ;;
esac

#   target_optspace     --enable-target-optspace ("yes", "no", "")
case "${target_optspace}:${host}" in
  yes:*)
    gc_cflags="${gc_cflags} -Os"
    ;;
  :m32r-* | :d10v-* | :d30v-*)
    gc_cflags="${gc_cflags} -Os"
    ;;
  no:* | :*)
    # Nothing.
    ;;
esac

# Set any host dependent compiler flags.
case "${host}" in
  mips-tx39-*|mipstx39-unknown-*)
        gc_cflags="${gc_cflags} -G 0"
        ;;
  *)
        ;;
esac


GC_CFLAGS=${gc_cflags}
AC_SUBST(GC_CFLAGS)

AC_ARG_ENABLE(boehm-threads,
  [AC_HELP_STRING([--enable-boehm-threads=TYPE], [choose threading package])],
  THREADS=$enableval,
  [ AC_MSG_CHECKING([for thread model used by GCC])
    THREADS=`$CC -v 2>&1 | sed -n 's/^Thread model: //p'`
    if test -z "$THREADS"; then
      THREADS=no
    fi
    if test "$THREADS" = "posix"; then
      case "$host" in
        *-*-mingw*)
          # Adjust thread model if cross-compiling for MinGW.
          THREADS=win32
          ;;
      esac
    fi
    AC_MSG_RESULT([$THREADS]) ])

AC_ARG_ENABLE(parallel-mark,
   [AC_HELP_STRING([--enable-parallel-mark],
        [parallelize marking and free list construction])],
   [case "$THREADS" in
      no | none | single)
        if test "${enable_parallel_mark}" != no; then
          AC_MSG_ERROR([Parallel mark requires --enable-boehm-threads=x spec])
        fi
        ;;
    esac ]
)

AC_ARG_ENABLE(cplusplus,
    [AC_HELP_STRING([--enable-cplusplus], [install C++ support])])

dnl Features which may be selected in the following thread-detection switch.
AH_TEMPLATE([PARALLEL_MARK], [Define to enable parallel marking.])
AH_TEMPLATE([THREAD_LOCAL_ALLOC],
            [Define to enable thread-local allocation optimization.])
AH_TEMPLATE([USE_COMPILER_TLS],
            [Define to use of compiler-support for thread-local variables.])

dnl Thread selection macros.
AH_TEMPLATE([GC_THREADS],           [Define to support platform-specific
                                     threads.])
AH_TEMPLATE([GC_AIX_THREADS],       [Define to support IBM AIX threads.])
AH_TEMPLATE([GC_DARWIN_THREADS],    [Define to support Darwin pthreads.])
AH_TEMPLATE([GC_FREEBSD_THREADS],   [Define to support FreeBSD pthreads.])
AH_TEMPLATE([GC_GNU_THREADS],       [Define to support GNU pthreads.])
AH_TEMPLATE([GC_HPUX_THREADS],      [Define to support HP/UX 11 pthreads.])
AH_TEMPLATE([GC_IRIX_THREADS],      [Define to support Irix pthreads.])
AH_TEMPLATE([GC_LINUX_THREADS],     [Define to support pthreads on Linux.])
AH_TEMPLATE([GC_NETBSD_THREADS],    [Define to support NetBSD pthreads.])
AH_TEMPLATE([GC_OPENBSD_THREADS],   [Define to support OpenBSD pthreads.])
AH_TEMPLATE([GC_OSF1_THREADS],      [Define to support Tru64 pthreads.])
AH_TEMPLATE([GC_SOLARIS_THREADS],   [Define to support Solaris pthreads.])
AH_TEMPLATE([GC_WIN32_THREADS],     [Define to support Win32 threads.])
AH_TEMPLATE([GC_WIN32_PTHREADS],    [Define to support win32-pthreads.])
AH_TEMPLATE([GC_RTEMS_PTHREADS],    [Define to support rtems-pthreads.])

dnl System header feature requests.
AH_TEMPLATE([_POSIX_C_SOURCE], [The POSIX feature macro.])
AH_TEMPLATE([_PTHREADS], [Indicates the use of pthreads (NetBSD).])

dnl Win32-specific API usage controls.
AH_TEMPLATE([GC_UNDERSCORE_STDCALL],
        [Explicitly prefix exported/imported WINAPI symbols with '_'.])
AH_TEMPLATE([UNICODE],
        [Use Unicode (W) variant of Win32 API instead of ASCII (A) one.])

dnl GC API symbols export control.
AH_TEMPLATE([GC_DLL],
        [Define to build dynamic libraries with only API symbols exposed.])

dnl Check for a flavor of supported inline keyword.
AC_C_INLINE

THREADDLLIBS=
need_atomic_ops_asm=false
## Libraries needed to support dynamic loading and/or threads.
case "$THREADS" in
 no | none | single)
    THREADS=none
    ;;
 posix | pthreads)
    THREADS=posix
    AC_CHECK_LIB(pthread, pthread_self, THREADDLLIBS="-lpthread",,)
    case "$host" in
     x86-*-linux* | ia64-*-linux* | i586-*-linux* | i686-*-linux* \
     | x86_64-*-linux* | alpha-*-linux* | powerpc*-*-linux* | sparc*-*-linux*)
        AC_DEFINE(GC_LINUX_THREADS)
        AC_DEFINE(_REENTRANT)
        if test "${enable_parallel_mark}" != no; then
          AC_DEFINE(PARALLEL_MARK)
        fi
        AC_DEFINE(THREAD_LOCAL_ALLOC)
        AC_MSG_WARN("Explicit GC_INIT() calls may be required.");
        ;;
     *-*-linux*)
        AC_DEFINE(GC_LINUX_THREADS)
        AC_DEFINE(_REENTRANT)
        ;;
     *-*-aix*)
        AC_DEFINE(GC_AIX_THREADS)
        AC_DEFINE(_REENTRANT)
        ;;
     *-*-hpux11*)
        AC_MSG_WARN("Only HP/UX 11 POSIX threads are supported.")
        AC_DEFINE(GC_HPUX_THREADS)
        AC_DEFINE(_POSIX_C_SOURCE,199506L)
        if test "${enable_parallel_mark}" = yes; then
          AC_DEFINE(PARALLEL_MARK)
        fi
        AC_DEFINE(THREAD_LOCAL_ALLOC)
        AC_MSG_WARN("Explicit GC_INIT() calls may be required.");
        THREADDLLIBS="-lpthread -lrt"
        # HPUX needs REENTRANT for the _r calls.
        AC_DEFINE(_REENTRANT, 1, [Required define if using POSIX threads.])
        ;;
     *-*-hpux10*)
        AC_MSG_WARN("Only HP-UX 11 POSIX threads are supported.")
        ;;
     *-*-openbsd*)
        AC_DEFINE(GC_OPENBSD_THREADS)
        THREADDLLIBS=-pthread
        AM_CFLAGS="$AM_CFLAGS -pthread"
        ;;
     *-*-freebsd*)
        AC_MSG_WARN("FreeBSD does not yet fully support threads with Boehm GC.")
        AC_DEFINE(GC_FREEBSD_THREADS)
        AM_CFLAGS="$AM_CFLAGS -pthread"
        if test "${enable_parallel_mark}" = yes; then
          AC_DEFINE(PARALLEL_MARK)
        fi
        AC_DEFINE(THREAD_LOCAL_ALLOC)
        ;;
     *-*-kfreebsd*-gnu)
        AC_DEFINE(GC_FREEBSD_THREADS)
        AM_CFLAGS="$AM_CFLAGS -pthread"
        THREADDLLIBS=-pthread
        AC_DEFINE(_REENTRANT)
        if test "${enable_parallel_mark}" = yes; then
          AC_DEFINE(PARALLEL_MARK)
        fi
        AC_DEFINE(THREAD_LOCAL_ALLOC)
        AC_DEFINE(USE_COMPILER_TLS)
        ;;
     *-*-gnu*)
        AC_DEFINE(GC_GNU_THREADS)
        AC_DEFINE(_REENTRANT)
        AC_DEFINE(THREAD_LOCAL_ALLOC)
        ;;
     *-*-netbsd*)
        AC_MSG_WARN("Only on NetBSD 2.0 or later.")
        AC_DEFINE(GC_NETBSD_THREADS)
        AC_DEFINE(_REENTRANT)
        AC_DEFINE(_PTHREADS)
        THREADDLLIBS="-lpthread -lrt"
        ;;
     *-*-solaris*)
        AC_DEFINE(GC_SOLARIS_THREADS)
        if test "${enable_parallel_mark}" != no; then
          AC_DEFINE(PARALLEL_MARK)
        fi
        AC_DEFINE(THREAD_LOCAL_ALLOC)
        # Need to use alternate thread library, otherwise gctest hangs
        # on Solaris 8.
        multi_os_directory=`$CC -print-multi-os-directory`
        THREADDLLIBS="-L/usr/lib/lwp/$multi_os_directory \
                      -R/usr/lib/lwp/$multi_os_directory -lpthread -lrt"
        ;;
     *-*-irix*)
        AC_DEFINE(GC_IRIX_THREADS)
        ;;
     *-*-cygwin*)
        AC_DEFINE(GC_WIN32_THREADS)
        if test "${enable_parallel_mark}" != no; then
          AC_DEFINE(PARALLEL_MARK)
        fi
        AC_DEFINE(THREAD_LOCAL_ALLOC)
        # Cygwin doesn't have a real libpthread, so Libtool can't link
        # against it.
        THREADDLLIBS=""
        win32_threads=true
        ;;
     *-*-mingw*)
        AC_DEFINE(GC_WIN32_PTHREADS)
        # Using win32-pthreads
        if test "${enable_parallel_mark}" != no; then
          AC_DEFINE(PARALLEL_MARK)
        fi
        AC_DEFINE(THREAD_LOCAL_ALLOC)
        THREADDLLIBS="-lpthread"
        win32_threads=true
        ;;
     *-*-darwin*)
        AC_DEFINE(GC_DARWIN_THREADS)
        AC_MSG_WARN("Explicit GC_INIT() calls may be required.");
        # Parallel-mark is not well-tested on Darwin
        if test "${enable_parallel_mark}" != no; then
          AC_DEFINE(PARALLEL_MARK)
        fi
        AC_DEFINE(THREAD_LOCAL_ALLOC)
        darwin_threads=true
        ;;
     *-*-osf*)
        AC_DEFINE(GC_OSF1_THREADS)
        if test "${enable_parallel_mark}" = yes; then
          AC_DEFINE(PARALLEL_MARK)
          AC_DEFINE(THREAD_LOCAL_ALLOC)
          AC_MSG_WARN("Explicit GC_INIT() calls may be required.");
          # May want to enable it in other cases, too.
          # Measurements have not yet been done.
        fi
        AM_CFLAGS="$AM_CFLAGS -pthread"
        THREADDLLIBS="-lpthread -lrt"
        ;;
      *)
        AC_MSG_ERROR("Pthreads not supported by the GC on this platform.")
        ;;
    esac
    case "$host" in
     sparc*-*-solaris*)
        if test "$GCC" != yes; then
          CFLAGS="-O $CFLAGS"
          need_atomic_ops_asm=true
        fi
        ;;
    esac
    ;;
 win32)
    AC_DEFINE(GC_WIN32_THREADS)
    if test "${enable_parallel_mark}" != no; then
      AC_DEFINE(PARALLEL_MARK)
      AC_DEFINE(THREAD_LOCAL_ALLOC)
    else
      if test "${enable_shared}" != yes || test "${enable_static}" != no; then
        # Imply THREAD_LOCAL_ALLOC unless GC_DLL.
        AC_DEFINE(THREAD_LOCAL_ALLOC)
      fi
    fi
    if test "${enable_win32_dllmain}" = yes; then
      AC_DEFINE(GC_INSIDE_DLL, 1,
                [Enable Win32 DllMain-based approach of threads registering.])
    fi
    win32_threads=true
    AC_DEFINE([EMPTY_GETENV_RESULTS], [1],
              [Wine getenv may not return NULL for missing entry.])
    ;;
 dgux386)
    THREADS=dgux386
    AC_MSG_RESULT($THREADDLLIBS)
    # Use pthread GCC switch
    THREADDLLIBS=-pthread
    if test "${enable_parallel_mark}" = yes; then
        AC_DEFINE(PARALLEL_MARK)
    fi
    AC_DEFINE(THREAD_LOCAL_ALLOC)
    AC_MSG_WARN("Explicit GC_INIT() calls may be required.");
    AC_DEFINE([GC_DGUX386_THREADS], 1,
              [Define to enable support for DB/UX threads on i386.])
    AC_DEFINE([DGUX_THREADS], 1,
              [Define to enable support for DB/UX threads.])
    # Enable _POSIX4A_DRAFT10_SOURCE with flag -pthread
    AM_CFLAGS="-pthread $AM_CFLAGS"
    ;;
 aix)
    THREADS=posix
    THREADDLLIBS=-lpthread
    AC_DEFINE(GC_AIX_THREADS)
    AC_DEFINE(_REENTRANT)
    ;;
 rtems)
    THREADS=posix
    AC_DEFINE(GC_RTEMS_PTHREADS)
    AC_DEFINE(THREAD_LOCAL_ALLOC)
    ;;
 decosf1 | irix | mach | os2 | solaris | dce | vxworks)
    AC_MSG_ERROR(thread package $THREADS not yet supported)
    ;;
 *)
    AC_MSG_ERROR($THREADS is an unknown thread package)
    ;;
esac
AC_SUBST(THREADDLLIBS)
AM_CONDITIONAL(THREADS, test x$THREADS != xnone)
AM_CONDITIONAL(PTHREADS, test x$THREADS = xposix)
AM_CONDITIONAL(DARWIN_THREADS, test x$darwin_threads = xtrue)
AM_CONDITIONAL(WIN32_THREADS, test x$win32_threads = xtrue)

case "$host" in
   powerpc-*-darwin*)
      powerpc_darwin=true
      ;;
   *-*-wince*)
      if test "$enable_gc_debug" != "no"; then
        AC_DEFINE([GC_READ_ENV_FILE], 1,
                  [Read environment variables from the GC 'env' file.])
      fi
      ;;
esac

if test "$GCC" = yes; then
  # Output all warnings.
  AC_MSG_CHECKING(for gcc -Wextra)
  old_CFLAGS="$CFLAGS"
  CFLAGS="-Wextra $CFLAGS"
  AC_TRY_COMPILE([],[], [ac_cv_cc_wextra=yes], [ac_cv_cc_wextra=no])
  CFLAGS="$old_CFLAGS"
  AC_MSG_RESULT($ac_cv_cc_wextra)
  AS_IF([test "$ac_cv_cc_wextra" = yes], [WEXTRA="-Wextra"], [WEXTRA="-W"])
  CFLAGS="-Wall $WEXTRA $CFLAGS"
fi

AC_MSG_CHECKING(for xlc)
AC_TRY_COMPILE([],[
 #ifndef __xlC__
 # error
 #endif
], [compiler_xlc=yes], [compiler_xlc=no])
AC_MSG_RESULT($compiler_xlc)
AM_CONDITIONAL(COMPILER_XLC,test $compiler_xlc = yes)
if test $compiler_xlc = yes -a "$powerpc_darwin" = true; then
  # the darwin stack-frame-walking code is completely broken on xlc
  AC_DEFINE([DARWIN_DONT_PARSE_STACK], 1, [See doc/README.macros.])
fi

if test "$GCC" = yes; then
  # Disable aliasing optimization unless forced to.
  AC_MSG_CHECKING([whether gcc supports -fno-strict-aliasing])
  ac_cv_fno_strict_aliasing=no
  for cflag in $CFLAGS; do
    case "$cflag" in
      -fstrict-aliasing)
        # Opposite option already present
        ac_cv_fno_strict_aliasing=skipped
        break
        ;;
    esac
  done
  if test "$ac_cv_fno_strict_aliasing" != skipped; then
    old_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -fno-strict-aliasing"
    AC_TRY_COMPILE([],[], [ac_cv_fno_strict_aliasing=yes], [])
    CFLAGS="$old_CFLAGS"
    AS_IF([test "$ac_cv_fno_strict_aliasing" = yes],
          [CFLAGS="$CFLAGS -fno-strict-aliasing"], [])
  fi
  AC_MSG_RESULT($ac_cv_fno_strict_aliasing)
fi

case "$host" in
# While IRIX 6 has libdl for the O32 and N32 ABIs, it's missing for N64
# and unnecessary everywhere.
  mips-sgi-irix6*) ;;
# We never want libdl on darwin. It is a fake libdl that just ends up making
# dyld calls anyway.  The same applies to Cygwin.
  *-*-darwin*) ;;
  *-*-cygwin*) ;;
  *)
    AC_CHECK_LIB(dl, dlopen, THREADDLLIBS="$THREADDLLIBS -ldl")
    ;;
esac

case "$host" in
  *-*-hpux*)
    avoid_cpp_lib=yes;;
  *)
    avoid_cpp_lib=no;
    ;;
esac
AM_CONDITIONAL(AVOID_CPP_LIB,test $avoid_cpp_lib = yes)

# extra LD Flags which are required for targets
case "${host}" in
  *-*-darwin*)
    extra_ldflags_libgc=-Wl,-single_module
    ;;
esac
AC_SUBST(extra_ldflags_libgc)

AC_SUBST(EXTRA_TEST_LIBS)

target_all=libgc.la
AC_SUBST(target_all)

dnl If the target is an eCos system, use the appropriate eCos
dnl I/O routines.
dnl FIXME: this should not be a local option but a global target
dnl system; at present there is no eCos target.
TARGET_ECOS="no"
AC_ARG_WITH(ecos,
[  --with-ecos             enable runtime eCos target support],
TARGET_ECOS="$with_ecos"
)

addobjs=
addlibs=
CXXLIBS=

case "$TARGET_ECOS" in
   no)
      ;;
   *)
      AC_DEFINE([ECOS], 1, [Define to enable eCos target support.])
      AM_CPPFLAGS="-I${TARGET_ECOS}/include $AM_CPPFLAGS"
      addobjs="$addobjs ecos.lo"
      ;;
esac

AM_CONDITIONAL(CPLUSPLUS, test "${enable_cplusplus}" = yes)

if test "$GCC" = yes; then
  if test "${enable_cplusplus}" = yes; then
    case "$host" in
      *-*-cygwin* | *-*-mingw*)
        AC_MSG_CHECKING([whether libsupc++ required])
        SUPC="`$CXX -print-file-name=libsupc++.a 2>/dev/null`"
        if test -n "$SUPC" -a "$SUPC" != "libsupc++.a"; then
          AC_MSG_RESULT(yes)
          CXXLIBS="-lsupc++"
        else
          AC_MSG_RESULT(no)
        fi
        ;;
    esac
  fi
fi

AC_SUBST(CXX)
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CPPFLAGS)
AC_SUBST(CXXLIBS)

# Configuration of shared libraries
#
AC_MSG_CHECKING(whether to build shared libraries)
AC_ENABLE_SHARED

case "$host" in
 alpha-*-openbsd*)
     enable_shared=no
     ;;
 *)
     ;;
esac

AC_MSG_RESULT($enable_shared)

# Compile with GC_DLL defined unless building static libraries.
if test "${enable_shared}" = yes; then
  if test "${enable_static}" = no; then
    AC_DEFINE(GC_DLL)
    if test "$GCC" = yes; then
      # Pass -fvisibility=hidden option if supported
      AC_MSG_CHECKING([whether gcc supports -fvisibility])
      old_CFLAGS="$CFLAGS"
      CFLAGS="-Werror -fvisibility=hidden $CFLAGS"
      AC_TRY_COMPILE([],[], [ac_cv_fvisibility_hidden=yes],
                     [ac_cv_fvisibility_hidden=no])
      CFLAGS="$old_CFLAGS"
      AS_IF([test "$ac_cv_fvisibility_hidden" = yes],
            [CFLAGS="-DGC_VISIBILITY_HIDDEN_SET -fvisibility=hidden $CFLAGS"])
      AC_MSG_RESULT($ac_cv_fvisibility_hidden)
    fi
  fi
fi

# Configuration of machine-dependent code
#
AC_MSG_CHECKING(which machine-dependent code should be used)
machdep=
case "$host" in
 alpha-*-openbsd*)
    if test x"${ac_cv_lib_dl_dlopen}" != xyes ; then
       AC_MSG_WARN(
         "OpenBSD/Alpha without dlopen(). Shared library support is disabled.")
    fi
    ;;
 i?86-*-solaris2.[[89]])
    # PROC_VDB appears to work in 2.8 and 2.9 but not in 2.10+ (for now).
    AC_DEFINE([SOLARIS25_PROC_VDB_BUG_FIXED], 1,
              [See the comment in gcconfig.h.])
    ;;
 mips-*-*)
    dnl AC_DEFINE(NO_EXECUTE_PERMISSION)
    dnl This is now redundant, but it is also important for incremental GC
    dnl performance under Irix.
    ;;
 sparc-*-netbsd*)
    machdep="sparc_netbsd_mach_dep.lo"
    ;;
 sparc*-*-linux* | sparc*-*-openbsd* | sparc64-*-freebsd* | sparc64-*-netbsd*)
    machdep="sparc_mach_dep.lo"
    ;;
 sparc-sun-solaris2.3)
    machdep="sparc_mach_dep.lo"
    AC_DEFINE(SUNOS53_SHARED_LIB, 1,
              [Define to work around a Solaris 5.3 bug (see dyn_load.c).])
    ;;
 sparc*-sun-solaris2*)
    machdep="sparc_mach_dep.lo"
    ;;
 ia64-*-*)
    machdep="ia64_save_regs_in_stack.lo"
    ;;
esac
AC_MSG_RESULT($machdep)
addobjs="$addobjs $machdep"
AC_SUBST(addobjs)
AC_SUBST(addlibs)

AC_PROG_LIBTOOL

dnl We use these options to decide which functions to include.
AC_ARG_WITH(target-subdir,
[  --with-target-subdir=SUBDIR
                          configuring with a cross compiler])
AC_ARG_WITH(cross-host,
[  --with-cross-host=HOST  configuring with a cross compiler])

# automake wants to see AC_EXEEXT.  But we don't need it.  And having
# it is actually a problem, because the compiler we're passed can't
# necessarily do a full link.  So we fool automake here.
if false; then
  # autoconf 2.50 runs AC_EXEEXT by default, and the macro expands
  # to nothing, so nothing would remain between `then' and `fi' if it
  # were not for the `:' below.
  :
  AC_EXEEXT
fi

dnl As of 4.13a2, the collector will not properly work on Solaris when
dnl built with gcc and -O.  So we remove -O in the appropriate case.
dnl Not needed anymore on Solaris.
AC_MSG_CHECKING(whether Solaris gcc optimization fix is necessary)
case "$host" in
 *aix*)
    if test "$GCC" = yes; then
       AC_MSG_RESULT(yes)
       new_CFLAGS=
       for i in $CFLAGS; do
          case "$i" in
           -O*)
              ;;
           *)
              new_CFLAGS="$new_CFLAGS $i"
              ;;
          esac
       done
       CFLAGS="$new_CFLAGS"
    else
       AC_MSG_RESULT(no)
    fi
    ;;
 *) AC_MSG_RESULT(no) ;;
esac

dnl Include defines that have become de facto standard.
dnl ALL_INTERIOR_POINTERS and NO_EXECUTE_PERMISSION can be overridden
dnl in the startup code.
AC_DEFINE([NO_EXECUTE_PERMISSION], [1],
          [Define to make the collector not allocate executable memory
           by default.])
AC_DEFINE([ALL_INTERIOR_POINTERS], [1],
          [Define to recognise all pointers to the interior of objects.])


dnl Interface Selection
dnl -------------------
dnl
dnl By default, make the library as general as possible.
dnl enable_gcj_support=no
AC_ARG_ENABLE(gcj-support,
    [AC_HELP_STRING([--disable-gcj-support],
        [Disable support for gcj.])])
if test x"$enable_gcj_support" != xno; then
    AC_DEFINE(GC_GCJ_SUPPORT, 1, [Define to include support for gcj.])
fi

dnl Interaction with other programs that might use signals.
AC_ARG_ENABLE(sigrt-signals,
    [AC_HELP_STRING([--enable-sigrt-signals],
        [Force GC to use SIGRTMIN-based signals for thread suspend/resume])])
if test x"${enable_sigrt_signals}" = xyes; then
    AC_DEFINE([GC_USESIGRT_SIGNALS], 1,
              [Force the GC to use signals based on SIGRTMIN+k.])
fi


dnl Debugging
dnl ---------

AH_TEMPLATE([GC_HAVE_BUILTIN_BACKTRACE],
            [Define if backtrace information is supported.])
AH_TEMPLATE([MAKE_BACK_GRAPH], [See doc/README.macros.])
AH_TEMPLATE([SAVE_CALL_COUNT],
            [The number of caller frames saved when allocating with the
             debugging API.])
UNWINDLIBS=
AC_ARG_ENABLE(gc-debug,
[AC_HELP_STRING([--enable-gc-debug],
    [include full support for pointer backtracing etc.])],
[ if test "$enable_gc_debug" = "yes"; then
    AC_MSG_WARN("Should define GC_DEBUG and use debug alloc in clients.")
    AC_DEFINE([KEEP_BACK_PTRS], 1,
              [Define to save back-pointers in debugging headers.])
    keep_back_ptrs=true
    AC_DEFINE([DBG_HDRS_ALL], 1,
              [Define to force debug headers on all objects.])
    AH_TEMPLATE([SHORT_DBG_HDRS],
                [Shorten the headers to minimize object size at the expense
                 of checking for writes past the end (see doc/README.macros).])

    case $host in
      ia64-*-linux* )
        AC_DEFINE(MAKE_BACK_GRAPH)
        AC_DEFINE(SAVE_CALL_COUNT, 8)
        AC_CHECK_LIB(unwind, backtrace, [
          AC_DEFINE(GC_HAVE_BUILTIN_BACKTRACE)
          UNWINDLIBS=-lunwind
          AC_MSG_WARN("Client code may need to link against libunwind.")
        ])
      ;;
      x86-*-linux* | i586-*-linux* | i686-*-linux* | x86_64-*-linux* )
        AC_DEFINE(MAKE_BACK_GRAPH)
        AC_MSG_WARN("Client must not use -fomit-frame-pointer.")
        AC_DEFINE(SAVE_CALL_COUNT, 8)
      ;;
      i[3456]86-*-dgux*)
        AC_DEFINE(MAKE_BACK_GRAPH)
      ;;
    esac ]
  fi)
AM_CONDITIONAL([MAKE_BACK_GRAPH], [test x"$enable_gc_debug" = xyes])
AM_CONDITIONAL([KEEP_BACK_PTRS], [test x"$keep_back_ptrs" = xtrue])

# Check for dladdr (used for debugging).
AC_MSG_CHECKING(for dladdr)
have_dladdr=no
AC_TRY_COMPILE([
#define _GNU_SOURCE 1
#include <dlfcn.h>], [{
    Dl_info info;
    (void)dladdr("", &info);
}], [ have_dladdr=yes ])
AC_MSG_RESULT($have_dladdr)
if test x"$have_dladdr" = xyes; then
  AC_DEFINE([HAVE_DLADDR], 1, [Define to use 'dladdr' function.])
fi

# Check for AViiON Machines running DGUX
ac_is_dgux=no
AC_CHECK_HEADER(sys/dg_sys_info.h,
[ac_is_dgux=yes;])

    ## :GOTCHA: we do not check anything but sys/dg_sys_info.h
if test $ac_is_dgux = yes; then
    dgux_spec_opts="-DDGUX -D_DGUX_SOURCE -Di386 -mno-legend -O2"
    CFLAGS="$dgux_spec_opts $CFLAGS"
    CXXFLAGS="$dgux_spec_opts $CXXFLAGS"
    if test "$enable_gc_debug" = "yes"; then
      CFLAGS="-g -mstandard $CFLAGS"
      CXXFLAGS="-g -mstandard $CXXFLAGS"
    fi
    AC_SUBST(CFLAGS)
    AC_SUBST(CXXFLAGS)
fi

AC_ARG_ENABLE(java-finalization,
    [AC_HELP_STRING([--disable-java-finalization],
        [Disable support for java finalization.])])
if test x"$enable_java_finalization" != xno; then
    AC_DEFINE([JAVA_FINALIZATION], 1, [See doc/README.macros.])
fi

AC_ARG_ENABLE(atomic-uncollectable,
    [AC_HELP_STRING([--disable-atomic-uncollectible],
        [Disable support for atomic uncollectible allocation.])])
if test x"$enable_atomic_uncollectible" != x"no"; then
    AC_DEFINE(ATOMIC_UNCOLLECTABLE, 1,
        [Define to enable atomic uncollectible allocation.])
fi

AC_ARG_ENABLE(redirect-malloc,
    [AC_HELP_STRING([--enable-redirect-malloc],
        [Redirect malloc and friends to GC routines])])

if test "${enable_redirect_malloc}" = yes; then
    if test "${enable_gc_debug}" = yes; then
        AC_DEFINE([REDIRECT_MALLOC], GC_debug_malloc_replacement,
                  [If defined, redirect malloc to this function.])
        AC_DEFINE([REDIRECT_REALLOC], GC_debug_realloc_replacement,
                  [If defined, redirect GC_realloc to this function.])
        AC_DEFINE([REDIRECT_FREE], GC_debug_free,
                  [If defined, redirect free to this function.])
    else
        AC_DEFINE(REDIRECT_MALLOC, GC_malloc)
    fi
    AC_DEFINE([GC_USE_DLOPEN_WRAP], 1, [See doc/README.macros.])
fi

AC_ARG_ENABLE(disclaim,
    [AC_HELP_STRING([--disable-disclaim],
        [Disable alternative (more efficient) finalization interface.])])
if test x"$enable_disclaim" != xno; then
    AC_DEFINE(ENABLE_DISCLAIM, 1,
        [Define to enable alternative finalization interface.])
fi
AM_CONDITIONAL(ENABLE_DISCLAIM,
    [test x"$enable_disclaim" != xno])

AC_ARG_ENABLE(large-config,
    [AC_HELP_STRING([--enable-large-config],
        [Optimize for large (> 100 MB) heap or root set])])

if test "${enable_large_config}" = yes; then
    AC_DEFINE(LARGE_CONFIG, 1, [Define to optimize for large heaps or root sets.])
fi

AC_ARG_ENABLE(handle-fork,
    [AC_HELP_STRING([--enable-handle-fork],
        [Attempt to ensure a usable collector after fork() in multi-threaded
         programs.])])

if test "${enable_handle_fork}" = yes; then
    AC_DEFINE(HANDLE_FORK, 1,
              [Define to install pthread_atfork() handlers by default.])
elif test "${enable_handle_fork}" = no; then
    AC_DEFINE(NO_HANDLE_FORK, 1,
              [Prohibit installation of pthread_atfork() handlers.])
fi

dnl This is something of a hack.  When cross-compiling we turn off
dnl some functionality.  We also enable the "small" configuration.
dnl These is only correct when targetting an embedded system.  FIXME.
if test -n "${with_cross_host}"; then
   AC_DEFINE([NO_CLOCK], 1, [Define to not use system clock (cross compiling).])
   AC_DEFINE([SMALL_CONFIG], 1,
             [Define to tune the collector for small heap sizes.])
fi

if test "$enable_gc_debug" = "no"; then
   AC_DEFINE([NO_DEBUGGING], 1,
             [Disable debugging, like GC_dump and its callees.])
fi

AC_SUBST(UNWINDLIBS)

AC_ARG_ENABLE(gc-assertions,
    [AC_HELP_STRING([--enable-gc-assertions],
        [collector-internal assertion checking])])
if test "${enable_gc_assertions}" = yes; then
    AC_DEFINE([GC_ASSERTIONS], 1, [Define to enable internal debug assertions.])
fi

AC_ARG_ENABLE(munmap,
    [AC_HELP_STRING([--enable-munmap=N],
        [return page to the os if empty for N collections])],
    MUNMAP_THRESHOLD=$enableval)
if test "${enable_munmap}" != ""; then
    AC_DEFINE([USE_MMAP], 1,
              [Define to use mmap instead of sbrk to expand the heap.])
    case "$host" in
      *-*-cygwin*)
        # Workaround for Cygwin: use VirtualAlloc since mmap(PROT_NONE) fails
        AC_DEFINE([USE_WINALLOC], 1,
                  [Define to use Win32 VirtualAlloc (instead of sbrk or
                   mmap) to expand the heap.])
        ;;
    esac
    AC_DEFINE([USE_MUNMAP], 1,
              [Define to return memory to OS with munmap calls
               (see doc/README.macros).])
    if test "${MUNMAP_THRESHOLD}" = "yes"; then
      MUNMAP_THRESHOLD=6
    fi
    AC_DEFINE_UNQUOTED([MUNMAP_THRESHOLD], [${MUNMAP_THRESHOLD}],
        [Number of GC cycles to wait before unmapping an unused block.])
else
    if test "${gc_use_mmap}" = "yes"; then
      AC_DEFINE([USE_MMAP], 1,
                [Define to use mmap instead of sbrk to expand the heap.])
    fi
fi

AM_CONDITIONAL(USE_LIBDIR, test -z "$with_cross_host")

AC_ARG_ENABLE(single-obj-compilation,
   [AC_HELP_STRING([--enable-single-obj-compilation],
                   [Compile all library .c files into single .o])],
   [single_obj_compilation=yes])
AM_CONDITIONAL([SINGLE_GC_OBJ], [test "$single_obj_compilation" = "yes"])

# Atomic Ops
# ----------

# Do we want to use an external libatomic_ops?  By default use it if it's
# found.
AC_ARG_WITH([libatomic-ops],
    [AS_HELP_STRING([--with-libatomic-ops[=yes|no|check]],
                    [Use a external libatomic_ops? (default: check)])],
    [], [with_libatomic_ops=check])

# Check for an external libatomic_ops if the answer was yes or check.  If not
# found, fail on yes, and convert check to no.
# Note: "syntax error near unexpected token ATOMIC_OPS" reported by configure
# means Autotools pkg.m4 file was not found during aclocal.m4 generation.
missing_libatomic_ops=false
AS_IF([test x"$with_libatomic_ops" != xno],
  [ PKG_CHECK_MODULES([ATOMIC_OPS], [atomic_ops], [],
                      [ missing_libatomic_ops=true ]) ])
AS_IF([test x$missing_libatomic_ops = xtrue ],
      [ AS_IF([test x"$with_libatomic_ops" != xcheck],
              [ AC_MSG_ERROR([An external libatomic_ops was not found]) ])
        with_libatomic_ops=no ])

# If we have neither an external or an internal version, offer a useful hint
# and exit.
AS_IF([test x"$with_libatomic_ops" = xno -a ! -e "$srcdir/libatomic_ops"],
  [ AC_MSG_ERROR([libatomic_ops is required.  You can either install it on
                  your system, or fetch and unpack a recent version into the
                  source directory and link or rename it to libatomic_ops.]) ])

# Finally, emit the definitions for bundled or external AO.
AC_MSG_CHECKING([which libatomic_ops to use])
AS_IF([test x"$with_libatomic_ops" != xno],
  [ AC_MSG_RESULT([external]) ],
  [ AC_MSG_RESULT([internal])
    ATOMIC_OPS_CFLAGS='-I$(top_builddir)/libatomic_ops/src -I$(top_srcdir)/libatomic_ops/src'
    ATOMIC_OPS_LIBS=""
    AC_SUBST([ATOMIC_OPS_CFLAGS])
  ])
AC_CONFIG_SUBDIRS([libatomic_ops])
AM_CONDITIONAL([USE_INTERNAL_LIBATOMIC_OPS],
    [test x$with_libatomic_ops = xno -a x"$THREADS" != xnone])
AM_CONDITIONAL([NEED_ATOMIC_OPS_ASM],
    [test x$with_libatomic_ops = xno -a x$need_atomic_ops_asm = xtrue])

dnl Produce the Files
dnl -----------------

AC_CONFIG_FILES([Makefile bdw-gc.pc])

AC_CONFIG_COMMANDS([default],,
  [ srcdir="${srcdir}"
    host=${host}
    CONFIG_SHELL=${CONFIG_SHELL-/bin/sh}
    CC="${CC}"
    DEFS="$DEFS" ])

AC_OUTPUT
