/* src/vm/jit/stacktrace.c

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

   Changes: Christian Thalinger

   $Id: stacktrace.c 3053 2005-07-18 21:55:31Z twisti $

*/


#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "asmoffsets.h"

#include "mm/boehm.h"
#include "native/native.h"

#include "vm/global.h"                   /* required here for native includes */
#include "native/include/java_lang_ClassLoader.h"

#include "toolbox/logging.h"
#include "vm/builtin.h"
#include "vm/class.h"
#include "vm/exceptions.h"
#include "vm/loader.h"
#include "vm/stringlocal.h"
#include "vm/tables.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen.inc.h"


#undef JWDEBUG
#undef JWDEBUG2
#undef JWDEBUG3


/* lineNumberTableEntry *******************************************************/

/* Keep the type of line the same as the pointer type, otherwise we run into  */
/* alignment troubles (like on MIPS64).                                       */

typedef struct lineNumberTableEntry {
	ptrint  line;
	u1     *pc;
} lineNumberTableEntry;


typedef struct lineNumberTableEntryInlineBegin {
	/* this should have the same layout and size as the lineNumberTableEntry */
	ptrint      lineNrOuter;
	methodinfo *method;
} lineNumberTableEntryInlineBegin;


typedef void(*CacaoStackTraceCollector)(void **,stackTraceBuffer*);

#define BLOCK_INITIALSIZE 40
#define BLOCK_SIZEINCREMENT 40


/* stacktrace_create_inline_stackframeinfo *************************************

   Creates an stackframe info structure for an inline exception stub.

*******************************************************************************/

void stacktrace_create_inline_stackframeinfo(stackframeinfo *sfi, u1 *pv,
											 u1 *sp, functionptr ra,
											 functionptr xpc)
{
	void **osfi;

	/* get current stackframe info pointer */

	osfi = builtin_asm_get_stackframeinfo();

	/* fill new stackframe info structure */

	sfi->oldThreadspecificHeadValue = *osfi;
	sfi->addressOfThreadspecificHead = osfi;
	sfi->method = NULL;
	sfi->pv = pv;
	sfi->sp = sp;
	sfi->ra = ra;
	sfi->xpc = xpc;

	/* store new stackframe info pointer */

	*osfi = sfi;
}


/* stacktrace_create_extern_stackframeinfo *************************************

   Creates an stackframe info structure for an extern exception
   (hardware or assembler).

*******************************************************************************/

void stacktrace_create_extern_stackframeinfo(stackframeinfo *sfi, u1 *pv,
											 u1 *sp, functionptr ra,
											 functionptr xpc)
{
	void **osfi;
#if !defined(__I386__) && !defined(__X86_64__)
	bool   isleafmethod;
#endif
	s4     framesize;

	/* get current stackframe info pointer */

	osfi = builtin_asm_get_stackframeinfo();

	/* sometimes we don't have pv handy (e.g. in asmpart.S: */
	/* L_asm_call_jit_compiler_exception) */

	if (pv == NULL)
		pv = (u1 *) (ptrint) codegen_findmethod(ra);

#if defined(__I386__) || defined(__X86_64__)
	/* On i386 and x86_64 we always have to get the return address from the */
	/* stack. */

	framesize = *((u4 *) (pv + FrameSize));

	ra = md_stacktrace_get_returnaddress(sp, framesize);
#else
	/* If the method is a non-leaf function, we need to get the return        */
	/* address from the stack. For leaf functions the return address is set   */
	/* correctly. This makes the assembler and the signal handler code        */
	/* simpler.                                                               */

	isleafmethod = *((s4 *) (pv + IsLeaf));

	if (!isleafmethod) {
		framesize = *((u4 *) (pv + FrameSize));

		ra = md_stacktrace_get_returnaddress(sp, framesize);
	}
#endif

	/* fill new stackframe info structure */

	sfi->oldThreadspecificHeadValue = *osfi;
	sfi->addressOfThreadspecificHead = osfi;
	sfi->method = NULL;
	sfi->pv = pv;
	sfi->sp = sp;
	sfi->ra = ra;
	sfi->xpc = xpc;

	/* store new stackframe info pointer */

	*osfi = sfi;
}


/* stacktrace_create_native_stackframeinfo *************************************

   Creates a stackframe info structure for a native stub.

*******************************************************************************/

void stacktrace_create_native_stackframeinfo(stackframeinfo *sfi, u1 *pv,
											 u1 *sp, functionptr ra)
{
	void       **osfi;
	methodinfo  *m;

	/* get methodinfo pointer from data segment */

	m = *((methodinfo **) (pv + MethodPointer));

	/* get current stackframe info pointer */

	osfi = builtin_asm_get_stackframeinfo();

	/* fill new stackframe info structure */

	sfi->oldThreadspecificHeadValue = *osfi;
	sfi->addressOfThreadspecificHead = osfi;
	sfi->method = m;
	sfi->pv = NULL;
	sfi->sp = sp;
	sfi->ra = ra;
	sfi->xpc = NULL;

	/* store new stackframe info pointer */

	*osfi = sfi;
}


/* stacktrace_remove_stackframeinfo ********************************************

   XXX

*******************************************************************************/

void stacktrace_remove_stackframeinfo(stackframeinfo *sfi)
{
	void **osfi;

	/* get address of pointer */

	osfi = sfi->addressOfThreadspecificHead;

	/* restore the old pointer */

	*osfi = sfi->oldThreadspecificHeadValue;
}


/* stacktrace_inline_arithmeticexception ***************************************

   Creates an ArithemticException for inline stub.

*******************************************************************************/

java_objectheader *stacktrace_inline_arithmeticexception(u1 *pv, u1 *sp,
														 functionptr ra,
														 functionptr xpc)
{
	stackframeinfo     sfi;
	java_objectheader *o;

	/* create stackframeinfo */

	stacktrace_create_inline_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* create exception */

	o = new_arithmeticexception();

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* stacktrace_inline_arrayindexoutofboundsexception ****************************

   Creates an ArrayIndexOutOfBoundsException for inline stub.

*******************************************************************************/

java_objectheader *stacktrace_inline_arrayindexoutofboundsexception(u1 *pv,
																	u1 *sp,
																	functionptr ra,
																	functionptr xpc,
																	s4 index)
{
	stackframeinfo     sfi;
	java_objectheader *o;

	/* create stackframeinfo */

	stacktrace_create_inline_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* create exception */

	o = new_arrayindexoutofboundsexception(index);

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* stacktrace_inline_arraystoreexception ***************************************

   Creates an ArrayStoreException for inline stub.

*******************************************************************************/

java_objectheader *stacktrace_inline_arraystoreexception(u1 *pv, u1 *sp,
														 functionptr ra,
														 functionptr xpc)
{
	stackframeinfo     sfi;
	java_objectheader *o;

	/* create stackframeinfo */

	stacktrace_create_inline_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* create exception */

	o = new_arraystoreexception();

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* stacktrace_inline_classcastexception ****************************************

   Creates an ClassCastException for inline stub.

*******************************************************************************/

java_objectheader *stacktrace_inline_classcastexception(u1 *pv, u1 *sp,
														functionptr ra,
														functionptr xpc)
{
	stackframeinfo     sfi;
	java_objectheader *o;

	/* create stackframeinfo */

	stacktrace_create_inline_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* create exception */

	o = new_classcastexception();

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* stacktrace_inline_negativearraysizeexception ********************************

   Creates an NegativeArraySizeException for inline stub.

*******************************************************************************/

java_objectheader *stacktrace_inline_negativearraysizeexception(u1 *pv, u1 *sp,
																functionptr ra,
																functionptr xpc)
{
	stackframeinfo     sfi;
	java_objectheader *o;

	/* create stackframeinfo */

	stacktrace_create_inline_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* create exception */

	o = new_negativearraysizeexception();

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* stacktrace_inline_nullpointerexception **************************************

   Creates an NullPointerException for inline stub.

*******************************************************************************/

java_objectheader *stacktrace_inline_nullpointerexception(u1 *pv, u1 *sp,
														  functionptr ra,
														  functionptr xpc)
{
	stackframeinfo     sfi;
	java_objectheader *o;

	/* create stackframeinfo */

	stacktrace_create_inline_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* create exception */

	o = new_nullpointerexception();

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* stacktrace_hardware_arithmeticexception *************************************

   Creates an ArithemticException for inline stub.

*******************************************************************************/

java_objectheader *stacktrace_hardware_arithmeticexception(u1 *pv, u1 *sp,
														   functionptr ra,
														   functionptr xpc)
{
	stackframeinfo     sfi;
	java_objectheader *o;

	/* create stackframeinfo */

	stacktrace_create_extern_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* create exception */

	o = new_arithmeticexception();

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* stacktrace_hardware_nullpointerexception ************************************

   Creates an NullPointerException for the SIGSEGV signal handler.

*******************************************************************************/

java_objectheader *stacktrace_hardware_nullpointerexception(u1 *pv, u1 *sp,
															functionptr ra,
															functionptr xpc)
{
	stackframeinfo     sfi;
	java_objectheader *o;

	/* create stackframeinfo */

	stacktrace_create_extern_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* create exception */

	o = new_nullpointerexception();

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* stacktrace_inline_fillInStackTrace ******************************************

   Fills in the correct stacktrace into an existing exception object
   (this one is for inline exception stubs).

*******************************************************************************/

java_objectheader *stacktrace_inline_fillInStackTrace(u1 *pv, u1 *sp,
													  functionptr ra,
													  functionptr xpc)
{
	stackframeinfo     sfi;
	java_objectheader *o;
	methodinfo        *m;

	/* create stackframeinfo */

	stacktrace_create_inline_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* get exception */

	o = *exceptionptr;

	/* clear exception */

	*exceptionptr = NULL;

	/* resolve methodinfo pointer from exception object */

	m = class_resolvemethod(o->vftbl->class,
							utf_fillInStackTrace,
							utf_void__java_lang_Throwable);

	/* call function */

	asm_calljavafunction(m, o, NULL, NULL, NULL);

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* stacktrace_extern_fillInStackTrace ******************************************

   Fills in the correct stacktrace into an existing exception object
   (this one is for calling from assembler code).

*******************************************************************************/

java_objectheader *stacktrace_extern_fillInStackTrace(u1 *pv, u1 *sp,
													  functionptr ra,
													  functionptr xpc)
{
	java_objectheader *o;
	stackframeinfo     sfi;
	methodinfo        *m;

	/* get exception */

	o = *exceptionptr;

	/* create stackframeinfo */

	stacktrace_create_extern_stackframeinfo(&sfi, pv, sp, ra, xpc);

	/* clear exception */

	*exceptionptr = NULL;

	/* resolve methodinfo pointer from exception object */

	m = class_resolvemethod(o->vftbl->class,
							utf_fillInStackTrace,
							utf_void__java_lang_Throwable);

	/* call function */

	asm_calljavafunction(m, o, NULL, NULL, NULL);

	/* remove stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	return o;
}


/* addEntry ********************************************************************

   XXX

*******************************************************************************/

static void addEntry(stackTraceBuffer *buffer, methodinfo *method, u2 line)
{
	if (buffer->size > buffer->full) {
		stacktraceelement *tmp = &(buffer->start[buffer->full]);

		tmp->method = method;
		tmp->linenumber = line;
		buffer->full = buffer->full + 1;

	} else {
		stacktraceelement *newBuffer;

		newBuffer =
			(stacktraceelement *) malloc((buffer->size + BLOCK_SIZEINCREMENT) *
										 sizeof(stacktraceelement));

		if (newBuffer == 0) {
			log_text("OOM during stacktrace creation");
			assert(0);
		}

		memcpy(newBuffer, buffer->start, buffer->size * sizeof(stacktraceelement));
		if (buffer->needsFree)
			free(buffer->start);

		buffer->start = newBuffer;
		buffer->size = buffer->size + BLOCK_SIZEINCREMENT;
		buffer->needsFree = 1;

		addEntry(buffer, method, line);
	}
}


/* stacktrace_fillInStackTrace_methodRecursive *********************************

   XXX

*******************************************************************************/

static bool stacktrace_fillInStackTrace_methodRecursive(stackTraceBuffer *buffer,
														methodinfo *method,
														lineNumberTableEntry *lntentry,
														s4 lntsize,
														u1 *pc)
{
#if 0
	lineNumberTableEntryInlineBegin *lntinline;
#endif

	/* find the line number for the specified pc (going backwards) */

	for (; lntsize > 0; lntsize--, lntentry--) {
		/* did we reach the current line? */

		if (pc >= lntentry->pc) {
			/* check for special inline entries */

			switch (lntentry->line) {
#if 0
			/* XXX TWISTI we have to think about this inline stuff again */

			case -1: /* begin of inlined method */
				lntinline = (lineNumberTableEntryInlineBegin *) (--lntentry);
				lntentry++;
				lntsize--; lntsize--;
				if (stacktrace_fillInStackTrace_methodRecursive(buffer,
																ilStart->method,
																ent,
																&ent,
																&ahead,
																pc)) {
					addEntry(buffer, method, ilStart->lineNrOuter);

					return true;
				}
				break;

			case -2: /* end of inlined method */
				*entry = ent;
				*entriesAhead = ahead;
				return false;
				break;
#endif

			default:
				addEntry(buffer, method, lntentry->line);
				return true;
			}
		}
	}

	/* this should not happen */

	assert(0);

	return false;
}


/* stacktrace_fillInStackTrace_method ******************************************

   XXX

*******************************************************************************/

static void stacktrace_fillInStackTrace_method(stackTraceBuffer *buffer,
											   methodinfo *method, u1 *pv,
											   u1 *pc)
{
	ptrint                lntsize;      /* size of line number table          */
	u1                   *lntstart;     /* start of line number table         */
	lineNumberTableEntry *lntentry;     /* points to last entry in the table  */

	/* get size of line number table */

	lntsize  = *((ptrint *) (pv + LineNumberTableSize));
	lntstart = *((u1 **)    (pv + LineNumberTableStart));

	/* subtract the size of the line number entry of the structure, since the */
	/* line number table start points to the pc */

	lntentry = (lineNumberTableEntry *) (lntstart - SIZEOF_VOID_P);

	if (lntsize == 0) {
		/* this happens when an exception is thrown in the native stub */

		addEntry(buffer, method, 0);

	} else {
		if (!stacktrace_fillInStackTrace_methodRecursive(buffer,
														 method,
														 lntentry,
														 lntsize,
														 pc)) {
			log_text("Trace point not found in suspected method");
			assert(0);
		}
	}
}


/* cacao_stacktrace_fillInStackTrace *******************************************

   XXX

*******************************************************************************/

void cacao_stacktrace_fillInStackTrace(void **target,
									   CacaoStackTraceCollector coll)
{
	stacktraceelement primaryBlock[BLOCK_INITIALSIZE*sizeof(stacktraceelement)];
	stackTraceBuffer  buffer;
	stackframeinfo   *info;
	methodinfo       *m;
	u1               *pv;
	u1               *sp;
	u4                framesize;
	functionptr       ra;
	functionptr       xpc;

	/* prevent compiler warnings */

	pv = NULL;
	sp = NULL;
	ra = NULL;

	/* In most cases this should be enough -> one malloc less. I don't think  */
	/* temporary data should be allocated with the GC, only the result.       */

	buffer.needsFree = 0;
	buffer.start = primaryBlock;
	buffer.size = BLOCK_INITIALSIZE; /*  *sizeof(stacktraceelement); */
	buffer.full = 0;

	/* the first element in the stackframe chain must always be a native      */
	/* stackframeinfo (VMThrowable.fillInStackTrace is a native function)     */

	info = *((void **) builtin_asm_get_stackframeinfo());

	if (!info) {
		*target = NULL;
		return;
	}

#define PRINTMETHODS 0

#if PRINTMETHODS
	printf("\n\nfillInStackTrace start:\n");
#endif

	/* loop while we have a method pointer (asm_calljavafunction has NULL) or */
	/* there is a stackframeinfo in the chain                                 */

	m = NULL;

	while (m || info) {
		/* m == NULL should only happen for the first time and inline         */
		/* stackframe infos, like from the exception stubs or the patcher     */
		/* wrapper                                                            */

		if (m == NULL) {
			/* for native stub stackframe infos, pv is always NULL */

			if (info->pv == NULL) {
				/* get methodinfo, sp and ra from the current stackframe info */

				m = info->method;
				sp = info->sp;          /* sp of parent Java function         */
				ra = info->ra;

				if (m)
					addEntry(&buffer, m, 0);

#if PRINTMETHODS
				utf_display_classname(m->class->name);
				printf(".");
				utf_display(m->name);
				utf_display(m->descriptor);
				printf(": native stub\n");
#endif
				/* this is an native stub stackframe info, so we can get the */
				/* parent pv from the return address (ICMD_INVOKE*) */

				pv = (u1 *) (ptrint) codegen_findmethod(ra);

				/* get methodinfo pointer from parent data segment */

				m = *((methodinfo **) (pv + MethodPointer));

			} else {
				/* Inline stackframe infos are special: they have a xpc of    */
				/* the actual exception position and the return address saved */
				/* since an inline stackframe info can also be in a leaf      */
				/* method (no return address saved on stack!!!).              */
				/* ATTENTION: This one is also for hardware exceptions!!!     */

				/* get methodinfo, sp and ra from the current stackframe info */

				m   = info->method;     /* m == NULL                          */
				pv  = info->pv;         /* pv of parent Java function         */
				sp  = info->sp;         /* sp of parent Java function         */
				ra  = info->ra;         /* ra of parent Java function         */
				xpc = info->xpc;        /* actual exception position          */

#if PRINTMETHODS
				printf("NULL: inline stub\n");
#endif

				/* get methodinfo from current Java method */

				m = *((methodinfo **) (pv + MethodPointer));

				/* if m == NULL, this is a asm_calljavafunction call */

				if (m != NULL) {

					/* add it to the stacktrace */

					stacktrace_fillInStackTrace_method(&buffer, m, pv,
													   (u1 *) ((ptrint) xpc));

					/* get the current stack frame size */

					framesize = *((u4 *) (pv + FrameSize));

					/* set stack pointer to stackframe of parent Java */
					/* function of the current Java function */

#if defined(__I386__) || defined (__X86_64__)
					sp += framesize + SIZEOF_VOID_P;
#else
					sp += framesize;
#endif

					/* get data segment and methodinfo pointer from parent */
					/* method */

					pv = (u1 *) (ptrint) codegen_findmethod(ra);
					m = *((methodinfo **) (pv + MethodPointer));
				}
			}

			/* get next stackframeinfo in the chain */

			info = info->oldThreadspecificHeadValue;

		} else {
#if PRINTMETHODS
			utf_display_classname(m->class->name);
			printf(".");
			utf_display(m->name);
			utf_display(m->descriptor);
			printf(": JIT\n");
#endif

			/* JIT method found, add it to the stacktrace (we subtract 1 from */
			/* the return address since it points the the instruction after */
			/* call) */

			stacktrace_fillInStackTrace_method(&buffer, m, pv,
											   (u1 *) ((ptrint) ra) - 1);

			/* get the current stack frame size */

			framesize = *((u4 *) (pv + FrameSize));

			/* get return address of current stack frame */

			ra = md_stacktrace_get_returnaddress(sp, framesize);

			/* get data segment and methodinfo pointer from parent method */

			pv = (u1 *) (ptrint) codegen_findmethod(ra);
			m = *((methodinfo **) (pv + MethodPointer));

			/* walk the stack */

#if defined(__I386__) || defined (__X86_64__)
			sp += framesize + SIZEOF_VOID_P;
#else
			sp += framesize;
#endif
		}
	}
			
	if (coll)
		coll(target, &buffer);

	if (buffer.needsFree)
		free(buffer.start);
}


static
void stackTraceCollector(void **target, stackTraceBuffer *buffer) {
	stackTraceBuffer *dest=*target=heap_allocate(sizeof(stackTraceBuffer)+buffer->full*sizeof(stacktraceelement),true,0);
	memcpy(*target,buffer,sizeof(stackTraceBuffer));
	memcpy(dest+1,buffer->start,buffer->full*sizeof(stacktraceelement));

	dest->needsFree=0;
	dest->size=dest->full;
	dest->start=(stacktraceelement*)(dest+1);

	/*
	if (buffer->full>0) {
		printf("SOURCE BUFFER:%s\n",buffer->start[0].method->name->text);
		printf("DEST BUFFER:%s\n",dest->start[0].method->name->text);
	} else printf("Buffer is empty\n");
	*/
}


void cacao_stacktrace_NormalTrace(void **target)
{
	cacao_stacktrace_fillInStackTrace(target, &stackTraceCollector);
}



static void classContextCollector(void **target, stackTraceBuffer *buffer)
{
	java_objectarray  *tmpArray;
	stacktraceelement *current;
	stacktraceelement *start;
	size_t size;
	size_t targetSize;
	s4 i;

	size = buffer->full;
	targetSize = 0;

	for (i = 0; i < size; i++)
		if (buffer->start[i].method != 0)
			targetSize++;

	start = buffer->start;
	start++;
	targetSize--;

	if (targetSize > 0) {
		if (start->method && (start->method->class == class_java_lang_SecurityManager)) {
			targetSize--;
			start++;
		}
	}

	tmpArray = builtin_anewarray(targetSize, class_java_lang_Class);

	for(i = 0, current = start; i < targetSize; i++, current++) {
		/* XXX TWISTI: should we use this skipping for native stubs? */

		if (!current->method) {
			i--;
			continue;
		}

		use_class_as_object(current->method->class);

		tmpArray->data[i] = (java_objectheader *) current->method->class;
	}

	*target = tmpArray;
}



java_objectarray *cacao_createClassContextArray(void)
{
	java_objectarray *array=0;

	cacao_stacktrace_fillInStackTrace((void **) &array, &classContextCollector);

	return array;
}


/* stacktrace_classLoaderCollector *********************************************

   XXX

*******************************************************************************/

static void stacktrace_classLoaderCollector(void **target,
											stackTraceBuffer *buffer)
{
	stacktraceelement *current;
	stacktraceelement *start;
	methodinfo        *m;
	ptrint             size;
	s4                 i;

	size = buffer->full;
	start = &(buffer->start[0]);

	for(i = 0, current = start; i < size; i++, current++) {
		m = current->method;

		if (!m)
			continue;

		if (m->class == class_java_security_PrivilegedAction) {
			*target = NULL;
			return;
		}

		if (m->class->classloader) {
			*target = (java_lang_ClassLoader *) m->class->classloader;
			return;
		}
	}

	*target = NULL;
}


/* cacao_currentClassLoader ****************************************************

   XXX

*******************************************************************************/

java_objectheader *cacao_currentClassLoader(void)
{
	java_objectheader *header=0;

	cacao_stacktrace_fillInStackTrace((void**)&header,
									  &stacktrace_classLoaderCollector);

	return header;
}


static
void callingMethodCollector(void **target, stackTraceBuffer *buffer) {	
        if (buffer->full >2) (*target)=buffer->start[2].method;
	else (*target=0);
}

methodinfo *cacao_callingMethod() {
	methodinfo *method;
	cacao_stacktrace_fillInStackTrace((void**)&method,&callingMethodCollector);
	return method;
}


static
void getStackCollector(void **target, stackTraceBuffer *buffer)
{
	java_objectarray *classes;
	java_objectarray *methodnames;
	java_objectarray **result=(java_objectarray**)target;
	java_lang_String *str;
	classinfo *c;
	stacktraceelement *current;
	int i,size;

	/*log_text("getStackCollector");*/

	size = buffer->full;

	*result = builtin_anewarray(2, arrayclass_java_lang_Object);

	if (!(*result))
		return;

	classes = builtin_anewarray(size, class_java_lang_Class);

	if (!classes)
		return;

	methodnames = builtin_anewarray(size, class_java_lang_String);

	if (!methodnames)
		return;

	(*result)->data[0] = (java_objectheader *) classes;
	(*result)->data[1] = (java_objectheader *) methodnames;

	/*log_text("Before for loop");*/
	for (i = 0, current = &(buffer->start[0]); i < size; i++, current++) {
		/*log_text("In loop");*/
		c = current->method->class;
		use_class_as_object(c);
		classes->data[i] = (java_objectheader *) c;
		str = javastring_new(current->method->name);
		if (!str)
			return;
		methodnames->data[i] = (java_objectheader *) str;
		/*printf("getStackCollector: %s.%s\n",c->name->text,current->method->name->text);*/
	}

	/*if (*exceptionptr) panic("Exception in getStackCollector");*/

	/*log_text("loop left");*/
        return;

}


java_objectarray *cacao_getStackForVMAccessController(void)
{
	java_objectarray *result = NULL;

	cacao_stacktrace_fillInStackTrace((void **) &result, &getStackCollector);

	return result;
}


/* stacktrace_dump_trace *******************************************************

   This method is call from signal_handler_sigusr1 to dump the
   stacktrace of the current thread to stdout.

*******************************************************************************/

void stacktrace_dump_trace(void)
{
	stackTraceBuffer      *buffer;
	stacktraceelement     *element;
	methodinfo            *m;
	s4                     i;

#if 0
	/* get thread stackframeinfo */

	info = &THREADINFO->_stackframeinfo;

	/* fill stackframeinfo structure */

	tmp.oldThreadspecificHeadValue = *info;
	tmp.addressOfThreadspecificHead = info;
	tmp.method = NULL;
	tmp.sp = NULL;
	tmp.ra = _mc->gregs[REG_RIP];

	*info = &tmp;
#endif

	/* generate stacktrace */

	cacao_stacktrace_NormalTrace((void **) &buffer);

	/* print stacktrace */

	if (buffer) {
		element = buffer->start;

		for (i = 0; i < buffer->size; i++, element++) {
			m = element->method;

			printf("\tat ");
			utf_display_classname(m->class->name);
			printf(".");
			utf_display(m->name);
			utf_display(m->descriptor);

			if (m->flags & ACC_NATIVE) {
				printf("(Native Method)\n");

			} else {
				printf("(");
				utf_display(m->class->sourcefile);
				printf(":%d)\n", (u4) element->linenumber);
			}
		}
	}

	/* flush stdout */

	fflush(stdout);
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
