/* headers.c - main for header generation (cacaoh)

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

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

   Changes: Mark Probst
            Philipp Tomsich

   $Id: headers.c 1173 2004-06-16 14:56:18Z jowenn $

*/


#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"
#include "global.h"
#include "tables.h"
#include "loader.h"
#include "builtin.h"
#include "mm/boehm.h"
#include "toolbox/chain.h"
#include "toolbox/memory.h"
#include "toolbox/logging.h"
#include "nat/java_lang_String.h"


/* replace command line options */

bool verbose =  false;
bool verboseexception =  false;
bool compileall = false;
bool runverbose = false;
bool collectverbose = false;

bool loadverbose = false;
bool linkverbose = false;
bool initverbose = false;

bool opt_rt = false;            /* true if RTA parse should be used     RT-CO */
bool opt_xta = false;           /* true if XTA parse should be used    XTA-CO */
bool opt_vta = false;           /* true if VTA parse should be used    VTA-CO */
bool opt_verify = true;        /* true if classfiles should be verified      */
bool opt_liberalutf = false;   /* Don't check overlong UTF-8 sequences       */

bool opt_stat = false;
bool opt_eager = false;

bool showmethods = false;
bool showconstantpool = false;
bool showutf = false;

bool makeinitializations = true;

bool getloadingtime = false;
s8 loadingtime = 0;


/******* replace some external functions  *********/
 
functionptr native_findfunction(utf *cname, utf *mname, utf *desc, bool isstatic)
{ return NULL; }

java_objectheader *javastring_new(utf *text)
{ return NULL; }

utf *javastring_toutf(java_lang_String *string, bool isclassname)
{ return NULL; }


/* some exception stuff */

char *string_java_lang_IllegalMonitorStateException =
    "java/lang/IllegalMonitorStateException";

char *string_java_lang_NegativeArraySizeException =
    "java/lang/NegativeArraySizeException";


char *string_java_lang_AbstractMethodError =
    "java/lang/AbstractMethodError";

char *string_java_lang_ClassCircularityError =
    "java/lang/ClassCircularityError";

char *string_java_lang_ClassFormatError =
    "java/lang/ClassFormatError";

char *string_java_lang_ExceptionInInitializerError =
    "java/lang/ExceptionInInitializerError";

char *string_java_lang_IncompatibleClassChangeError =
    "java/lang/IncompatibleClassChangeError";

char *string_java_lang_InternalError =
    "java/lang/InternalError";

char *string_java_lang_LinkageError =
    "java/lang/LinkageError";

char *string_java_lang_NoClassDefFoundError =
    "java/lang/NoClassDefFoundError";

char *string_java_lang_NoSuchFieldError =
    "java/lang/NoSuchFieldError";

char *string_java_lang_NoSuchMethodError =
	"java/lang/NoSuchMethodError";

char *string_java_lang_OutOfMemoryError =
    "java/lang/OutOfMemoryError";


void throw_exception_exit() {}

void throw_cacao_exception_exit(char *exception, char *message)
{
	fprintf(stderr,
			"Exception in thread \"main\" %s: %s\n", exception, message);
	fflush(stderr);

	/* good bye! */
	exit(1);
}


void new_exception(char *classname) {}
void new_exception_message(char *classname, char *message) {}
void new_exception_throwable(char *classname, java_objectheader *t) {}
void new_exception_utfmessage(char *classname, utf *message)
{
	printf("%s: ", classname);
	utf_display(message);
	printf("\n");
	exit(1);
}

java_objectheader *literalstring_new(utf *u) { return NULL; }  


void literalstring_free(java_objectheader *o) { }
void stringtable_update() { }
void synchronize_caches() { }
void asm_call_jit_compiler() { }
void asm_calljavafunction() { }
s4 asm_builtin_checkcast(java_objectheader *obj, classinfo *class) { return 0; }

s4 asm_builtin_idiv(s4 a, s4 b) {return 0;}
s4 asm_builtin_irem(s4 a, s4 b) {return 0;}
s8 asm_builtin_ldiv(s8 a, s8 b) {return 0;}
s8 asm_builtin_lrem(s8 a, s8 b) {return 0;}

s4 asm_builtin_f2i(float a) { return 0; }
s8 asm_builtin_f2l(float a) { return 0; }
s4 asm_builtin_d2i(double a) { return 0; }
s8 asm_builtin_d2l(double a) { return 0; }

void asm_builtin_monitorenter(java_objectheader *o) {}
void asm_builtin_monitorexit(java_objectheader *o) {}

s4 asm_builtin_checkarraycast(java_objectheader *obj, vftbl *target) {return 0;}

#if defined(__MIPS__)
void atomic_swap() {}
void compare_and_swap() {}
#endif


#if defined(__I386__)
s4 asm_builtin_arrayinstanceof(java_objectheader *obj, classinfo *class) { return 0; }
void asm_builtin_newarray(s4 size, vftbl *arrayvftbl) {}
#endif

void asm_builtin_aastore(java_objectarray *a, s4 index, java_objectheader *o) {}

u1 *createcompilerstub(methodinfo *m) {return NULL;}
u1 *createnativestub(functionptr f, methodinfo *m) {return NULL;}
u1 *oldcreatenativestub(functionptr f, methodinfo *m) {return NULL;}

void removecompilerstub(u1 *stub) {}
void removenativestub(u1 *stub) {}

void asm_perform_threadswitch(u1 **from, u1 **to, u1 **stackTop) {}
u1* asm_initialize_thread_stack(void *func, u1 *stack) { return NULL; }
void thread_restartcriticalsection() {}
void asm_switchstackandcall() {}
void asm_handle_builtin_exception(classinfo *c) {}
void asm_getclassvalues_atomic() {}

#if defined(__DARWIN__)
int cacao_catch_Handler() {}
#endif

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
threadcritnode asm_criticalsections;
#endif

java_objectheader *native_new_and_init(void *p) { return NULL; }


/************************ global variables **********************/

THREADSPECIFIC java_objectheader *_exceptionptr;

static chain *nativemethod_chain;    /* chain with native methods     */
static chain *nativeclass_chain;		               /* chain with processed classes  */	
static chain *ident_chain;        /* chain with method and field names in current class */
static FILE *file = NULL;
static u4 outputsize;
static bool dopadding;


static void printIDpart(int c) 
{
	if ((c >= 'a' && c <= 'z')
		|| (c >= 'A' && c <= 'Z')
		|| (c >= '0' && c <= '9')
		|| (c == '_'))
		putc(c, file);
	else
		putc('_', file);
}


static void printID(utf *u)
{
	char *utf_ptr = u->text;
	int i;

	for (i = 0; i < utf_strlen(u); i++) 
		printIDpart(utf_nextu2(&utf_ptr));
}


static void addoutputsize (int len)
{
	u4 newsize,i;
	if (!dopadding) return;

	newsize = ALIGN(outputsize, len);
	
	for (i = outputsize; i < newsize; i++) fprintf(file, "   u1 pad%d\n", (int) i);
	outputsize = newsize;
}

static void printOverloadPart(utf *desc)
{
	char *utf_ptr=desc->text;
	u2 c;

	fprintf(file,"__");
	while ((c=utf_nextu2(&utf_ptr))!=')') {
		switch (c) {
			case 'I':
			case 'S':
			case 'B':
			case 'C':
			case 'Z':
			case 'J':
			case 'F':
			case 'D': 
				fprintf (file, "%c",(char)c);
				break;
			case '[':
				fprintf(file,"_3");
                           	break;
			case 'L':
				putc('L',file);
				while ( (c=utf_nextu2(&utf_ptr)) != ';')
					printIDpart (c);
				fprintf(file,"_2");
				break;
			case '(':
				break;
			default: panic ("invalid method descriptor");
		}
	}
}

static char *printtype(char *utf_ptr)
{
	u2 c;

	switch (utf_nextu2(&utf_ptr)) {
	case 'V': fprintf (file, "void");
		break;
	case 'I':
	case 'S':
	case 'B':
	case 'C':
	case 'Z': addoutputsize (4);
		fprintf (file, "s4");
		break;
	case 'J': addoutputsize (8);
		fprintf (file, "s8");
		break;
	case 'F': addoutputsize (4);
		fprintf (file, "float");
		break;
	case 'D': addoutputsize (8);
		fprintf (file, "double");
		break;
	case '[':
		addoutputsize ( sizeof(java_arrayheader*) ); 
		switch (utf_nextu2(&utf_ptr)) {
		case 'I':  fprintf (file, "java_intarray*"); break;
		case 'J':  fprintf (file, "java_longarray*"); break;
		case 'Z':  fprintf (file, "java_booleanarray*"); break;
		case 'B':  fprintf (file, "java_bytearray*"); break;
		case 'S':  fprintf (file, "java_shortarray*"); break;
		case 'C':  fprintf (file, "java_chararray*"); break;
		case 'F':  fprintf (file, "java_floatarray*"); break;
		case 'D':  fprintf (file, "java_doublearray*"); break;
				
		case '[':  fprintf (file, "java_objectarray*");					       
			while ((c = utf_nextu2(&utf_ptr)) == '[') ;
			if (c=='L') 
				while (utf_nextu2(&utf_ptr) != ';');
			break;
                           
		case 'L':  fprintf (file, "java_objectarray*");
			while ( utf_nextu2(&utf_ptr) != ';');
			break;
		default: panic ("invalid type descriptor");
		}
		break;
		
	case 'L': 
		addoutputsize ( sizeof(java_objectheader*));
		fprintf (file, "struct ");
		while ( (c = utf_nextu2(&utf_ptr)) != ';' ) printIDpart (c);   	 
		fprintf (file, "*");
		break;
					
	default:  panic ("Unknown type in field descriptor");
	}
	
	return utf_ptr;
}


/***** determine the number of entries of a utf string in the ident chain *****/

static int searchidentchain_utf(utf *ident) 
{
	utf *u = chain_first(ident_chain);     /* first element of list */
	int count = 0;

	while (u) {
		if (u==ident) count++;         /* string found */
		u = chain_next(ident_chain);   /* next element in list */ 
	}

	return count;
}


/************** print structure for direct access to objects ******************/

static void printfields(classinfo *c)
{
	u4 i;
	fieldinfo *f;
	int ident_count;
	
	if (!c) {
		addoutputsize(sizeof(java_objectheader));
		fprintf(file, "   java_objectheader header;\n");
		return;
	}
		
	printfields(c->super);
	
	for (i = 0; i < c->fieldscount; i++) {
		f = &(c->fields[i]);
		
		if (!(f->flags & ACC_STATIC)) {
			fprintf(file, "   ");
			printtype(f->descriptor->text);
			fprintf(file, " ");
			utf_fprint(file, f->name);

			/* rename multiple fieldnames */
			if ((ident_count = searchidentchain_utf(f->name)))
				fprintf(file, "%d", ident_count - 1);
			chain_addlast(ident_chain, f->name);	

			fprintf(file, ";\n");
		}
	}
}


/***************** store prototype for native method in file ******************/

static void printmethod(methodinfo *m)
{
	char *utf_ptr;
	u2 paramnum = 1;
	u2 ident_count;

	/* search for return-type in descriptor */	
	utf_ptr = m->descriptor->text;
	while (utf_nextu2(&utf_ptr) != ')');

	/* create remarks */
	fprintf(file, "\n/*\n * Class:     ");
	utf_fprint(file, m->class->name);
	fprintf(file, "\n * Method:    ");
	utf_fprint(file, m->name);
	fprintf(file, "\n * Signature: ");
	utf_fprint(file, m->descriptor);
	fprintf(file, "\n */\n");	

	/* create prototype */ 			
	fprintf(file, "JNIEXPORT ");				
	printtype(utf_ptr);
	fprintf(file, " JNICALL Java_");
	printID(m->class->name);           

	chain_addlast(ident_chain, m->name);	

	fprintf(file, "_");
	printID(m->name);
	if (m->nativelyoverloaded) printOverloadPart(m->descriptor);
	fprintf(file, "(JNIEnv *env");
	
	utf_ptr = m->descriptor->text + 1;
			
	if (!(m->flags & ACC_STATIC)) {
		fprintf(file, ", struct ");
		printID(m->class->name);
		fprintf(file, "* this");

	} else {
		fprintf(file, ", jclass clazz");
	}

	if ((*utf_ptr) != ')') fprintf(file, ", ");
			
	while ((*utf_ptr) != ')') {
		utf_ptr = printtype(utf_ptr);
		fprintf(file, " par%d", paramnum++);
		if ((*utf_ptr)!=')') fprintf(file, ", ");
	}
			
	fprintf(file, ");\n\n");
}


/******* remove package-name in fully-qualified classname *********************/

static void gen_header_filename(char *buffer, utf *u)
{
	int i;
  
	for (i = 0;i < utf_strlen(u); i++) {
		if ((u->text[i] == '/') || (u->text[i] == '$')) buffer[i] = '_';  /* convert '$' and '/' to '_' */
		else buffer[i] = u->text[i];
	}
	buffer[utf_strlen(u)] = '\0';
}


/* create headerfile for classes and store native methods in chain ************/

static void headerfile_generate(classinfo *c)
{
	char header_filename[1024] = "";
	char classname[1024]; 
	char uclassname[1024];
	u2 i;
	methodinfo *m;	      		
	u2 i2;
	methodinfo *m2;
	u2 nativelyoverloaded;	      		
		      
	/* store class in chain */		      
	chain_addlast(nativeclass_chain, c);
			    	
	/* open headerfile for class */
	gen_header_filename(classname, c->name);

	/* create chain for renaming fields */
	ident_chain = chain_new();
	
	sprintf(header_filename, "nat/%s.h", classname);
   	file = fopen(header_filename, "w");
   	if (!file) panic("Can not open file to store header information");

   	fprintf(file, "/* This file is machine generated, don't edit it !*/\n\n");

	/* convert to uppercase */
	for (i = 0; classname[i]; i++) {
		uclassname[i] = toupper(classname[i]);
	}
	uclassname[i] = '\0';

	fprintf(file, "#ifndef _%s_H\n#define _%s_H\n\n", uclassname, uclassname);

	/* create structure for direct access to objects */	
	fprintf(file, "/* Structure information for class: ");
	utf_fprint(file, c->name);
	fprintf(file, " */\n\n");
	fprintf(file, "typedef struct ");
	printID(c->name);							
	fprintf(file, " {\n");
	outputsize = 0;
	dopadding = true;

	printfields(c);

	fprintf(file, "} ");
	printID(c->name);
	fprintf(file, ";\n\n");

	/* create chain for renaming overloaded methods */
	chain_free(ident_chain);
	ident_chain = chain_new();

	/* create method-prototypes */
		      		
	/* find overloaded methods */
	for (i = 0; i < c->methodscount; i++) {

		m = &(c->methods[i]);

		if (!(m->flags & ACC_NATIVE)) continue;
		if (!m->nativelyoverloaded) {
			nativelyoverloaded=false;
			for (i2=i+1;i2<c->methodscount; i2++) {
				m2 = &(c->methods[i2]);
				if (!(m2->flags & ACC_NATIVE)) continue;
				if (m->name==m2->name) {
					m2->nativelyoverloaded=true;
					nativelyoverloaded=true;
				}
			}
			m->nativelyoverloaded=nativelyoverloaded;
		}

	}

	for (i = 0; i < c->methodscount; i++) {

		m = &(c->methods[i]);

		if (m->flags & ACC_NATIVE) {
			chain_addlast(nativemethod_chain, m);
			printmethod(m);
		}
	}

	chain_free(ident_chain);

	fprintf(file, "#endif\n\n");

   	fclose(file);
}


/******** print classname, '$' used to seperate inner-class name ***********/

void print_classname(classinfo *clazz)
{
	utf *u = clazz->name;
    char *endpos  = u->text + u->blength;
    char *utf_ptr = u->text; 
	u2 c;

    while (utf_ptr < endpos) {
		if ((c = utf_nextu2(&utf_ptr)) == '_') {
			putc('$', file);

		} else {
			putc(c, file);
		}
	}
} 


/*************** create table for locating native functions ****************/

static void printnativetableentry(methodinfo *m)
{
	fprintf(file, "   { \"");
	print_classname(m->class);
	fprintf(file, "\",\n     \"");
	utf_fprint(file, m->name);
	fprintf(file, "\",\n     \"");
	utf_fprint(file, m->descriptor);
	fprintf(file, "\",\n     ");

	if ((m->flags & ACC_STATIC) != 0)
		fprintf(file, "true");
	else
		fprintf(file, "false");

	fprintf(file, ",\n     ");
	fprintf(file, "(functionptr) Java_");
	printID(m->class->name);
	fprintf(file,"_");
	printID(m->name);
	if (m->nativelyoverloaded) printOverloadPart(m->descriptor);
	fprintf(file,"\n   },\n");
}


/***************************************************************************

	create the nativetypes-headerfile which includes 
	the headerfiles of the classes stored in the classes-chain 

****************************************************************************/

static void headers_finish()
{
	methodinfo *m;
	classinfo *c;
	char classname[1024];
	
	file = fopen("nativetypes.hh", "w");

	if (!file)
		panic("Can not open file 'native.h' to store header information");
	
	fprintf(file, "/* Headerfile for native methods: nativetypes.hh */\n");
	fprintf(file, "/* This file is machine generated, don't edit it !*/\n\n"); 	
	fprintf(file, "\n/* include native-Headerfiles */\n\n");
			
	c = chain_first(nativeclass_chain);
	while (c) {
		dopadding = false;
		gen_header_filename(classname, c->name);										    													
		fprintf(file, "#include \"nat/%s.h\"\n", classname);		
		c = chain_next(nativeclass_chain);		
	}

    fclose(file);
	chain_free(nativeclass_chain);
	
	/* create table of native-methods */

	file = fopen("nativetable.hh", "w");

	if (!file)
		panic("Can not open file 'nativetable' to store native-link-table");

	fprintf(file, "/* Table of native methods: nativetables.hh */\n");
	fprintf(file, "/* This file is machine generated, don't edit it !*/\n\n"); 

	while ((m = chain_first(nativemethod_chain)) != NULL) {
		chain_remove(nativemethod_chain);		
		printnativetableentry(m);
	}
		
	chain_free(nativemethod_chain);
	fclose(file);
}


/******************** internal function: print_usage ************************

Prints usage information for the JAVA header generator to stdout.

***************************************************************************/

static void print_usage()
{
	printf("Usage: cacaoh class [class..]\n");
}   



/************************** Function: main *******************************

   Main program.
   
**************************************************************************/

int main(int argc, char **argv)
{
	s4 i,a;
	char *cp;
	classinfo *topclass;
		

	/********** internal (only used by main) *****************************/
   
	char classpath[500] = "";
	char offsets_filename[1024] = ""; /* phil */
	u4 heapmaxsize = 1000000;
	u4 heapstartsize = 100000;

	/*********** options so only headers are generated *******************/
   
	makeinitializations = false;
   

	/************ Collect some info from the environment *****************/

	cp = getenv("CLASSPATH");
	if (cp) {
		strcpy(classpath + strlen(classpath), ":");
		strcpy(classpath + strlen(classpath), cp);
	}

	if (argc < 2) {
   		print_usage();
   		exit(10);
	}


	/**************************** Program start **************************/

	log_init(NULL);
	log_text("Java - header-generator started"); 
	
	sprintf(offsets_filename, "jit/%s/offsets.h", ARCH_DIR);
	file = fopen(offsets_filename, "w");
	if (file == NULL) {
		fprintf(stderr, "Can not open file '%s' for write", offsets_filename);
		exit(-1);
	}
	
	fprintf(file, "/* This file is machine generated, don't edit it !*/\n\n"); 

	fprintf(file, "#define offobjvftbl    %3d\n", (int) OFFSET(java_objectheader, vftbl));
	fprintf(file, "#define offarraysize   %3d\n", (int) OFFSET(java_arrayheader, size));
	fprintf(file, "#define offobjarrdata  %3d\n\n", (int) OFFSET(java_objectarray, data[0]));
	fprintf(file, "#define offbaseval     %3d\n", (int) OFFSET(vftbl, baseval));
	fprintf(file, "#define offdiffval     %3d\n\n", (int) OFFSET(vftbl, diffval));
	fprintf(file, "#define offclass     %3d\n\n", (int) OFFSET(vftbl, class));

	fprintf(file, "#define offclassvftbl  %3d\n", (int) OFFSET(classinfo, vftbl));
	fprintf(file, "#define offclassinit   %3d\n", (int) OFFSET(classinfo, initialized));
	fprintf(file, "#define offclassloaded %3d\n", (int) OFFSET(classinfo, loaded));
	fprintf(file, "#define offclasslinked %3d\n\n", (int) OFFSET(classinfo, linked));

	fprintf(file, "#define offjniitemtype %3d\n", (int) OFFSET(jni_callblock, itemtype));
	fprintf(file, "#define offjniitem     %3d\n", (int) OFFSET(jni_callblock, item));
	fprintf(file, "#define sizejniblock   %3d\n\n", (int) sizeof(jni_callblock));

	fprintf(file, "#define offclassmethodinfo   %3d\n\n", (int) OFFSET(methodinfo,class));

	fprintf(file, "#define offmethodnative %3d\n", (int) OFFSET(native_stackframeinfo,method));
/*	fprintf(file, "#define offreturnfromnative %3d\n", (int) OFFSET(native_stackframeinfo,returnFromNative));
	fprintf(file, "#define offaddrreturnfromnative %3d\n", (int) OFFSET(native_stackframeinfo,addrReturnFromNative));
	fprintf(file, "#define offprevnative %3d\n", (int) OFFSET(native_stackframeinfo,prev));
	fprintf(file, "#define offnextnative %3d\n", (int) OFFSET(native_stackframeinfo,next));*/

	fprintf(file, "\n");
	fprintf(file, "#define offcast_super_baseval  %3d\n", (int) OFFSET(castinfo, super_baseval));
	fprintf(file, "#define offcast_super_diffval  %3d\n", (int) OFFSET(castinfo, super_diffval));
	fprintf(file, "#define offcast_sub_baseval    %3d\n", (int) OFFSET(castinfo, sub_baseval));

	fclose(file);

	/* initialize the garbage collector */
	gc_init(heapmaxsize, heapstartsize);

	suck_init(classpath);
   
	tables_init();

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
	initThreadsEarly();
#endif
	loader_init();


	/*********************** Load JAVA classes  **************************/
   	
	nativemethod_chain = chain_new();
	nativeclass_chain = chain_new();
	
	for (a = 1; a < argc; a++) {
   		cp = argv[a];

		/* convert classname */
   		for (i = strlen(cp) - 1; i >= 0; i--) {
			switch (cp[i]) {
			case '.': cp[i]='/';
				break;
			case '_': cp[i]='$';    
  	 		}
		}
	
/*  		topclass = loader_load(utf_new_char(cp)); */
		topclass = class_load(class_new(utf_new_char(cp)));
		class_link(topclass);

        headerfile_generate(topclass);
	}

	headers_finish();

	/************************ Release all resources **********************/

	loader_close();
	tables_close(literalstring_free);

	/* Print "finished" message */

	log_text("Java - header-generator stopped");
	log_cputime();
	mem_usagelog(1);
	
	return 0;
}


void setVMClassField(classinfo *c)
{
}


void *Java_java_lang_VMObject_clone(void *env, void *clazz, void * this) {
	return 0;
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
