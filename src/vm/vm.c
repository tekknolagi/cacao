/* src/vm/finalizer.c - finalizer linked list and thread

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

   Changes:

   $Id: finalizer.c 4357 2006-01-22 23:33:38Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <stdlib.h>

#include "vm/types.h"

#include "mm/boehm.h"
#include "mm/memory.h"
#include "native/jni.h"
#include "native/native.h"

#if defined(USE_THREADS)
# if defined(NATIVE_THREADS)
#  include "threads/native/threads.h"
# else
#  include "threads/green/threads.h"
#  include "threads/green/locks.h"
# endif
#endif

#include "vm/classcache.h"
#include "vm/exceptions.h"
#include "vm/finalizer.h"
#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/options.h"
#include "vm/properties.h"
#include "vm/signallocal.h"
#include "vm/stringlocal.h"
#include "vm/suck.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/profile/profile.h"


/* Invocation API variables ***************************************************/

JavaVM     *_Jv_jvm;                    /* denotes a Java VM                  */
_Jv_JNIEnv *_Jv_env;                    /* pointer to native method interface */


/* global variables ***********************************************************/

s4 vms = 0;                             /* number of VMs created              */

bool vm_initializing = false;
bool vm_exiting = false;

#if defined(ENABLE_INTRP)
u1 *intrp_main_stack = NULL;
#endif

void **stackbottom = NULL;

char *mainstring = NULL;
classinfo *mainclass = NULL;

char *specificmethodname = NULL;
char *specificsignature = NULL;

bool startit = true;


/* define heap sizes **********************************************************/

#define HEAP_MAXSIZE      64 * 1024 * 1024  /* default 64MB                   */
#define HEAP_STARTSIZE    2  * 1024 * 1024  /* default 2MB                    */
#define STACK_SIZE              128 * 1024  /* default 128kB                  */


/* define command line options ************************************************/

enum {
	/* Java options */

	OPT_JAR,

	OPT_D32,
	OPT_D64,

	OPT_CLASSPATH,
	OPT_D,

	OPT_VERBOSE,

	OPT_VERSION,
	OPT_SHOWVERSION,
	OPT_FULLVERSION,

	OPT_HELP,
	OPT_X,

	/* Java non-standard options */

	OPT_JIT,
	OPT_INTRP,

	OPT_BOOTCLASSPATH,
	OPT_BOOTCLASSPATH_A,
	OPT_BOOTCLASSPATH_P,

	OPT_PROF,
	OPT_PROF_OPTION,

	OPT_MS,
	OPT_MX,

	/* CACAO options */

	OPT_VERBOSE1,
	OPT_NOIEEE,
	OPT_SOFTNULL,
	OPT_TIME,

#if defined(ENABLE_STATISTICS)
	OPT_STAT,
#endif

	OPT_LOG,
	OPT_CHECK,
	OPT_LOAD,
	OPT_METHOD,
	OPT_SIGNATURE,
	OPT_SHOW,
	OPT_ALL,
	OPT_OLOOP,
	OPT_INLINING,

	OPT_VERBOSETC,
	OPT_NOVERIFY,
	OPT_LIBERALUTF,
	OPT_EAGER,

	/* optimization options */

#if defined(ENABLE_IFCONV)
	OPT_IFCONV,
#endif

#if defined(ENABLE_LSRA)
	OPT_LSRA,
#endif

#if defined(ENABLE_INTRP)
	/* interpreter options */

	OPT_NO_DYNAMIC,
	OPT_NO_REPLICATION,
	OPT_NO_QUICKSUPER,
	OPT_STATIC_SUPERS,
	OPT_TRACE,
#endif

	OPT_SS,

#ifdef ENABLE_JVMTI
	OPT_DEBUG,
	OPT_AGENTLIB,
	OPT_AGENTPATH,
#endif

	DUMMY
};


opt_struct opts[] = {
	/* Java options */

	{ "jar",               false, OPT_JAR },

	{ "d32",               false, OPT_D32 },
	{ "d64",               false, OPT_D64 },
	{ "client",            false, OPT_IGNORE },
	{ "server",            false, OPT_IGNORE },
	{ "hotspot",           false, OPT_IGNORE },

	{ "classpath",         true,  OPT_CLASSPATH },
	{ "cp",                true,  OPT_CLASSPATH },
	{ "D",                 true,  OPT_D },
	{ "version",           false, OPT_VERSION },
	{ "showversion",       false, OPT_SHOWVERSION },
	{ "fullversion",       false, OPT_FULLVERSION },
	{ "help",              false, OPT_HELP },
	{ "?",                 false, OPT_HELP },
	{ "X",                 false, OPT_X },

	{ "noasyncgc",         false, OPT_IGNORE },
	{ "noverify",          false, OPT_NOVERIFY },
	{ "liberalutf",        false, OPT_LIBERALUTF },
	{ "v",                 false, OPT_VERBOSE1 },
	{ "verbose:",          true,  OPT_VERBOSE },

#ifdef TYPECHECK_VERBOSE
	{ "verbosetc",         false, OPT_VERBOSETC },
#endif
#if defined(__ALPHA__)
	{ "noieee",            false, OPT_NOIEEE },
#endif
	{ "softnull",          false, OPT_SOFTNULL },
	{ "time",              false, OPT_TIME },
#if defined(ENABLE_STATISTICS)
	{ "stat",              false, OPT_STAT },
#endif
	{ "log",               true,  OPT_LOG },
	{ "c",                 true,  OPT_CHECK },
	{ "l",                 false, OPT_LOAD },
	{ "eager",             false, OPT_EAGER },
	{ "sig",               true,  OPT_SIGNATURE },
	{ "all",               false, OPT_ALL },
	{ "oloop",             false, OPT_OLOOP },
#if defined(ENABLE_IFCONV)
	{ "ifconv",            false, OPT_IFCONV },
#endif
#if defined(ENABLE_LSRA)
	{ "lsra",              false, OPT_LSRA },
#endif

#if defined(ENABLE_INTRP)
	/* interpreter options */

	{ "trace",             false, OPT_TRACE },
	{ "static-supers",     true,  OPT_STATIC_SUPERS },
	{ "no-dynamic",        false, OPT_NO_DYNAMIC },
	{ "no-replication",    false, OPT_NO_REPLICATION },
	{ "no-quicksuper",     false, OPT_NO_QUICKSUPER },
#endif

	/* JVMTI Agent Command Line Options */
#ifdef ENABLE_JVMTI
	{ "agentlib:",         true,  OPT_AGENTLIB },
	{ "agentpath:",        true,  OPT_AGENTPATH },
#endif

	/* Java non-standard options */

	{ "Xjit",              false, OPT_JIT },
	{ "Xint",              false, OPT_INTRP },
	{ "Xbootclasspath:",   true,  OPT_BOOTCLASSPATH },
	{ "Xbootclasspath/a:", true,  OPT_BOOTCLASSPATH_A },
	{ "Xbootclasspath/p:", true,  OPT_BOOTCLASSPATH_P },
#ifdef ENABLE_JVMTI
	{ "Xdebug",            false, OPT_DEBUG },
#endif 
	{ "Xms",               true,  OPT_MS },
	{ "ms",                true,  OPT_MS },
	{ "Xmx",               true,  OPT_MX },
	{ "mx",                true,  OPT_MX },
	{ "Xss",               true,  OPT_SS },
	{ "ss",                true,  OPT_SS },
	{ "Xprof:",            true,  OPT_PROF_OPTION },
	{ "Xprof",             false, OPT_PROF },

	/* keep these at the end of the list */

	{ "i",                 true,  OPT_INLINING },
	{ "m",                 true,  OPT_METHOD },
	{ "s",                 true,  OPT_SHOW },

	{ NULL,                false, 0 }
};


/* usage ***********************************************************************

   Prints the correct usage syntax to stdout.

*******************************************************************************/

void usage(void)
{
	puts("Usage: cacao [-options] classname [arguments]");
	puts("               (to run a class file)");
	puts("       cacao [-options] -jar jarfile [arguments]");
	puts("               (to run a standalone jar file)\n");

	puts("Java options:");
	puts("    -d32                     use 32-bit data model if available");
	puts("    -d64                     use 64-bit data model if available");
	puts("    -client                  compatibility (currently ignored)");
	puts("    -server                  compatibility (currently ignored)");
	puts("    -hotspot                 compatibility (currently ignored)\n");

	puts("    -cp <path>               specify a path to look for classes");
	puts("    -classpath <path>        specify a path to look for classes");
	puts("    -D<name>=<value>         add an entry to the property list");
	puts("    -verbose[:class|gc|jni]  enable specific verbose output");
	puts("    -version                 print product version and exit");
	puts("    -fullversion             print jpackage-compatible product version and exit");
	puts("    -showversion             print product version and continue");
	puts("    -help, -?                print this help message");
	puts("    -X                       print help on non-standard Java options\n");

#ifdef ENABLE_JVMTI
	puts("    -agentlib:<agent-lib-name>=<options>  library to load containg JVMTI agent");
	puts("    -agentpath:<path-to-agent>=<options>  path to library containg JVMTI agent");
#endif

	puts("CACAO options:\n");
	puts("    -v                       write state-information");
	puts("    -verbose[:call|exception]enable specific verbose output");
#ifdef TYPECHECK_VERBOSE
	puts("    -verbosetc               write debug messages while typechecking");
#endif
#if defined(__ALPHA__)
	puts("    -noieee                  don't use ieee compliant arithmetic");
#endif
	puts("    -noverify                don't verify classfiles");
	puts("    -liberalutf              don't warn about overlong UTF-8 sequences");
	puts("    -softnull                use software nullpointer check");
	puts("    -time                    measure the runtime");
#if defined(ENABLE_STATISTICS)
	puts("    -stat                    detailed compiler statistics");
#endif
	puts("    -log logfile             specify a name for the logfile");
	puts("    -c(heck)b(ounds)         don't check array bounds");
	puts("            s(ync)           don't check for synchronization");
	puts("    -oloop                   optimize array accesses in loops"); 
	puts("    -l                       don't start the class after loading");
	puts("    -eager                   perform eager class loading and linking");
	puts("    -all                     compile all methods, no execution");
	puts("    -m                       compile only a specific method");
	puts("    -sig                     specify signature for a specific method");
	puts("    -s(how)a(ssembler)       show disassembled listing");
	puts("           c(onstants)       show the constant pool");
	puts("           d(atasegment)     show data segment listing");
	puts("           e(xceptionstubs)  show disassembled exception stubs (only with -sa)");
	puts("           i(ntermediate)    show intermediate representation");
	puts("           m(ethods)         show class fields and methods");
	puts("           n(ative)          show disassembled native stubs");
	puts("           u(tf)             show the utf - hash");
	puts("    -i     n(line)           activate inlining");
	puts("           v(irtual)         inline virtual methods (uses/turns rt option on)");
	puts("           e(exception)      inline methods with exceptions");
	puts("           p(aramopt)        optimize argument renaming");
	puts("           o(utsiders)       inline methods of foreign classes");
#if defined(ENABLE_IFCONV)
	puts("    -ifconv                  use if-conversion");
#endif
#if defined(ENABLE_LSRA)
	puts("    -lsra                    use linear scan register allocation");
#endif

	/* exit with error code */

	exit(1);
}   


static void Xusage(void)
{
#if defined(ENABLE_JIT)
	puts("    -Xjit                    JIT mode execution (default)");
#endif
#if defined(ENABLE_INTRP)
	puts("    -Xint                    interpreter mode execution");
#endif
	puts("    -Xbootclasspath:<zip/jar files and directories separated by :>");
    puts("                             value is set as bootstrap class path");
	puts("    -Xbootclasspath/a:<zip/jar files and directories separated by :>");
	puts("                             value is appended to the bootstrap class path");
	puts("    -Xbootclasspath/p:<zip/jar files and directories separated by :>");
	puts("                             value is prepended to the bootstrap class path");
	puts("    -Xms<size>               set the initial size of the heap (default: 2MB)");
	puts("    -Xmx<size>               set the maximum size of the heap (default: 64MB)");
	puts("    -Xss<size>               set the thread stack size (default: 128kB)");
	puts("    -Xprof[:bb]              collect and print profiling data");
#if defined(ENABLE_JVMTI)
	puts("    -Xdebug<transport>       enable remote debugging");
#endif 

	/* exit with error code */

	exit(1);
}   


/* version *********************************************************************

   Only prints cacao version information.

*******************************************************************************/

static void version(void)
{
	puts("java version \""JAVA_VERSION"\"");
	puts("CACAO version "VERSION"");

	puts("Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,");
	puts("C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,");
	puts("E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,");
	puts("J. Wenninger, Institut f. Computersprachen - TU Wien\n");

	puts("This program is free software; you can redistribute it and/or");
	puts("modify it under the terms of the GNU General Public License as");
	puts("published by the Free Software Foundation; either version 2, or (at");
	puts("your option) any later version.\n");

	puts("This program is distributed in the hope that it will be useful, but");
	puts("WITHOUT ANY WARRANTY; without even the implied warranty of");
	puts("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU");
	puts("General Public License for more details.\n");

	puts("Configure/Build options:\n");
	puts("  ./configure: "VERSION_CONFIGURE_ARGS"");
	puts("  CC         : "VERSION_CC" ("__VERSION__")");
	puts("  CFLAGS     : "VERSION_CFLAGS"");
}


/* fullversion *****************************************************************

   Prints a Sun compatible version information (required e.g. by
   jpackage, www.jpackage.org).

*******************************************************************************/

static void fullversion(void)
{
	puts("java full version \"cacao-"JAVA_VERSION"\"");

	/* exit normally */

	exit(0);
}


/* vm_create *******************************************************************

   Creates a JVM.  Called by JNI_CreateJavaVM.

*******************************************************************************/

bool vm_create(JavaVMInitArgs *vm_args)
{
	char *cp;
	s4    cplen;
	u4    heapmaxsize;
	u4    heapstartsize;
	s4    opt;
	s4    i, j, k;

	/* check the JNI version requested */

	switch (vm_args->version) {
	case JNI_VERSION_1_1:
		break;
	case JNI_VERSION_1_2:
	case JNI_VERSION_1_4:
		break;
	default:
		return false;
	}

	/* we only support 1 JVM instance */

	if (vms > 0)
		return false;


	/* get stuff from the environment *****************************************/

#if defined(DISABLE_GC)
	nogc_init(HEAP_MAXSIZE, HEAP_STARTSIZE);
#endif

	/* set the bootclasspath */

	cp = getenv("BOOTCLASSPATH");

	if (cp) {
		bootclasspath = MNEW(char, strlen(cp) + strlen("0"));
		strcpy(bootclasspath, cp);

	} else {
		cplen = strlen(CACAO_VM_ZIP_PATH) +
			strlen(":") +
			strlen(CLASSPATH_GLIBJ_ZIP_PATH) +
			strlen("0");

		bootclasspath = MNEW(char, cplen);
		strcat(bootclasspath, CACAO_VM_ZIP_PATH);
		strcat(bootclasspath, ":");
		strcat(bootclasspath, CLASSPATH_GLIBJ_ZIP_PATH);
	}


	/* set the classpath */

	cp = getenv("CLASSPATH");

	if (cp) {
		classpath = MNEW(char, strlen(cp) + strlen("0"));
		strcat(classpath, cp);

	} else {
		classpath = MNEW(char, strlen(".") + strlen("0"));
		strcpy(classpath, ".");
	}


	/* interpret the options **************************************************/
   
	checknull  = false;
	opt_noieee = false;

	heapmaxsize   = HEAP_MAXSIZE;
	heapstartsize = HEAP_STARTSIZE;
	opt_stacksize = STACK_SIZE;

	/* initialize properties before commandline handling */

	if (!properties_init())
		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "Unable to init properties");

	/* iterate over all passed options */

	while ((opt = options_get(opts, vm_args)) != OPT_DONE) {
		switch (opt) {
		case OPT_IGNORE:
			break;
			
		case OPT_JAR:
			opt_jar = true;
			break;

		case OPT_D32:
#if SIZEOF_VOID_P == 8
			puts("Running a 32-bit JVM is not supported on this platform.");
			exit(1);
#endif
			break;

		case OPT_D64:
#if SIZEOF_VOID_P == 4
			puts("Running a 64-bit JVM is not supported on this platform.");
			exit(1);
#endif
			break;

		case OPT_CLASSPATH:
			/* forget old classpath and set the argument as new classpath */
			MFREE(classpath, char, strlen(classpath));

			classpath = MNEW(char, strlen(opt_arg) + strlen("0"));
			strcpy(classpath, opt_arg);
			break;

		case OPT_D:
			for (j = 0; j < strlen(opt_arg); j++) {
				if (opt_arg[j] == '=') {
					opt_arg[j] = '\0';
					properties_add(opt_arg, opt_arg + j + 1);
					goto didit;
				}
			}

			/* if no '=' is given, just create an empty property */

			properties_add(opt_arg, "");
					
		didit:
			break;

		case OPT_BOOTCLASSPATH:
			/* Forget default bootclasspath and set the argument as
			   new boot classpath. */
			MFREE(bootclasspath, char, strlen(bootclasspath));

			bootclasspath = MNEW(char, strlen(opt_arg) + strlen("0"));
			strcpy(bootclasspath, opt_arg);
			break;

		case OPT_BOOTCLASSPATH_A:
			/* append to end of bootclasspath */
			cplen = strlen(bootclasspath);

			bootclasspath = MREALLOC(bootclasspath,
									 char,
									 cplen,
									 cplen + strlen(":") +
									 strlen(opt_arg) + strlen("0"));

			strcat(bootclasspath, ":");
			strcat(bootclasspath, opt_arg);
			break;

		case OPT_BOOTCLASSPATH_P:
			/* prepend in front of bootclasspath */
			cp = bootclasspath;
			cplen = strlen(cp);

			bootclasspath = MNEW(char, strlen(opt_arg) + strlen(":") +
								 cplen + strlen("0"));

			strcpy(bootclasspath, opt_arg);
			strcat(bootclasspath, ":");
			strcat(bootclasspath, cp);

			MFREE(cp, char, cplen);
			break;

#if defined(ENABLE_JVMTI)
		case OPT_DEBUG:
			dbg = true;
			transport = opt_arg;
			break;

		case OPT_AGENTPATH:
		case OPT_AGENTLIB:
			set_jvmti_phase(JVMTI_PHASE_ONLOAD);
			agentload(opt_arg);
			set_jvmti_phase(JVMTI_PHASE_PRIMORDIAL);
			break;
#endif
			
		case OPT_MX:
		case OPT_MS:
		case OPT_SS:
			{
				char c;
				c = opt_arg[strlen(opt_arg) - 1];

				if ((c == 'k') || (c == 'K')) {
					j = atoi(opt_arg) * 1024;

				} else if ((c == 'm') || (c == 'M')) {
					j = atoi(opt_arg) * 1024 * 1024;

				} else
					j = atoi(opt_arg);

				if (opt == OPT_MX)
					heapmaxsize = j;
				else if (opt == OPT_MS)
					heapstartsize = j;
				else
					opt_stacksize = j;
			}
			break;

		case OPT_VERBOSE1:
			opt_verbose = true;
			break;

		case OPT_VERBOSE:
			if (strcmp("class", opt_arg) == 0)
				opt_verboseclass = true;

			else if (strcmp("gc", opt_arg) == 0)
				opt_verbosegc = true;

			else if (strcmp("jni", opt_arg) == 0)
				opt_verbosejni = true;

			else if (strcmp("call", opt_arg) == 0)
				opt_verbosecall = true;

			else if (strcmp("jit", opt_arg) == 0) {
				opt_verbose = true;
				loadverbose = true;
				linkverbose = true;
				initverbose = true;
				compileverbose = true;
			}
			else if (strcmp("exception", opt_arg) == 0)
				opt_verboseexception = true;
			break;

#ifdef TYPECHECK_VERBOSE
		case OPT_VERBOSETC:
			typecheckverbose = true;
			break;
#endif
				
		case OPT_VERSION:
			version();
			exit(0);
			break;

		case OPT_FULLVERSION:
			fullversion();
			break;

		case OPT_SHOWVERSION:
			version();
			break;

		case OPT_NOIEEE:
			opt_noieee = true;
			break;

		case OPT_NOVERIFY:
			opt_verify = false;
			break;

		case OPT_LIBERALUTF:
			opt_liberalutf = true;
			break;

		case OPT_SOFTNULL:
			checknull = true;
			break;

		case OPT_TIME:
			getcompilingtime = true;
			getloadingtime = true;
			break;
					
#if defined(ENABLE_STATISTICS)
		case OPT_STAT:
			opt_stat = true;
			break;
#endif
					
		case OPT_LOG:
			log_init(opt_arg);
			break;
			
		case OPT_CHECK:
			for (j = 0; j < strlen(opt_arg); j++) {
				switch (opt_arg[j]) {
				case 'b':
					checkbounds = false;
					break;
				case 's':
					checksync = false;
					break;
				default:
					usage();
				}
			}
			break;
			
		case OPT_LOAD:
			opt_run = false;
			makeinitializations = false;
			break;

		case OPT_EAGER:
			opt_eager = true;
			break;

		case OPT_METHOD:
			opt_run = false;
			opt_method = opt_arg;
			makeinitializations = false;
			break;
         		
		case OPT_SIGNATURE:
			opt_signature = opt_arg;
			break;
         		
		case OPT_ALL:
			compileall = true;
			opt_run = false;
			makeinitializations = false;
			break;
         		
		case OPT_SHOW:       /* Display options */
			for (j = 0; j < strlen(opt_arg); j++) {		
				switch (opt_arg[j]) {
				case 'a':
					opt_showdisassemble = true;
					compileverbose = true;
					break;
				case 'c':
					showconstantpool = true;
					break;
				case 'd':
					opt_showddatasegment = true;
					break;
				case 'e':
					opt_showexceptionstubs = true;
					break;
				case 'i':
					opt_showintermediate = true;
					compileverbose = true;
					break;
				case 'm':
					showmethods = true;
					break;
				case 'n':
					opt_shownativestub = true;
					break;
				case 'u':
					showutf = true;
					break;
				default:
					usage();
				}
			}
			break;
			
		case OPT_OLOOP:
			opt_loops = true;
			break;

		case OPT_INLINING:
			for (j = 0; j < strlen(opt_arg); j++) {		
				switch (opt_arg[j]) {
				case 'n':
					/* define in options.h; Used in main.c, jit.c
					   & inline.c inlining is currently
					   deactivated */
					break;
				case 'v':
					inlinevirtuals = true;
					break;
				case 'e':
					inlineexceptions = true;
					break;
				case 'p':
					inlineparamopt = true;
					break;
				case 'o':
					inlineoutsiders = true;
					break;
				default:
					usage();
				}
			}
			break;

#if defined(ENABLE_IFCONV)
		case OPT_IFCONV:
			opt_ifconv = true;
			break;
#endif

#if defined(ENABLE_LSRA)
		case OPT_LSRA:
			opt_lsra = true;
			break;
#endif

		case OPT_HELP:
			usage();
			break;

		case OPT_X:
			Xusage();
			break;

		case OPT_PROF_OPTION:
			/* use <= to get the last \0 too */

			for (j = 0, k = 0; j <= strlen(opt_arg); j++) {
				if (opt_arg[j] == ',')
					opt_arg[j] = '\0';

				if (opt_arg[j] == '\0') {
					if (strcmp("bb", opt_arg + k) == 0)
						opt_prof_bb = true;

					else {
						printf("Unknown option: -Xprof:%s\n", opt_arg + k);
						usage();
					}

					/* set k to next char */

					k = j + 1;
				}
			}
			/* fall through */

		case OPT_PROF:
			opt_prof = true;
			break;

		case OPT_JIT:
#if defined(ENABLE_JIT)
			opt_jit = true;
#else
			printf("-Xjit option not enabled.\n");
			exit(1);
#endif
			break;

		case OPT_INTRP:
#if defined(ENABLE_INTRP)
			opt_intrp = true;
#else
			printf("-Xint option not enabled.\n");
			exit(1);
#endif
			break;

#if defined(ENABLE_INTRP)
		case OPT_STATIC_SUPERS:
			opt_static_supers = atoi(opt_arg);
			break;

		case OPT_NO_DYNAMIC:
			opt_no_dynamic = true;
			break;

		case OPT_NO_REPLICATION:
			opt_no_replication = true;
			break;

		case OPT_NO_QUICKSUPER:
			opt_no_quicksuper = true;
			break;

		case OPT_TRACE:
			vm_debug = true;
			break;
#endif

		default:
			printf("Unknown option: %s\n",
				   vm_args->options[opt_index].optionString);
			usage();
		}
	}


	/* get the main class *****************************************************/

	if (opt_index < vm_args->nOptions) {
		mainstring = vm_args->options[opt_index++].optionString;

		if (opt_jar == true) {

			/* prepend the jar file to the classpath (if any) */

			if (opt_jar == true) {
				/* put jarfile in classpath */

				cp = classpath;

				classpath = MNEW(char, strlen(mainstring) + strlen(":") +
								 strlen(classpath) + strlen("0"));

				strcpy(classpath, mainstring);
				strcat(classpath, ":");
				strcat(classpath, cp);
		
				MFREE(cp, char, strlen(cp));

			} else {
				/* replace .'s with /'s in classname */

				for (i = strlen(mainstring) - 1; i >= 0; i--)
					if (mainstring[i] == '.')
						mainstring[i] = '/';
			}
		}
	}


	/* initialize this JVM ****************************************************/

	vm_initializing = true;

	/* initialize the garbage collector */

	gc_init(heapmaxsize, heapstartsize);

#if defined(ENABLE_INTRP)
	/* Allocate main thread stack on the Java heap. */

	if (opt_intrp) {
		intrp_main_stack = GCMNEW(u1, opt_stacksize);
		MSET(intrp_main_stack, 0, u1, opt_stacksize);
	}
#endif

#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
  	threads_preinit();
#endif
	initLocks();
#endif

	/* initialize the string hashtable stuff: lock (must be done
	   _after_ threads_preinit) */

	if (!string_init())
		throw_main_exception_exit();

	/* initialize the utf8 hashtable stuff: lock, often used utf8
	   strings (must be done _after_ threads_preinit) */

	if (!utf8_init())
		throw_main_exception_exit();

	/* initialize the classcache hashtable stuff: lock, hashtable
	   (must be done _after_ threads_preinit) */

	if (!classcache_init())
		throw_main_exception_exit();

	/* initialize the loader with bootclasspath (must be done _after_
	   thread_preinit) */

	if (!suck_init())
		throw_main_exception_exit();

	suck_add_from_property("java.endorsed.dirs");
	suck_add(bootclasspath);

	/* initialize the memory subsystem (must be done _after_
	   threads_preinit) */

	if (!memory_init())
		throw_main_exception_exit();

	/* initialize the finalizer stuff (must be done _after_
	   threads_preinit) */

	if (!finalizer_init())
		throw_main_exception_exit();

	/* install architecture dependent signal handler used for exceptions */

	signal_init();

	/* initialize the codegen subsystems */

	codegen_init();

	/* initializes jit compiler */

	jit_init();

	/* machine dependent initialization */

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp)
		intrp_md_init();
	else
# endif
		md_init();
#else
	intrp_md_init();
#endif

	/* initialize the loader subsystems (must be done _after_
       classcache_init) */

	if (!loader_init((u1 *) stackbottom))
		throw_main_exception_exit();

	if (!linker_init())
		throw_main_exception_exit();

	if (!native_init())
		throw_main_exception_exit();

	if (!exceptions_init())
		throw_main_exception_exit();

	if (!builtin_init())
		throw_main_exception_exit();

#if defined(USE_THREADS)
  	if (!threads_init((u1 *) stackbottom))
		throw_main_exception_exit();
#endif

	/* That's important, otherwise we get into trouble, if the Runtime
	   static initializer is called before (circular dependency. This
	   is with classpath 0.09. Another important thing is, that this
	   has to happen after initThreads!!! */

	if (!initialize_class(class_java_lang_System))
		throw_main_exception_exit();

	/* JNI init creates a Java object (this means running Java code) */

	if (!jni_init())
		throw_main_exception_exit();

	/* initialize profiling */

	if (!profile_init())
		throw_main_exception_exit();
		
#if defined(USE_THREADS)
	/* finally, start the finalizer thread */

	if (!finalizer_start_thread())
		throw_main_exception_exit();

	/* start the profile sampling thread */

/* 	if (!profile_start_thread()) */
/* 		throw_main_exception_exit(); */
#endif

	/* increment the number of VMs */

	vms++;

	/* initialization is done */

	vm_initializing = false;

	/* everything's ok */

	return true;
}


/* vm_destroy ******************************************************************

   Unloads a Java VM and reclaims its resources.

*******************************************************************************/

s4 vm_destroy(JavaVM *vm)
{
#if defined(USE_THREADS)
#if defined(NATIVE_THREADS)
	joinAllThreads();
#else
	killThread(currentThread);
#endif
#endif

	/* everything's ok */

	return 0;
}


/* vm_exit *********************************************************************

   Calls java.lang.System.exit(I)V to exit the JavaVM correctly.

*******************************************************************************/

void vm_exit(s4 status)
{
	methodinfo *m;

	/* signal that we are exiting */

	vm_exiting = true;

	assert(class_java_lang_System);
	assert(class_java_lang_System->state & CLASS_LOADED);

#if defined(ENABLE_JVMTI)
	set_jvmti_phase(JVMTI_PHASE_DEAD);
	agentunload();
#endif

	if (!link_class(class_java_lang_System))
		throw_main_exception_exit();

	/* call java.lang.System.exit(I)V */

	m = class_resolveclassmethod(class_java_lang_System,
								 utf_new_char("exit"),
								 utf_int__void,
								 class_java_lang_Object,
								 true);
	
	if (!m)
		throw_main_exception_exit();

	/* call the exit function with passed exit status */

	ASM_CALLJAVAFUNCTION(m, (void *) (ptrint) status, NULL, NULL, NULL);

	/* this should never happen */

	if (*exceptionptr)
		throw_exception_exit();

	throw_cacao_exception_exit(string_java_lang_InternalError,
							   "System.exit(I)V returned without exception");
}


/* vm_shutdown *****************************************************************

   Terminates the system immediately without freeing memory explicitly
   (to be used only for abnormal termination).
	
*******************************************************************************/

void vm_shutdown(s4 status)
{
#if defined(ENABLE_JVMTI)
	agentunload();
#endif

	if (opt_verbose || getcompilingtime || opt_stat) {
		log_text("CACAO terminated by shutdown");
		dolog("Exit status: %d\n", (s4) status);
	}

	exit(status);
}


/* vm_exit_handler *************************************************************

   The exit_handler function is called upon program termination.

   ATTENTION: Don't free system resources here! Some threads may still
   be running as this is called from VMRuntime.exit(). The OS does the
   cleanup for us.

*******************************************************************************/

void vm_exit_handler(void)
{
#if !defined(NDEBUG)
	if (showmethods)
		class_showmethods(mainclass);

	if (showconstantpool)
		class_showconstantpool(mainclass);

	if (showutf)
		utf_show();

	if (opt_prof)
		profile_printstats();
#endif

#if defined(USE_THREADS) && !defined(NATIVE_THREADS)
	clear_thread_flags();		/* restores standard file descriptor
	                               flags */
#endif

	if (opt_verbose || getcompilingtime || opt_stat) {
		log_text("CACAO terminated");

#if defined(ENABLE_STATISTICS)
		if (opt_stat) {
			print_stats();
#ifdef TYPECHECK_STATISTICS
			typecheck_print_statistics(get_logfile());
#endif
		}

		mem_usagelog(1);

		if (getcompilingtime)
			print_times();
#endif
	}
	/* vm_print_profile(stderr);*/
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
