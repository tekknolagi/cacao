/* vm/jit/stack.h - stack analysis header

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

   Changes: Christian Ullrich

   $Id: stack.h 5367 2006-09-06 11:12:55Z edwin $

*/


#ifndef _STACK_H
#define _STACK_H

#include "config.h"

#include "vm/types.h"

#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/jit/jit.h"
#include "vm/jit/reg.h"


/* macros used internally by analyse_stack ************************************/

/* convenient abbreviations */
#define CURKIND    curstack->varkind
#define CURTYPE    curstack->type


/*--------------------------------------------------*/
/* STACK DEPTH CHECKING                             */
/*--------------------------------------------------*/

#if defined(ENABLE_VERIFIER)
#define CHECK_STACK_DEPTH(depthA,depthB) \
	do { \
		if ((depthA) != (depthB)) \
			goto throw_stack_depth_error; \
	} while (0)
#else /* !ENABLE_VERIFIER */
#define CHECK_STACK_DEPTH(depthA,depthB)
#endif /* ENABLE_VERIFIER */


/*--------------------------------------------------*/
/* BASIC TYPE CHECKING                              */
/*--------------------------------------------------*/

/* XXX would be nice if we did not have to pass the expected type */

#if defined(ENABLE_VERIFIER)
#define CHECK_BASIC_TYPE(expected,actual) \
	do { \
		if ((actual) != (expected)) { \
			expectedtype = (expected); \
			goto throw_stack_type_error; \
		} \
	} while (0)
#else /* !ENABLE_VERIFIER */
#define CHECK_BASIC_TYPE(expected,actual)
#endif /* ENABLE_VERIFIER */

/*--------------------------------------------------*/
/* STACK UNDERFLOW/OVERFLOW CHECKS                  */
/*--------------------------------------------------*/

/* underflow checks */

#if defined(ENABLE_VERIFIER)
#define REQUIRE(num) \
    do { \
        if (stackdepth < (num)) \
			goto throw_stack_underflow; \
	} while (0)
#else /* !ENABLE_VERIFIER */
#define REQUIRE(num)
#endif /* ENABLE_VERIFIER */

#define REQUIRE_1     REQUIRE(1)
#define REQUIRE_2     REQUIRE(2)
#define REQUIRE_3     REQUIRE(3)
#define REQUIRE_4     REQUIRE(4)


/* overflow check */
/* We allow ACONST instructions inserted as arguments to builtin
 * functions to exceed the maximum stack depth.  Maybe we should check
 * against maximum stack depth only at block boundaries?
 */

/* XXX we should find a way to remove the opc/op1 check */
#if defined(ENABLE_VERIFIER)
#define CHECKOVERFLOW \
	do { \
		if (stackdepth > m->maxstack) \
			if ((iptr->opc != ICMD_ACONST) || !(iptr->flags.bits & INS_FLAG_NOCHECK)) \
				goto throw_stack_overflow; \
	} while(0)
#else /* !ENABLE_VERIFIER */
#define CHECKOVERFLOW
#endif /* ENABLE_VERIFIER */

/*--------------------------------------------------*/
/* ALLOCATING STACK SLOTS                           */
/*--------------------------------------------------*/

#define NEWSTACK(s,v,n) \
    do { \
        new->prev = curstack; \
        new->type = (s); \
        new->flags = 0; \
        new->varkind = (v); \
        new->varnum = (n); \
        curstack = new; \
        new++; \
    } while (0)

/* Initialize regoff, so -sia can show regnames even before reg.inc */
/* regs[rd->intregargnum] has to be set for this                    */
/* new->regoff = (IS_FLT_DBL_TYPE(s))?-1:rd->intreg_argnum; }       */

#define NEWSTACKn(s,n)  NEWSTACK(s,UNDEFVAR,n)
#define NEWSTACK0(s)    NEWSTACK(s,UNDEFVAR,0)

/* allocate the input stack for an exception handler */
#define NEWXSTACK   {NEWSTACK(TYPE_ADR,STACKVAR,0);curstack=0;}


/*--------------------------------------------------*/
/* STACK MANIPULATION                               */
/*--------------------------------------------------*/

/* resetting to an empty operand stack */

#define STACKRESET \
    do { \
        curstack = 0; \
        stackdepth = 0; \
    } while (0)


/* set the output stack of the current instruction */

#define SETDST    iptr->dst = curstack;


/* The following macros do NOT check stackdepth, set stackdepth or iptr->dst */

#define POP(s) \
    do { \
		CHECK_BASIC_TYPE((s),curstack->type); \
        if (curstack->varkind == UNDEFVAR) \
            curstack->varkind = TEMPVAR; \
        curstack = curstack->prev; \
    } while (0)

#define POPANY \
    do { \
        if (curstack->varkind == UNDEFVAR) \
            curstack->varkind = TEMPVAR; \
        curstack = curstack->prev; \
    } while (0)

/* Do not copy Interface Stackslots over DUPx, Swaps! */
#define COPY(s,d) \
    do { \
        (d)->flags = 0; \
        (d)->type = (s)->type; \
		if ( (s)->varkind != STACKVAR) {		\
			(d)->varkind = (s)->varkind; \
			(d)->varnum = (s)->varnum;	 \
		} else { \
			(d)->varkind = TEMPVAR; \
			(d)->varnum = 0; \
		} \
    } while (0)


/*--------------------------------------------------*/
/* MACROS FOR HANDLING BASIC BLOCKS                 */
/*--------------------------------------------------*/

/* COPYCURSTACK makes a copy of the current operand stack (curstack)
 * and returns it in the variable copy.
 *
 * This macro is used to propagate the operand stack from one basic
 * block to another. The destination block receives the copy as its
 * input stack.
 */
#define COPYCURSTACK(copy) {\
	int d;\
	stackptr s;\
	if(curstack){\
		s=curstack;\
		new+=stackdepth;\
		d=stackdepth;\
		copy=new;\
		while(s){\
			copy--;d--;\
			copy->prev=copy-1;\
			copy->type=s->type;\
			copy->flags=0;\
			copy->varkind=STACKVAR;\
			copy->varnum=d;\
			s=s->prev;\
			}\
		copy->prev=NULL;\
		copy=new-1;\
		}\
	else\
		copy=NULL;\
}

/* MARKREACHED marks the destination block <b> as reached. If this
 * block has been reached before we check if stack depth and types
 * match. Otherwise the destination block receives a copy of the
 * current stack as its input stack.
 *
 * b...destination block
 * c...current stack
 */

/* XXX this macro is much too big! */

#define MARKREACHED(b,c) \
    do { \
		if ((b) <= (bptr)) \
			(b)->bitflags |= BBFLAG_REPLACEMENT; \
	    if ((b)->flags < BBREACHED) { \
			int locali; \
		    COPYCURSTACK((c)); \
            (b)->flags = BBREACHED; \
            (b)->instack = (c); \
            (b)->indepth = stackdepth; \
			(b)->invars = DMNEW(stackptr, stackdepth); \
			for (locali = stackdepth; locali--; (c) = (c)->prev) \
				(b)->invars[locali] = (c); \
        } else { \
            stackptr s = curstack; \
            stackptr t = (b)->instack; \
			CHECK_STACK_DEPTH((b)->indepth, stackdepth); \
		    while (s) { \
				CHECK_BASIC_TYPE(s->type,t->type); \
			    s = s->prev; \
                t = t->prev; \
			} \
		} \
    } while (0)


/* external macros ************************************************************/

#define BLOCK_OF(index)                                              \
    (jd->new_basicblocks + jd->new_basicblockindex[index])


/* function prototypes ********************************************************/

bool stack_init(void);

bool new_stack_analyse(jitdata *jd);

#endif /* _STACK_H */


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
 * vim:noexpandtab:sw=4:ts=4:
 */
