/* src/vm/jit/s390/codegen.c - machine code generator for s390

   Copyright (C) 1996-2005, 2006, 2007, 2008
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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "native/jni.h"
#include "native/localref.h"
#include "native/native.h"

#include "mm/memory.h"

#include "threads/lock-common.h"

#include "vmcore/loader.h"
#include "vmcore/options.h"
#include "vmcore/statistics.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/global.h"
#include "vm/jit/abi.h"
#if defined(ENABLE_LSRA)
# include "vm/jit/allocator/lsra.h"
#endif
#include "vm/jit/asmpart.h"
#include "vm/jit/codegen-common.h"
#include "vm/jit/dseg.h"
#include "vm/jit/emit-common.h"
#include "vm/jit/jit.h"
#include "vm/jit/linenumbertable.h"
#include "vm/jit/methodheader.h"
#include "vm/jit/parse.h"
#include "vm/jit/patcher-common.h"
#include "vm/jit/reg.h"
#include "vm/jit/replace.h"
#include "vm/jit/s390/arch.h"
#include "vm/jit/s390/codegen.h"
#include "vm/jit/s390/emit.h"
#include "vm/jit/s390/md-abi.h"
#include "vm/jit/stacktrace.h"
#include "vm/types.h"
#include "vm/stringlocal.h"
#include "vm/vm.h"

/* DO__LOG generates a call to do__log. No registers are destroyed,
 * so you may use it anywhere. regs is an array containing all general
 * purpose registers.
 */

/*
static void do__log(u4 *regs) {
}
*/

#define DO__LOG \
	N_AHI(REG_SP, -200); \
	N_STM(R0, R15, 96, REG_SP); \
	M_ALD_DSEG(R14, dseg_add_address(cd, &do__log)); \
	N_LA(R2, 96, RN, REG_SP); \
	N_BASR(R14, R14); \
	N_LM(R0, R15, 96, REG_SP); \
	N_AHI(REG_SP, 200);

/* If the following macro is defined, workaround code for hercules quirks
 * is generated
 */

/* #define SUPPORT_HERCULES 1 */

/* codegen *********************************************************************

   Generates machine code.

*******************************************************************************/

/*

Layout of stackframe:

Meaning                                Offset
===============================================================================
return_address                         (stackframesize - 1) * 8 
saved_int_reg[INT_SAV_CNT - 1]         (stackframseize - 2) * 8
...
saved_int_reg[rd->savintreguse]  
saved_flt_reg[FLT_SAV_CNT - 1]
...
saved_flt_reg[rd->savfltreguse]        (stackframesize - 1 - savedregs_num) * 8

return_value_tmp                       (rd->memuse + 1) * 8
monitorenter_argument                  (rd->memuse) * 8 
???
local[rd->memuse - 1]                  (rd->memuse - 1) * 8
....
local[2]                               2 * 8
local[1]                               1 * 8
local[0]                               0 * 8

*/

bool codegen_emit(jitdata *jd)
{
	methodinfo         *m;
	codeinfo           *code;
	codegendata        *cd;
	registerdata       *rd;
	s4                  len, s1, s2, s3, d, dd, disp;
	u2                  currentline;
	varinfo            *var;
	basicblock         *bptr;
	instruction        *iptr;
	constant_classref  *cr;
	unresolved_class   *uc;
	methodinfo         *lm;             /* local methodinfo for ICMD_INVOKE*  */
	unresolved_method  *um;
	builtintable_entry *bte;
	methoddesc         *md;
	fieldinfo          *fi;
	unresolved_field   *uf;
	s4                  fieldtype;
#if 0
	rplpoint           *replacementpoint;
#endif
	s4                 varindex;

	/* get required compiler data */

	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;
	rd   = jd->rd;

	/* prevent compiler warnings */

	d   = 0;
	lm  = NULL;
	um  = NULL;
	bte = NULL;

	{
	s4 i, p, t, l;
	s4 savedregs_num;

  	savedregs_num = 0; 

	/* space to save used callee saved registers */

	savedregs_num += (INT_SAV_CNT - rd->savintreguse);
	savedregs_num += (FLT_SAV_CNT - rd->savfltreguse);

	cd->stackframesize = rd->memuse + savedregs_num + 1  /* space to save RA */;

	/* CAUTION:
	 * As REG_ITMP2 == REG_RA, do not touch REG_ITMP2, until it has been saved.
	 */

#if defined(ENABLE_THREADS)
	/* Space to save argument of monitor_enter and Return Values to
	   survive monitor_exit. The stack position for the argument can
	   not be shared with place to save the return register
	   since both values reside in R2. */

	if (checksync && code_is_synchronized(code)) {
		/* 1 slot space to save argument of monitor_enter */
		/* 1 slot to temporary store return value before monitor_exit */
		cd->stackframesize += 2;
	}
#endif

	/* Keep stack of non-leaf functions 16-byte aligned for calls into
	   native code e.g. libc or jni (alignment problems with
	   movaps). */

	if (!code_is_leafmethod(code) || opt_verbosecall )
		/* TODO really 16 bytes ? */
		cd->stackframesize = (cd->stackframesize + 2) & ~2;

	/* create method header */

	(void) dseg_add_unique_address(cd, code);              /* CodeinfoPointer */
	(void) dseg_add_unique_s4(cd, cd->stackframesize * 8); /* FrameSize       */

	code->synchronizedoffset = rd->memuse * 8;

	/* REMOVEME: We still need it for exception handling in assembler. */

	if (code_is_leafmethod(code))
		(void) dseg_add_unique_s4(cd, 1);
	else
		(void) dseg_add_unique_s4(cd, 0);

	(void) dseg_add_unique_s4(cd, INT_SAV_CNT - rd->savintreguse); /* IntSave */
	(void) dseg_add_unique_s4(cd, FLT_SAV_CNT - rd->savfltreguse); /* FltSave */

	/* Offset PV */

	M_AADD_IMM(N_PV_OFFSET, REG_PV);

	/* create stack frame (if necessary) */

	if (cd->stackframesize) {
		M_ASUB_IMM(cd->stackframesize * 8, REG_SP);
	}

	/* store return address */

	M_AST(REG_RA, REG_SP, (cd->stackframesize - 1) * 8);

	/* generate method profiling code */

#if defined(ENABLE_PROFILING)
	if (JITDATA_HAS_FLAG_INSTRUMENT(jd)) {
		/* count frequency */
		M_ALD_DSEG(REG_ITMP1, CodeinfoPointer);
		ICONST(REG_ITMP2, 1);
		N_AL(REG_ITMP2, OFFSET(codeinfo, frequency), RN, REG_ITMP1);
		M_IST(REG_ITMP2, REG_ITMP1, OFFSET(codeinfo, frequency));

		PROFILE_CYCLE_START;
	}
#endif

	/* save used callee saved registers and return address */

  	p = cd->stackframesize - 1;

	for (i = INT_SAV_CNT - 1; i >= rd->savintreguse; i--) {
 		p--; M_IST(rd->savintregs[i], REG_SP, p * 8);
	}
	for (i = FLT_SAV_CNT - 1; i >= rd->savfltreguse; i--) {
		p--; M_DST(rd->savfltregs[i], REG_SP, p * 8);
	}

	/* take arguments out of register or stack frame */

	md = m->parseddesc;

 	for (p = 0, l = 0; p < md->paramcount; p++) {
 		t = md->paramtypes[p].type;
		varindex = jd->local_map[l * 5 + t];

 		l++;

		if (IS_2_WORD_TYPE(t))
			l++;

		if (varindex == UNUSED)
			continue;

		var = VAR(varindex);

		s1 = md->params[p].regoff;

		if (IS_INT_LNG_TYPE(t)) {                    /* integer args          */
			if (IS_2_WORD_TYPE(t)) {
				s2 = PACK_REGS(
					GET_LOW_REG(s1),
					GET_HIGH_REG(s1)
				);
			} else {
				s2 = s1;
			}
 			if (!md->params[p].inmemory) {           /* register arguments    */
 				if (!IS_INMEMORY(var->flags)) {      /* reg arg -> register   */
					if (IS_2_WORD_TYPE(t)) {
						M_LNGMOVE(s2, var->vv.regoff);
					} else {
						M_INTMOVE(s2, var->vv.regoff);
					}
				} else {                             /* reg arg -> spilled    */
					if (IS_2_WORD_TYPE(t)) {
						M_LST(s2, REG_SP, var->vv.regoff);
					} else {
						M_IST(s2, REG_SP, var->vv.regoff);
					}
				}

			} else {                                 /* stack arguments       */
 				if (!IS_INMEMORY(var->flags)) {      /* stack arg -> register */
					if (IS_2_WORD_TYPE(t)) {
						M_LLD(var->vv.regoff, REG_SP, cd->stackframesize * 8 + s1);
					} else {
						M_ILD(var->vv.regoff, REG_SP, cd->stackframesize * 8 + s1);
					}
				} else {                             /* stack arg -> spilled  */
					N_MVC(var->vv.regoff, 8, REG_SP, cd->stackframesize * 8 + s1, REG_SP);
				}
			}

		} else {                                     /* floating args         */
 			if (!md->params[p].inmemory) {           /* register arguments    */
				s2 = s1;
 				if (!IS_INMEMORY(var->flags)) {      /* reg arg -> register   */
 					M_FLTMOVE(s2, var->vv.regoff);

 				} else {			                 /* reg arg -> spilled    */
					if (IS_2_WORD_TYPE(t))
						M_DST(s2, REG_SP, var->vv.regoff);
					else
						M_FST(s2, REG_SP, var->vv.regoff);
 				}

 			} else {                                 /* stack arguments       */
 				if (!IS_INMEMORY(var->flags)) {      /* stack-arg -> register */
					if (IS_2_WORD_TYPE(t))
						M_DLD(var->vv.regoff, REG_SP, cd->stackframesize * 8 + s1);

					else
						M_FLD(var->vv.regoff, REG_SP, cd->stackframesize * 8 + s1);

 				} else {                             /* stack-arg -> spilled  */
					N_MVC(var->vv.regoff, 8, REG_SP, cd->stackframesize * 8 + s1, REG_SP);
					var->vv.regoff = cd->stackframesize * 8 + s1;
				}
			}
		}
	} /* end for */

	/* save monitorenter argument */

#if defined(ENABLE_THREADS)
	if (checksync && code_is_synchronized(code)) {
		/* stack offset for monitor argument */

		s1 = rd->memuse;

#if !defined(NDEBUG)
		if (opt_verbosecall) {
			M_ASUB_IMM((INT_ARG_CNT + FLT_ARG_CNT) * 8, REG_SP);

			for (p = 0; p < INT_ARG_CNT; p++)
				M_IST(abi_registers_integer_argument[p], REG_SP, p * 8);

			for (p = 0; p < FLT_ARG_CNT; p++)
				M_DST(abi_registers_float_argument[p], REG_SP, (INT_ARG_CNT + p) * 8);

			s1 += ((INT_ARG_CNT + FLT_ARG_CNT));
		}
#endif

		/* decide which monitor enter function to call */

		if (m->flags & ACC_STATIC) {
			disp = dseg_add_address(cd, &m->class->object.header);
			M_ALD_DSEG(REG_A0, disp);
		}
		else {
			M_TEST(REG_A0);
			M_BNE(SZ_BRC + SZ_ILL);
			M_ILL(EXCEPTION_HARDWARE_NULLPOINTER);
		}

		disp = dseg_add_functionptr(cd, LOCK_monitor_enter);
		M_ALD_DSEG(REG_ITMP2, disp);

		M_AST(REG_A0, REG_SP, s1 * 8);

		M_ASUB_IMM(96, REG_SP);	
		M_CALL(REG_ITMP2);
		M_AADD_IMM(96, REG_SP);	

#if !defined(NDEBUG)
		if (opt_verbosecall) {
			for (p = 0; p < INT_ARG_CNT; p++)
				M_ILD(abi_registers_integer_argument[p], REG_SP, p * 8);

			for (p = 0; p < FLT_ARG_CNT; p++)
				M_DLD(abi_registers_float_argument[p], REG_SP, (INT_ARG_CNT + p) * 8);

			M_AADD_IMM((INT_ARG_CNT + FLT_ARG_CNT) * 8, REG_SP);
		}
#endif
	}
#endif

#if !defined(NDEBUG)
	if (JITDATA_HAS_FLAG_VERBOSECALL(jd))
		emit_verbosecall_enter(jd);
#endif /* !defined(NDEBUG) */

	}

	/* end of header generation */

	/* create replacement points */

	REPLACEMENT_POINTS_INIT(cd, jd);

	/* walk through all basic blocks */

	for (bptr = jd->basicblocks; bptr != NULL; bptr = bptr->next) {

		bptr->mpc = (u4) ((u1 *) cd->mcodeptr - cd->mcodebase);

		if (bptr->flags >= BBREACHED) {

		/* branch resolving */

		codegen_resolve_branchrefs(cd, bptr);

		/* handle replacement points */

		REPLACEMENT_POINT_BLOCK_START(cd, bptr);

		/* copy interface registers to their destination */

		len = bptr->indepth;
		MCODECHECK(512);

#if defined(ENABLE_PROFILING)
		/* generate basicblock profiling code */

		if (JITDATA_HAS_FLAG_INSTRUMENT(jd)) {
			/* count frequency */

			M_ALD_DSEG(REG_ITMP1, CodeinfoPointer);
			M_ALD(REG_ITMP1, REG_ITMP1, OFFSET(codeinfo, bbfrequency));
			ICONST(REG_ITMP2, 1);
			N_AL(REG_ITMP2, bptr->nr * 4, RN, REG_ITMP1);
			M_IST(REG_ITMP2, REG_ITMP1, bptr->nr * 4);

			/* if this is an exception handler, start profiling again */

			if (bptr->type == BBTYPE_EXH)
				PROFILE_CYCLE_START;
		}
#endif

#if defined(ENABLE_LSRA)
		if (opt_lsra) {
			while (len) {
				len--;
				src = bptr->invars[len];
				if ((len == bptr->indepth-1) && (bptr->type != BBTYPE_STD)) {
					if (bptr->type == BBTYPE_EXH) {
/*  					d = reg_of_var(rd, src, REG_ITMP1); */
						if (!IS_INMEMORY(src->flags))
							d = src->vv.regoff;
						else
							d = REG_ITMP3_XPTR;
						M_INTMOVE(REG_ITMP3_XPTR, d);
						emit_store(jd, NULL, src, d);
					}
				}
			}
			
		} else {
#endif

		while (len) {
			len--;
			var = VAR(bptr->invars[len]);
  			if ((len ==  bptr->indepth-1) && (bptr->type != BBTYPE_STD)) {
				if (bptr->type == BBTYPE_EXH) {
					d = codegen_reg_of_var(0, var, REG_ITMP3_XPTR);
					M_INTMOVE(REG_ITMP3_XPTR, d);
					emit_store(jd, NULL, var, d);
				}
			} 
			else {
				assert((var->flags & INOUT));
			}
		}
#if defined(ENABLE_LSRA)
		}
#endif
		/* walk through all instructions */
		
		len = bptr->icount;
		currentline = 0;

		for (iptr = bptr->iinstr; len > 0; len--, iptr++) {
			if (iptr->line != currentline) {
				linenumbertable_list_entry_add(cd, iptr->line);
				currentline = iptr->line;
			}

			MCODECHECK(1024);                         /* 1KB should be enough */

		switch (iptr->opc) {
		case ICMD_NOP:        /* ...  ==> ...                                 */
		case ICMD_POP:        /* ..., value  ==> ...                          */
		case ICMD_POP2:       /* ..., value, value  ==> ...                   */
			break;

		case ICMD_INLINE_START:

			REPLACEMENT_POINT_INLINE_START(cd, iptr);
			break;

		case ICMD_INLINE_BODY:

			REPLACEMENT_POINT_INLINE_BODY(cd, iptr);
			linenumbertable_list_entry_add_inline_start(cd, iptr);
			linenumbertable_list_entry_add(cd, iptr->line);
			break;

		case ICMD_INLINE_END:

			linenumbertable_list_entry_add_inline_end(cd, iptr);
			linenumbertable_list_entry_add(cd, iptr->line);
			break;

		case ICMD_CHECKNULL:  /* ..., objectref  ==> ..., objectref           */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			emit_nullpointer_check(cd, iptr, s1);
			break;

		/* constant operations ************************************************/

		case ICMD_ICONST:     /* ...  ==> ..., constant                       */
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			ICONST(d, iptr->sx.val.i);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LCONST:     /* ...  ==> ..., constant                       */

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP12_PACKED);
			LCONST(d, iptr->sx.val.l);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FCONST:     /* ...  ==> ..., constant                       */
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			disp = dseg_add_float(cd, iptr->sx.val.f);
			M_FLD_DSEG(d, disp, REG_ITMP1);
			emit_store_dst(jd, iptr, d);
			break;
		
		case ICMD_DCONST:     /* ...  ==> ..., constant                       */
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			disp = dseg_add_double(cd, iptr->sx.val.d);
			M_DLD_DSEG(d, disp, REG_ITMP1);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ACONST:     /* ...  ==> ..., constant                       */
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				cr = iptr->sx.val.c.ref;
				disp = dseg_add_unique_address(cd, cr);

/* 				PROFILE_CYCLE_STOP; */

				patcher_add_patch_ref(jd, PATCHER_resolve_classref_to_classinfo,
									  cr, disp);

/* 				PROFILE_CYCLE_START; */

				M_ALD_DSEG(d, disp);
			} else {
				if (iptr->sx.val.anyptr == 0) {
					M_CLR(d);
				} else {
					disp = dseg_add_unique_address(cd, iptr->sx.val.anyptr);
					M_ALD_DSEG(d, disp);
					/*
					if (((u4)(iptr->sx.val.anyptr) & 0x00008000) == 0) {
						N_LHI(d, ((u4)(iptr->sx.val.anyptr) >> 16) & 0xFFFF);
						M_SLL_IMM(16, d);
						N_AHI(d, (u4)(iptr->sx.val.anyptr) & 0xFFFF);
					} else {
						disp = dseg_add_unique_address(cd, iptr->sx.val.anyptr);
						M_ALD_DSEG(d, disp);
					}
					*/
				}
			}
			emit_store_dst(jd, iptr, d);
			break;


		/* load/store/copy/move operations ************************************/

		case ICMD_ILOAD:      /* ...  ==> ..., content of local variable      */
		case ICMD_ALOAD:      /* s1 = local variable                          */
		case ICMD_LLOAD:
		case ICMD_FLOAD:  
		case ICMD_DLOAD:  
		case ICMD_ISTORE:     /* ..., value  ==> ...                          */
		case ICMD_LSTORE:
		case ICMD_FSTORE:
		case ICMD_DSTORE: 
		case ICMD_COPY:
		case ICMD_MOVE:

			emit_copy(jd, iptr);
			break;

		case ICMD_ASTORE:
			if (!(iptr->flags.bits & INS_FLAG_RETADDR))
				emit_copy(jd, iptr);
			break;

		/* integer operations *************************************************/

		case ICMD_INEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1); 
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_INEG(s1, d);
			emit_store_dst(jd, iptr, d);

			break;

		case ICMD_LNEG:       /* ..., value  ==> ..., - value                 */

			s1 = emit_load_s1(jd, iptr, REG_ITMP12_PACKED);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP12_PACKED);
			M_INEG(GET_HIGH_REG(s1), GET_HIGH_REG(d));
			M_INEG(GET_LOW_REG(s1), GET_LOW_REG(d));
			N_BRC(8 /* result zero, no overflow */, SZ_BRC + SZ_AHI); 
			N_AHI(GET_HIGH_REG(d), -1);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_I2L:        /* ..., value  ==> ..., value                   */

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP31_PACKED); /* even-odd */
			if (! N_IS_EVEN_ODD(d)) {
				d = REG_ITMP31_PACKED;
			}
			assert(N_IS_EVEN_ODD(d));

			s1 = emit_load_s1(jd, iptr, REG_ITMP2);

			M_INTMOVE(s1, GET_HIGH_REG(d));
			M_SRDA_IMM(32, GET_HIGH_REG(d));

			emit_copy_dst(jd, iptr, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_L2I:        /* ..., value  ==> ..., value                   */
			s1 = emit_load_s1_low(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			M_INTMOVE(GET_LOW_REG(s1), d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_INT2BYTE:   /* ..., value  ==> ..., value                   */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_INTMOVE(s1, d);
			M_SLL_IMM(24, d);
			M_SRA_IMM(24, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_INT2CHAR:   /* ..., value  ==> ..., value                   */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_INTMOVE(s1, d);
			M_SLL_IMM(16, d);
			M_SRL_IMM(16, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_INT2SHORT:  /* ..., value  ==> ..., value                   */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			M_INTMOVE(s1, d);
			M_SLL_IMM(16, d);
			M_SRA_IMM(16, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if (s2 == d)
				M_IADD(s1, d);
			else {
				M_INTMOVE(s1, d);
				M_IADD(s2, d);
			}
			emit_store_dst(jd, iptr, d);

			break;

		case ICMD_IINC:
		case ICMD_IADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* sx.val.i = constant                             */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			M_INTMOVE(s1, d);

			if (N_VALID_IMM(iptr->sx.val.i)) {
				M_IADD_IMM(iptr->sx.val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->sx.val.i);
				M_IADD(REG_ITMP2, d);	
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LADD:       /* ..., val1, val2  ==> ..., val1 + val2        */

			/* M, (r, q) -> (r, q) */

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP12_PACKED);

			s1 = emit_load_s1_high(jd, iptr, GET_HIGH_REG(REG_ITMP12_PACKED));
			s2 = emit_load_s2_high(jd, iptr, REG_ITMP3);
			dd = GET_HIGH_REG(d);

			if (s2 == dd) {
				M_IADD(s1, dd);
			} else {
				M_INTMOVE(s1, dd);
				M_IADD(s2, dd);
			}

			s1 = emit_load_s1_low(jd, iptr, GET_LOW_REG(REG_ITMP12_PACKED));
			s2 = emit_load_s2_low(jd, iptr, REG_ITMP3);
			dd = GET_LOW_REG(d);

			if (s2 == dd) {
				N_ALR(dd, s1);
			} else {
				M_INTMOVE(s1, dd);
				N_ALR(dd, s2);
			}

			N_BRC(8 | 4, SZ_BRC + SZ_AHI); 
			N_AHI(GET_HIGH_REG(d), 1);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LADDCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* sx.val.l = constant                             */

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP12_PACKED);
			dd = GET_HIGH_REG(d);

			s1 = emit_load_s1_high(jd, iptr, dd);
			s3 = iptr->sx.val.l >> 32;

			M_INTMOVE(s1, dd);

			if (N_VALID_IMM(s3)) {
				M_IADD_IMM(s3, dd);
			} else {
				ICONST(REG_ITMP3, s3);
				M_IADD(REG_ITMP3, dd);
			}

			dd = GET_LOW_REG(d);
			s1 = emit_load_s1_low(jd, iptr, dd);
			s3 = iptr->sx.val.l & 0xffffffff;
			ICONST(REG_ITMP3, s3);

			M_INTMOVE(s1, dd);
			N_ALR(dd, REG_ITMP3);

			N_BRC(8 | 4, SZ_BRC + SZ_AHI); 
			N_AHI(GET_HIGH_REG(d), 1);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_ISUB:       /* ..., val1, val2  ==> ..., val1 - val2        */
								 
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if (s2 == d) {
				M_INTMOVE(s1, REG_ITMP1);
				M_ISUB(s2, REG_ITMP1);
				M_INTMOVE(REG_ITMP1, d);
			} else {
				M_INTMOVE(s1, d);
				M_ISUB(s2, d);
			}
			emit_store_dst(jd, iptr, d);

			break;

		case ICMD_ISUBCONST:  /* ..., value  ==> ..., value + constant        */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			M_INTMOVE(s1, d);

			if (N_VALID_IMM(-iptr->sx.val.i)) {
				M_ISUB_IMM(iptr->sx.val.i, d);
			} else {
				ICONST(REG_ITMP2, iptr->sx.val.i);
				M_ISUB(REG_ITMP2, d);
			}
			emit_store_dst(jd, iptr, d);

			break;

		case ICMD_LSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP12_PACKED);

			s1 = emit_load_s1_high(jd, iptr, GET_HIGH_REG(REG_ITMP12_PACKED));
			s2 = emit_load_s2_high(jd, iptr, REG_ITMP3);
			dd = GET_HIGH_REG(d);

			if (s2 == dd) {
				M_INTMOVE(s2, REG_ITMP3);
				s2 = REG_ITMP3;
			}

			M_INTMOVE(s1, dd);
			M_ISUB(s2, dd);

			s1 = emit_load_s1_low(jd, iptr, GET_LOW_REG(REG_ITMP12_PACKED));
			s2 = emit_load_s2_low(jd, iptr, REG_ITMP3);
			dd = GET_LOW_REG(d);

			if (s2 == dd) {
				M_INTMOVE(s2, REG_ITMP3);
				s2 = REG_ITMP3;
			} 

			M_INTMOVE(s1, dd);
			N_SLR(dd, s2);

			N_BRC(1 | 2, SZ_BRC + SZ_AHI); 
			N_AHI(GET_HIGH_REG(d), -1);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LSUBCONST:  /* ..., value  ==> ..., value - constant        */
		                      /* sx.val.l = constant                             */

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP12_PACKED);

			dd = GET_HIGH_REG(d);
			s1 = emit_load_s1_high(jd, iptr, dd);
			s3 = iptr->sx.val.l >> 32;

			M_INTMOVE(s1, dd);

			if (N_VALID_IMM(-s3)) {
				M_IADD_IMM(-s3, dd);
			} else {
				ICONST(REG_ITMP3, s3);
				M_ISUB(REG_ITMP3, dd);
			}

			dd = GET_LOW_REG(d);
			s1 = emit_load_s1_low(jd, iptr, dd);
			s3 = iptr->sx.val.l & 0xffffffff;
			ICONST(REG_ITMP3, s3);

			M_INTMOVE(s1, dd);
			N_SLR(dd, REG_ITMP3);

			N_BRC(1 | 2, SZ_BRC + SZ_AHI); 
			N_AHI(GET_HIGH_REG(d), -1);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if (s2 == d)
				M_IMUL(s1, d);
			else {
				M_INTMOVE(s1, d);
				M_IMUL(s2, d);
			}
			emit_store_dst(jd, iptr, d);

			break;

		case ICMD_IMULCONST:  /* ..., value  ==> ..., value * constant        */
		                      /* sx.val.i = constant                             */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			M_INTMOVE(s1, d);
			if (iptr->sx.val.i == 2) {
				M_SLL_IMM(1, d);
			} else if (N_VALID_IMM(iptr->sx.val.i)) {
				M_IMUL_IMM(iptr->sx.val.i, d);
			} else {
				disp = dseg_add_s4(cd, iptr->sx.val.i);
				M_ILD_DSEG(REG_ITMP2, disp);
				M_IMUL(REG_ITMP2, d);	
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */
		case ICMD_IREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			/* load s1 into r0 */

			s1 = emit_load_s1(jd, iptr, GET_HIGH_REG(REG_ITMP31_PACKED));
			M_INTMOVE(s1, GET_HIGH_REG(REG_ITMP31_PACKED));
			s1 = GET_HIGH_REG(REG_ITMP31_PACKED);

			s2 = emit_load_s2(jd, iptr, REG_ITMP2);

			/* extend s1 to long */

			M_SRDA_IMM(32, GET_HIGH_REG(REG_ITMP31_PACKED));

			/* divide */

			N_DR(GET_HIGH_REG(REG_ITMP31_PACKED), s2);

			/* take result */

			switch (iptr->opc) {
				case ICMD_IREM:
					d = codegen_reg_of_dst(jd, iptr, GET_HIGH_REG(REG_ITMP31_PACKED));
					M_INTMOVE(GET_HIGH_REG(REG_ITMP31_PACKED), d);
					break;
				case ICMD_IDIV:
					d = codegen_reg_of_dst(jd, iptr, GET_LOW_REG(REG_ITMP31_PACKED));
					M_INTMOVE(GET_LOW_REG(REG_ITMP31_PACKED), d);
					break;
			}

			emit_store_dst(jd, iptr, d);

			break;

		case ICMD_LDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */
		case ICMD_LREM:       /* ..., val1, val2  ==> ..., val1 % val2        */

			bte = iptr->sx.s23.s3.bte;
			md  = bte->md;

			/* test s2 for zero */

			s2 = emit_load_s2(jd, iptr, REG_ITMP12_PACKED);
			M_INTMOVE(GET_LOW_REG(s2), REG_ITMP3);
			M_IOR(GET_HIGH_REG(s2), REG_ITMP3);
			emit_arithmetic_check(cd, iptr, REG_ITMP3);

			/* TODO SIGFPE? */

			disp = dseg_add_functionptr(cd, bte->fp);

			/* load arguments */

			M_LNGMOVE(s2, PACK_REGS(REG_A3, REG_A2));

			s1 = emit_load_s1(jd, iptr, PACK_REGS(REG_A1, REG_A0));
			M_LNGMOVE(s1, PACK_REGS(REG_A1, REG_A0));

			/* call builtin */

			M_ASUB_IMM(96, REG_SP);
			M_ALD_DSEG(REG_ITMP2, disp);
			M_JSR(REG_RA, REG_ITMP2);
			M_AADD_IMM(96, REG_SP);

			/* store result */

			d = codegen_reg_of_dst(jd, iptr, REG_RESULT_PACKED);
			M_LNGMOVE(REG_RESULT_PACKED, d);
			emit_store_dst(jd, iptr, d);

			break;

		case ICMD_ISHL:       /* ..., val1, val2  ==> ..., val1 << val2       */
		case ICMD_ISHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */
		case ICMD_IUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);

			/* Use only 5 bits of sencond operand. */

			M_INTMOVE(s2, REG_ITMP2);
			s2 = REG_ITMP2;
			ICONST(REG_ITMP3, 0x1F);
			M_IAND(REG_ITMP3, s2);

			M_INTMOVE(s1, d);

			switch (iptr->opc) {
				case ICMD_ISHL:
					M_SLL(s2, d);
					break;
				case ICMD_ISHR:
					M_SRA(s2, d);
					break;
				case ICMD_IUSHR:
					M_SRL(s2, d);
					break;
				default:
					assert(0);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IDIVPOW2:   /* ..., value  ==> ..., value >> constant       */
		                      /* sx.val.i = constant                             */
			{
				u1 *ref;

				assert(iptr->sx.val.i <= 32);

				s1 = emit_load_s1(jd, iptr, REG_ITMP1);
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);

				M_INTMOVE(s1, d);
				M_TEST(d);
				ref = cd->mcodeptr;
				M_BGE(0);

				s3 = (1 << iptr->sx.val.i) - 1;

				if (N_VALID_IMM(s3)) {
					M_IADD_IMM(s3, d);
				} else  {
					ICONST(REG_ITMP1, -1);
					M_SRL_IMM(32 - iptr->sx.val.i, REG_ITMP1);
					M_IADD(REG_ITMP1, d);
				}

				N_BRC_BACK_PATCH(ref);

				M_SRA_IMM(iptr->sx.val.i, d);

				emit_store_dst(jd, iptr, d);
			}

			break;
	
		case ICMD_IREMPOW2:   /* ..., value  ==> ..., value % constant        */
		                      /* sx.val.i = constant                             */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}
				
			ICONST(REG_ITMP3, iptr->sx.val.i);

			M_INTMOVE(s1, d);
			M_IAND(REG_ITMP3, d);

			M_TEST(s1);
			M_BGE(SZ_BRC + SZ_LCR + SZ_NR + SZ_LCR);

			N_LCR(d, s1);
			N_NR(d, REG_ITMP3);
			N_LCR(d, d);

			emit_store_dst(jd, iptr, d);

			break;

		case ICMD_ISHLCONST:  /* ..., value  ==> ..., value << constant       */
		                      /* sx.val.i = constant                             */
		case ICMD_ISHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* sx.val.i = constant                             */
		case ICMD_IUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
		                      /* sx.val.i = constant                             */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);

			M_INTMOVE(s1, d);

			disp = iptr->sx.val.i & 0x1F; /* Use only 5 bits of value */

			switch (iptr->opc) {
				case ICMD_ISHLCONST:
					N_SLL(d, disp, RN);
					break;
				case ICMD_ISHRCONST:
					N_SRA(d, disp, RN);
					break;
				case ICMD_IUSHRCONST:
					N_SRL(d, disp, RN);
					break;
				default:
					assert(0);
			}

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LSHL:       /* ..., val1, val2  ==> ..., val1 << val2       */

		case ICMD_LSHR:       /* ..., val1, val2  ==> ..., val1 >> val2       */

		case ICMD_LUSHR:      /* ..., val1, val2  ==> ..., val1 >>> val2      */

			s2 = emit_load_s2(jd, iptr, REG_ITMP2);

			/* Use only 6 bits of second operand */

			M_INTMOVE(s2, REG_ITMP2);
			s2 = REG_ITMP2;
			ICONST(REG_ITMP1, 0x3F);
			M_IAND(REG_ITMP1, s2);

			s1 = emit_load_s1(jd, iptr, REG_ITMP31_PACKED); /* even-odd pair */
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP31_PACKED);

			/* Destination must be even-odd pair */

			if (! N_IS_EVEN_ODD(d)) {
				d = REG_ITMP31_PACKED;
			}

			assert(N_IS_EVEN_ODD(d));

			M_LNGMOVE(s1, d);

			switch (iptr->opc) {
				case ICMD_LSHL:
					M_SLDL(s2, GET_HIGH_REG(d));
					break;
				case ICMD_LSHR:
					M_SRDA(s2, GET_HIGH_REG(d));
					break;
				case ICMD_LUSHR:
					M_SRDL(s2, GET_HIGH_REG(d));
					break;
				default:
					assert(0);
			}

			emit_copy_dst(jd, iptr, d);
			emit_store_dst(jd, iptr, d);

			break;

        case ICMD_LSHLCONST:  /* ..., value  ==> ..., value << constant       */
 			                  /* sx.val.i = constant                             */
		case ICMD_LSHRCONST:  /* ..., value  ==> ..., value >> constant       */
		                      /* sx.val.i = constant                             */
  		case ICMD_LUSHRCONST: /* ..., value  ==> ..., value >>> constant      */
  		                      /* sx.val.l = constant                             */
		case ICMD_LMULPOW2:
		
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP31_PACKED); /* even-odd */
			if (! N_IS_EVEN_ODD(d)) {
				d = REG_ITMP31_PACKED;
			}
			assert(N_IS_EVEN_ODD(d));

			s1 = emit_load_s1(jd, iptr, d);
		
			M_LNGMOVE(s1, d);

			disp = iptr->sx.val.i & 0x3F; /* Use only 6 bits of operand */

			switch (iptr->opc) {
				case ICMD_LSHLCONST:
					N_SLDL(GET_HIGH_REG(d), disp, RN);
					break;
				case ICMD_LSHRCONST:
					N_SRDA(GET_HIGH_REG(d), disp, RN);
					break;
				case ICMD_LUSHRCONST:
					N_SRDL(GET_HIGH_REG(d), disp, RN);
					break;
				case ICMD_LMULPOW2:
					N_SLDL(GET_HIGH_REG(d), disp, RN);
					break;
				default:
					assert(0);
			}

			emit_copy_dst(jd, iptr, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if (s2 == d)
				M_IAND(s1, d);
			else {
				M_INTMOVE(s1, d);
				M_IAND(s2, d);
			}
			emit_store_dst(jd, iptr, d);

			break;

		case ICMD_IOR:        /* ..., val1, val2  ==> ..., val1 | val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if (s2 == d)
				M_IOR(s1, d);
			else {
				M_INTMOVE(s1, d);
				M_IOR(s2, d);
			}
			emit_store_dst(jd, iptr, d);

			break;

		case ICMD_IXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if (s2 == d)
				M_IXOR(s1, d);
			else {
				M_INTMOVE(s1, d);
				M_IXOR(s2, d);
			}
			emit_store_dst(jd, iptr, d);

			break;



		case ICMD_IORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* sx.val.i = constant                             */
		case ICMD_IANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* sx.val.i = constant                             */
		case ICMD_IXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* sx.val.i = constant                             */

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP1);
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);

			M_INTMOVE(s1, d);
			ICONST(REG_ITMP2, iptr->sx.val.i);

			switch (iptr->opc) {
				case ICMD_IANDCONST:
					M_IAND(REG_ITMP2, d);
					break;
				case ICMD_IXORCONST:
					M_IXOR(REG_ITMP2, d);
					break;
				case ICMD_IORCONST:
					M_IOR(REG_ITMP2, d);
					break;
				default:
					assert(0);
			}

			emit_store_dst(jd, iptr, d);

			break;

		case ICMD_LOR:        /* ..., val1, val2  ==> ..., val1 | val2        */
		case ICMD_LXOR:       /* ..., val1, val2  ==> ..., val1 ^ val2        */
		case ICMD_LAND:       /* ..., val1, val2  ==> ..., val1 & val2        */

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP12_PACKED);

			s1 = emit_load_s1_low(jd, iptr, GET_LOW_REG(REG_ITMP12_PACKED));
			s2 = emit_load_s2_low(jd, iptr, REG_ITMP3);
			dd = GET_LOW_REG(d);

			switch (iptr->opc) {
				case ICMD_LAND:
					if (s2 == dd) {
						M_IAND(s1, dd);
					} else {
						M_INTMOVE(s1, dd);
						M_IAND(s2, dd);
					}
					break;
				case ICMD_LXOR:
					if (s2 == dd) {
						M_IXOR(s1, dd);
					} else {
						M_INTMOVE(s1, dd);
						M_IXOR(s2, dd);
					}
					break;
				case ICMD_LOR:
					if (s2 == dd) {
						M_IOR(s1, dd);
					} else {
						M_INTMOVE(s1, dd);
						M_IOR(s2, dd);
					}
					break;
				default:
					assert(0);
			}

			s1 = emit_load_s1_high(jd, iptr, GET_HIGH_REG(REG_ITMP12_PACKED));
			s2 = emit_load_s2_high(jd, iptr, REG_ITMP3);
			dd = GET_HIGH_REG(d);

			switch (iptr->opc) {
				case ICMD_LAND:
					if (s2 == dd) {
						M_IAND(s1, dd);
					} else {
						M_INTMOVE(s1, dd);
						M_IAND(s2, dd);
					}
					break;
				case ICMD_LXOR:
					if (s2 == dd) {
						M_IXOR(s1, dd);
					} else {
						M_INTMOVE(s1, dd);
						M_IXOR(s2, dd);
					}
					break;
				case ICMD_LOR:
					if (s2 == dd) {
						M_IOR(s1, dd);
					} else {
						M_INTMOVE(s1, dd);
						M_IOR(s2, dd);
					}
					break;
				default:
					assert(0);
			}

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LORCONST:   /* ..., value  ==> ..., value | constant        */
		                      /* sx.val.l = constant                             */
		case ICMD_LXORCONST:  /* ..., value  ==> ..., value ^ constant        */
		                      /* sx.val.l = constant                             */
		case ICMD_LANDCONST:  /* ..., value  ==> ..., value & constant        */
		                      /* sx.val.l = constant                             */

			/* TODO should use memory operand to access data segment, not load */

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP12_PACKED);

			s1 = emit_load_s1_low(jd, iptr, GET_LOW_REG(d));
			s3 = iptr->sx.val.l & 0xffffffff;

			M_INTMOVE(s1, GET_LOW_REG(d));

			ICONST(REG_ITMP3, s3);

			switch (iptr->opc) {
				case ICMD_LANDCONST:
					M_IAND(REG_ITMP3, GET_LOW_REG(d));
					break;
				case ICMD_LXORCONST:
					M_IXOR(REG_ITMP3, GET_LOW_REG(d));
					break;
				case ICMD_LORCONST:
					M_IOR(REG_ITMP3, GET_LOW_REG(d));
					break;
				default:
					assert(0);
			}

			s1 = emit_load_s1_high(jd, iptr, GET_HIGH_REG(d));
			s3 = iptr->sx.val.l >> 32;

			M_INTMOVE(s1, GET_HIGH_REG(d));

			ICONST(REG_ITMP3, s3);

			switch (iptr->opc) {
				case ICMD_LANDCONST:
					M_IAND(REG_ITMP3, GET_HIGH_REG(d));
					break;
				case ICMD_LXORCONST:
					M_IXOR(REG_ITMP3, GET_HIGH_REG(d));
					break;
				case ICMD_LORCONST:
					M_IOR(REG_ITMP3, GET_HIGH_REG(d));
					break;
				default:
					assert(0);
			}

			emit_store_dst(jd, iptr, d);

			break;

		/* floating operations ************************************************/

		case ICMD_FNEG:       /* ..., value  ==> ..., - value                 */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			M_FMOVN(s1, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DNEG:       /* ..., value  ==> ..., - value                 */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			M_DMOVN(s1, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FADD:       /* ..., val1, val2  ==> ..., val1 + val2        */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			if (s2 == d)
				M_FADD(s1, d);
			else {
				M_FLTMOVE(s1, d);
				M_FADD(s2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DADD:       /* ..., val1, val2  ==> ..., val1 + val2        */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			if (s2 == d)
				M_DADD(s1, d);
			else {
				M_FLTMOVE(s1, d);
				M_DADD(s2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2_but(jd, iptr, REG_FTMP2, d);

			M_FLTMOVE(s1, d);
			M_FSUB(s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DSUB:       /* ..., val1, val2  ==> ..., val1 - val2        */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2_but(jd, iptr, REG_FTMP2, d);

			M_FLTMOVE(s1, d);
			M_DSUB(s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			if (s2 == d)
				M_FMUL(s1, d);
			else {
				M_FLTMOVE(s1, d);
				M_FMUL(s2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DMUL:       /* ..., val1, val2  ==> ..., val1 * val2        */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			if (s2 == d)
				M_DMUL(s1, d);
			else {
				M_FLTMOVE(s1, d);
				M_DMUL(s2, d);
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2_but(jd, iptr, REG_FTMP2, d);

			M_FLTMOVE(s1, d);
			M_FDIV(s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DDIV:       /* ..., val1, val2  ==> ..., val1 / val2        */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2_but(jd, iptr, REG_FTMP2, d);

			M_FLTMOVE(s1, d);
			M_DDIV(s2, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_I2F:       /* ..., value  ==> ..., (float) value            */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			M_CVTIF(s1, d);
  			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_I2D:       /* ..., value  ==> ..., (double) value           */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
			M_CVTID(s1, d);
  			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_F2I:       /* ..., value  ==> ..., (int) value              */
		case ICMD_D2I:
			{
				u1 *ref1;
#ifdef SUPPORT_HERCULES
				u1 *ref2, *ref3;
#endif

				s1 = emit_load_s1(jd, iptr, REG_FTMP1);
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);

				/* Test if NAN */

				switch (iptr->opc) {
					case ICMD_F2I:
						N_LTEBR(s1, s1); 
						break;
					case ICMD_D2I:
						N_LTDBR(s1, s1);
						break;
				}

				N_BRC(DD_0 | DD_1 | DD_2, SZ_BRC + SZ_LHI + SZ_BRC); /* Non a NaN */
				N_LHI(d, 0); /* Load 0 */
				ref1 = cd->mcodeptr;
				N_BRC(DD_ANY, 0); /* Exit */

				/* Convert */

				switch (iptr->opc) {
					case ICMD_F2I:
						M_CVTFI(s1, d); 
						break;
					case ICMD_D2I:
						M_CVTDI(s1, d); 
						break;
				}

#ifdef SUPPORT_HERCULES
				/* Hercules does the conversion using a plain C conversion.
				 * According to manual, real hardware should *NOT* require this.
				 *
				 * Corner case: Positive float leads to INT_MIN (overflow).
				 */

				switch (iptr->opc) {
					case ICMD_F2I:
						N_LTEBR(s1, s1); 
						break;
					case ICMD_D2I:
						N_LTDBR(s1, s1);
						break;
				}

				ref2 = cd->mcodeptr;
				N_BRC(DD_0 | DD_1 | DD_3, 0); /* If operand is positive, continue */

				M_TEST(d);

				ref3 = cd->mcodeptr;
				M_BGE(0); /* If integer result is negative, continue */

				disp = dseg_add_s4(cd, 0x7fffffff); /* Load INT_MAX */
				M_ILD_DSEG(d, disp);
#endif
				N_BRC_BACK_PATCH(ref1);
#ifdef SUPPORT_HERCULES
				N_BRC_BACK_PATCH(ref2);
				N_BRC_BACK_PATCH(ref3);
#endif
				emit_store_dst(jd, iptr, d);
			}
			break;

		case ICMD_F2D:       /* ..., value  ==> ..., (double) value           */
			{
#ifdef SUPPORT_HERCULES
				u1 *ref;
#endif
				s1 = emit_load_s1(jd, iptr, REG_FTMP1);
				d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
#ifdef SUPPORT_HERCULES
				N_LTEBR(s1, s1);
				ref = cd->mcodeptr;
				N_BRC(DD_0 | DD_1 | DD_2, 0); /* Non a NaN */
				disp = dseg_add_double(cd, 0.0 / 0.0);
				M_DLD_DSEG(d, disp, REG_ITMP1);
				emit_label_br(cd, BRANCH_LABEL_1);
				N_BRC_BACK_PATCH(ref);
#endif
				M_CVTFD(s1, d);
#ifdef SUPPORT_HERCULES
				emit_label(cd, BRANCH_LABEL_1);
#endif
				emit_store_dst(jd, iptr, d);
			}
			break;

		case ICMD_D2F:       /* ..., value  ==> ..., (float) value            */
			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP2);
			M_CVTDF(s1, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_FCMPL:      /* ..., val1, val2  ==> ..., val1 fcmpl val2    */
 			                  /* == => 0, < => 1, > => -1 */
		case ICMD_DCMPL:


		case ICMD_FCMPG:      /* ..., val1, val2  ==> ..., val1 fcmpg val2    */
 			                  /* == => 0, < => 1, > => -1 */
		case ICMD_DCMPG:

			s1 = emit_load_s1(jd, iptr, REG_FTMP1);
			s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);

			switch (iptr->opc) {
				case ICMD_FCMPG:
				case ICMD_FCMPL:
					M_FCMP(s1, s2);
					break;
				case ICMD_DCMPG:
				case ICMD_DCMPL:
					M_DCMP(s1, s2);
					break;	
			}

			N_BRC( /* load 1 */
				DD_H | (iptr->opc == ICMD_FCMPG || iptr->opc == ICMD_DCMPG ? DD_O : 0),
				SZ_BRC + SZ_BRC + SZ_BRC
			);

			N_BRC( /* load -1 */
				DD_L | (iptr->opc == ICMD_FCMPL || iptr->opc == ICMD_DCMPL ? DD_O : 0),
				SZ_BRC + SZ_BRC + SZ_LHI + SZ_BRC
			);

			N_BRC( /* load 0 */
				DD_E,
				SZ_BRC + SZ_LHI + SZ_BRC + SZ_LHI + SZ_BRC
			);

			N_LHI(d, 1); /* GT */
			M_BR(SZ_BRC + SZ_LHI + SZ_BRC + SZ_LHI);
			N_LHI(d, -1); /* LT */
			M_BR(SZ_BRC + SZ_LHI);
			N_LHI(d, 0); /* EQ */

			emit_store_dst(jd, iptr, d);

			break;


		/* memory operations **************************************************/

		case ICMD_ARRAYLENGTH: /* ..., arrayref  ==> ..., (int) length        */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP3);
			/* TODO softnull */
			/* implicit null-pointer check */
			M_ILD(d, s1, OFFSET(java_array_t, size));
			emit_store_dst(jd, iptr, d);

			break;

		case ICMD_BALOAD:     /* ..., arrayref, index  ==> ..., value         */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);

			N_IC(d, OFFSET(java_bytearray_t, data[0]), s2, s1);

			M_SLL_IMM(24, d);
			M_SRA_IMM(24, d);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_CALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);

			M_INTMOVE(s2, REG_ITMP2);
			M_SLL_IMM(1, REG_ITMP2);

			N_LH(d, OFFSET(java_chararray_t, data[0]), REG_ITMP2, s1);

			/* N_LH does sign extends, undo ! */

			M_SLL_IMM(16, d);
			M_SRL_IMM(16, d);

			emit_store_dst(jd, iptr, d);
			break;			

		case ICMD_SALOAD:     /* ..., arrayref, index  ==> ..., value         */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);

			M_INTMOVE(s2, REG_ITMP2);
			M_SLL_IMM(1, REG_ITMP2);

			N_LH(d, OFFSET(java_shortarray_t, data[0]), REG_ITMP2, s1);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_IALOAD:     /* ..., arrayref, index  ==> ..., value         */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);
			
			M_INTMOVE(s2, REG_ITMP2);
			M_SLL_IMM(2, REG_ITMP2); /* scale index by 4 */
			N_L(d, OFFSET(java_intarray_t, data[0]), REG_ITMP2, s1);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_LALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);

			d = codegen_reg_of_dst(jd, iptr, REG_ITMP13_PACKED);

			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);
			
			M_INTMOVE(s2, REG_ITMP2);
			M_SLL_IMM(3, REG_ITMP2); /* scale index by 8 */

			N_L(
				GET_LOW_REG(d) /* maybe itmp3 */, 
				OFFSET(java_intarray_t, data[0]) + 4, 
				REG_ITMP2, s1 /* maybe itmp1 */
			);

			N_L(
				GET_HIGH_REG(d) /* maybe itmp1 */, 
				OFFSET(java_intarray_t, data[0]), 
				REG_ITMP2, s1 /* maybe itmp1 */
			);

			emit_store_dst(jd, iptr, d);

			break;

		case ICMD_FALOAD:     /* ..., arrayref, index  ==> ..., value         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);

			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);

			M_INTMOVE(s2, REG_ITMP2);
			M_SLL_IMM(2, REG_ITMP2); /* scale index by 4 */
	
			N_LE(d, OFFSET(java_floatarray_t, data[0]), REG_ITMP2, s1);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_DALOAD:     /* ..., arrayref, index  ==> ..., value         */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);

			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);

			M_INTMOVE(s2, REG_ITMP2);
			M_SLL_IMM(3, REG_ITMP2); /* scale index by 8 */
	
			N_LD(d, OFFSET(java_floatarray_t, data[0]), REG_ITMP2, s1);

			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_AALOAD:     /* ..., arrayref, index  ==> ..., value         */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);

			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);
			
			M_INTMOVE(s2, REG_ITMP2);
			M_SLL_IMM(2, REG_ITMP2); /* scale index by 4 */
			N_L(d, OFFSET(java_objectarray_t, data[0]), REG_ITMP2, s1);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_BASTORE:    /* ..., arrayref, index, value  ==> ...         */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);
			s3 = emit_load_s3(jd, iptr, REG_ITMP3);

			N_STC(s3, OFFSET(java_bytearray_t, data[0]), s2, s1);
			break;

		case ICMD_CASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);
			s3 = emit_load_s3(jd, iptr, REG_ITMP3);

			M_INTMOVE(s2, REG_ITMP2);
			M_SLL_IMM(1, REG_ITMP2);

			N_STH(s3, OFFSET(java_chararray_t, data[0]), REG_ITMP2, s1);

			break;

		case ICMD_SASTORE:    /* ..., arrayref, index, value  ==> ...         */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);
			s3 = emit_load_s3(jd, iptr, REG_ITMP3);

			M_INTMOVE(s2, REG_ITMP2);
			M_SLL_IMM(1, REG_ITMP2);

			N_STH(s3, OFFSET(java_shortarray_t, data[0]), REG_ITMP2, s1);
			break;

		case ICMD_IASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);

			s3 = emit_load_s3(jd, iptr, REG_ITMP3);

			M_INTMOVE(s2, REG_ITMP2);
			M_SLL_IMM(2, REG_ITMP2);

			N_ST(s3, OFFSET(java_intarray_t, data[0]), REG_ITMP2, s1);
			break;

		case ICMD_LASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);

			M_INTMOVE(s2, REG_ITMP2);
			M_SLL_IMM(3, REG_ITMP2);

			s3 = emit_load_s3_high(jd, iptr, REG_ITMP3);
			N_ST(s3, OFFSET(java_intarray_t, data[0]), REG_ITMP2, s1);
			s3 = emit_load_s3_low(jd, iptr, REG_ITMP3);
			N_ST(s3, OFFSET(java_intarray_t, data[0]) + 4, REG_ITMP2, s1);
			break;

		case ICMD_FASTORE:    /* ..., arrayref, index, value  ==> ...         */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);
			s3 = emit_load_s3(jd, iptr, REG_FTMP1);

			M_INTMOVE(s2, REG_ITMP2);
			M_SLL_IMM(2, REG_ITMP2);

			N_STE(s3, OFFSET(java_floatarray_t, data[0]), REG_ITMP2, s1);
			break;

		case ICMD_DASTORE:    /* ..., arrayref, index, value  ==> ...         */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);
			s3 = emit_load_s3(jd, iptr, REG_FTMP1);

			M_INTMOVE(s2, REG_ITMP2);
			M_SLL_IMM(3, REG_ITMP2);

			N_STD(s3, OFFSET(java_doublearray_t, data[0]), REG_ITMP2, s1);
			break;

		case ICMD_AASTORE:    /* ..., arrayref, index, value  ==> ...         */

			s1 = emit_load_s1(jd, iptr, REG_A0);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			/* implicit null-pointer check */
			emit_arrayindexoutofbounds_check(cd, iptr, s1, s2);
			s3 = emit_load_s3(jd, iptr, REG_A1);

			M_INTMOVE(s1, REG_A0);
			M_INTMOVE(s3, REG_A1);

			disp = dseg_add_functionptr(cd, BUILTIN_FAST_canstore);
			M_ALD_DSEG(REG_ITMP2, disp);
			M_ASUB_IMM(96, REG_SP);
			M_JSR(REG_RA, REG_ITMP2);
			M_AADD_IMM(96, REG_SP);

			emit_arraystore_check(cd, iptr);

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			s3 = emit_load_s3(jd, iptr, REG_ITMP3);

			M_INTMOVE(s2, REG_ITMP2);
			M_SLL_IMM(2, REG_ITMP2);
			N_ST(s3, OFFSET(java_objectarray_t, data[0]), REG_ITMP2, s1);

			/*
			M_SAADDQ(s2, s1, REG_ITMP1); itmp1 := 4 * s2 + s1
			M_AST(s3, REG_ITMP1, OFFSET(java_objectarray, data[0]));
			*/
			break;


		case ICMD_GETSTATIC:  /* ...  ==> ..., value                          */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				uf        = iptr->sx.s23.s3.uf;
				fieldtype = uf->fieldref->parseddesc.fd->type;
				disp      = dseg_add_unique_address(cd, NULL);

/* 				PROFILE_CYCLE_STOP; */

				patcher_add_patch_ref(jd, PATCHER_get_putstatic, uf, disp);

/* 				PROFILE_CYCLE_START; */
			}
			else {
				fi        = iptr->sx.s23.s3.fmiref->p.field;
				fieldtype = fi->type;
				disp      = dseg_add_address(cd, fi->value);

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					PROFILE_CYCLE_STOP;

					patcher_add_patch_ref(jd, PATCHER_initialize_class, fi->class, 0);

					PROFILE_CYCLE_START;
				}
  			}

			M_ALD_DSEG(REG_ITMP1, disp);

			switch (fieldtype) {
			case TYPE_INT:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_ILD(d, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP23_PACKED);
				M_LLD(d, REG_ITMP1, 0);
				break;
			case TYPE_ADR:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_ALD(d, REG_ITMP1, 0);
				break;
			case TYPE_FLT:
				d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
				M_FLD(d, REG_ITMP1, 0);
				break;
			case TYPE_DBL:				
				d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
				M_DLD(d, REG_ITMP1, 0);
				break;
			}

			emit_store_dst(jd, iptr, d);

			break;

		case ICMD_PUTSTATIC:  /* ..., value  ==> ...                          */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				uf        = iptr->sx.s23.s3.uf;
				fieldtype = uf->fieldref->parseddesc.fd->type;
				disp      = dseg_add_unique_address(cd, uf);

				patcher_add_patch_ref(jd, PATCHER_get_putstatic, uf, disp);
			}
			else {
				fi        = iptr->sx.s23.s3.fmiref->p.field;
				fieldtype = fi->type;
				disp      = dseg_add_address(cd, fi->value);

				if (!CLASS_IS_OR_ALMOST_INITIALIZED(fi->class)) {
					PROFILE_CYCLE_STOP;
					patcher_add_patch_ref(jd, PATCHER_initialize_class, fi->class, disp);
					PROFILE_CYCLE_START;
				}
  			}

			M_ALD_DSEG(REG_ITMP1, disp);
			switch (fieldtype) {
			case TYPE_INT:
				s1 = emit_load_s1(jd, iptr, REG_ITMP2);
				M_IST(s1, REG_ITMP1, 0);
				break;
			case TYPE_LNG:
				s1 = emit_load_s1(jd, iptr, REG_ITMP23_PACKED);
				M_LST(s1, REG_ITMP1, 0);
				break;
			case TYPE_ADR:
				s1 = emit_load_s1(jd, iptr, REG_ITMP2);
				M_AST(s1, REG_ITMP1, 0);
				break;
			case TYPE_FLT:
				s1 = emit_load_s1(jd, iptr, REG_FTMP2);
				M_FST(s1, REG_ITMP1, 0);
				break;
			case TYPE_DBL:
				s1 = emit_load_s1(jd, iptr, REG_FTMP2);
				M_DST(s1, REG_ITMP1, 0);
				break;
			}
			break;

		case ICMD_GETFIELD:   /* ...  ==> ..., value                          */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			emit_nullpointer_check(cd, iptr, s1);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				uf        = iptr->sx.s23.s3.uf;
				fieldtype = uf->fieldref->parseddesc.fd->type;
				disp      = 0;

				patcher_add_patch_ref(jd, PATCHER_get_putfield, uf, 0);
			}
			else {
				fi        = iptr->sx.s23.s3.fmiref->p.field;
				fieldtype = fi->type;
				disp      = fi->offset;
			}

			switch (fieldtype) {
			case TYPE_INT:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_ILD(d, s1, disp);
				break;
			case TYPE_LNG:
   				d = codegen_reg_of_dst(jd, iptr, REG_ITMP23_PACKED);
				if (GET_HIGH_REG(d) == s1) {
					M_ILD(GET_LOW_REG(d), s1, disp + 4);
					M_ILD(GET_HIGH_REG(d), s1, disp);
				}
				else {
					M_ILD(GET_LOW_REG(d), s1, disp + 4);
					M_ILD(GET_HIGH_REG(d), s1, disp);
				}
				break;
			case TYPE_ADR:
				d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
				M_ALD(d, s1, disp);
				break;
			case TYPE_FLT:
				d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
				M_FLD(d, s1, disp);
				break;
			case TYPE_DBL:				
				d = codegen_reg_of_dst(jd, iptr, REG_FTMP1);
				M_DLD(d, s1, disp);
				break;
			}
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_PUTFIELD:   /* ..., objectref, value  ==> ...               */
			{
			u1 *ref;

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			emit_nullpointer_check(cd, iptr, s1);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				uf        = iptr->sx.s23.s3.uf;
				fieldtype = uf->fieldref->parseddesc.fd->type;
				disp      = 0;
			} 
			else {
				fi        = iptr->sx.s23.s3.fmiref->p.field;
				fieldtype = fi->type;
				disp      = fi->offset;
			}

			/* We can't add a patcher ref behind this load,
			 * because the patcher would destroy REG_ITMP3.
			 *
			 * We pass in the disp parameter, how many bytes
			 * to skip to the to the actual store.
			 *
			 * XXX this relies on patcher_add_patch_ref internals
			 */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				patcher_add_patch_ref(jd, PATCHER_get_putfield, uf, 0);
				ref = cd->mcodeptr;
			}


			if (IS_INT_LNG_TYPE(fieldtype)) {
				if (IS_2_WORD_TYPE(fieldtype))
					s2 = emit_load_s2(jd, iptr, REG_ITMP23_PACKED);
				else
					s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			} else {
				s2 = emit_load_s2(jd, iptr, REG_FTMP2);
			}

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				((patchref_t *)list_first(jd->code->patchers))->disp = (cd->mcodeptr - ref);
			}

			switch (fieldtype) {
				case TYPE_INT:
					M_IST(s2, s1, disp);
					break;
				case TYPE_LNG:
					M_IST(GET_LOW_REG(s2), s1, disp + 4);      /* keep this order */
					M_IST(GET_HIGH_REG(s2), s1, disp);         /* keep this order */
					break;
				case TYPE_ADR:
					M_AST(s2, s1, disp);
					break;
				case TYPE_FLT:
					M_FST(s2, s1, disp);
					break;
				case TYPE_DBL:
					M_DST(s2, s1, disp);
					break;
			}

			}
			break;

		/* branch operations **************************************************/

		case ICMD_ATHROW:       /* ..., objectref ==> ... (, objectref)       */

			/* PROFILE_CYCLE_STOP; */
		
			s1 = emit_load_s1(jd, iptr, REG_ITMP3);
			M_INTMOVE(s1, REG_ITMP3_XPTR);

#ifdef ENABLE_VERIFIER
			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				uc = iptr->sx.s23.s2.uc;

				patcher_add_patch_ref(jd, PATCHER_resolve_class, uc, 0);
			}
#endif /* ENABLE_VERIFIER */

			disp = dseg_add_functionptr(cd, asm_handle_exception);
			M_ALD_DSEG(REG_ITMP1, disp);
			M_JMP(REG_ITMP1_XPC, REG_ITMP1);
			M_NOP;

			break;

		case ICMD_GOTO:         /* ... ==> ...                                */
		case ICMD_RET:          /* ... ==> ...                                */

			emit_br(cd, iptr->dst.block);

			break;

		case ICMD_JSR:          /* ... ==> ...                                */

			emit_br(cd, iptr->sx.s23.s3.jsrtarget.block);

			break;
			
		case ICMD_IFNULL:       /* ..., value ==> ...                         */
		case ICMD_IFNONNULL:    /* ..., value ==> ...                         */
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			M_TEST(s1);
			switch (iptr->opc) {	
				case ICMD_IFNULL:
					emit_beq(cd, iptr->dst.block);
					break;
				case ICMD_IFNONNULL:
					emit_bne(cd, iptr->dst.block);
					break;
			}
			break;

		case ICMD_IFEQ:         /* ..., value ==> ...                         */
		case ICMD_IFLT:         /* ..., value ==> ...                         */
		case ICMD_IFLE:         /* ..., value ==> ...                         */
		case ICMD_IFNE:         /* ..., value ==> ...                         */
		case ICMD_IFGT:         /* ..., value ==> ...                         */
		case ICMD_IFGE:         /* ..., value ==> ...                         */
			
			s1 = emit_load_s1(jd, iptr, REG_ITMP1);

			if (N_VALID_IMM(iptr->sx.val.i))
				M_ICMP_IMM(s1, iptr->sx.val.i);
			else {
				disp = dseg_add_s4(cd, iptr->sx.val.i);
				if (N_VALID_DSEG_DISP(disp)) {
					N_C(s1, N_DSEG_DISP(disp), RN, REG_PV);
				} else {
					ICONST(REG_ITMP2, disp);
					N_C(s1, -N_PV_OFFSET, REG_ITMP2, REG_PV);
				}
			}

			switch (iptr->opc) {
			case ICMD_IFLT:
				emit_blt(cd, iptr->dst.block);
				break;
			case ICMD_IFLE:
				emit_ble(cd, iptr->dst.block);
				break;
			case ICMD_IFNE:
				emit_bne(cd, iptr->dst.block);
				break;
			case ICMD_IFGT:
				emit_bgt(cd, iptr->dst.block);
				break;
			case ICMD_IFGE:
				emit_bge(cd, iptr->dst.block);
				break;
			case ICMD_IFEQ:
				emit_beq(cd, iptr->dst.block);
				break;
			}

			break;

		case ICMD_IF_LLT:       /* ..., value ==> ...                         */
		case ICMD_IF_LLE:       /* op1 = target JavaVM pc, val.l = constant   */
		case ICMD_IF_LGT:
		case ICMD_IF_LGE:
		case ICMD_IF_LEQ:
		case ICMD_IF_LNE:

			/* ATTENTION: compare high words signed and low words unsigned */

#			define LABEL_OUT BRANCH_LABEL_1

			s1 = emit_load_s1_high(jd, iptr, REG_ITMP1);

			if (N_VALID_IMM(iptr->sx.val.l >> 32))
				M_ICMP_IMM(s1, iptr->sx.val.l >> 32);
			else {
				disp = dseg_add_s4(cd, iptr->sx.val.l >> 32);
				if (N_VALID_DSEG_DISP(disp)) {
					N_C(s1, N_DSEG_DISP(disp), RN, REG_PV);
				} else {
					ICONST(REG_ITMP2, disp);
					N_C(s1, -N_PV_OFFSET, REG_ITMP2, REG_PV);
				}
			}

			switch(iptr->opc) {
				case ICMD_IF_LLT:
				case ICMD_IF_LLE:
					emit_blt(cd, iptr->dst.block);
					/* EQ ... fall through */
					emit_label_bgt(cd, LABEL_OUT);
					break;
				case ICMD_IF_LGT:
				case ICMD_IF_LGE:
					emit_bgt(cd, iptr->dst.block);
					/* EQ ... fall through */
					emit_label_blt(cd, LABEL_OUT);
					break;
				case ICMD_IF_LEQ: 
					/* EQ ... fall through */
					emit_label_bne(cd, LABEL_OUT);
					break;
				case ICMD_IF_LNE:
					/* EQ ... fall through */
					emit_bne(cd, iptr->dst.block);
					break;
				default:
					assert(0);
			}

			s1 = emit_load_s1_low(jd, iptr, REG_ITMP1);

			disp = dseg_add_s4(cd, (s4)(iptr->sx.val.l & 0xffffffff));
			if (N_VALID_DSEG_DISP(disp)) {
				N_CL(s1, N_DSEG_DISP(disp), RN, REG_PV);
			} else {
				ICONST(REG_ITMP2, disp);
				N_CL(s1, -N_PV_OFFSET, REG_ITMP2, REG_PV);
			}

			switch(iptr->opc) {
				case ICMD_IF_LLT:
					emit_blt(cd, iptr->dst.block);
					emit_label(cd, LABEL_OUT);
					break;
				case ICMD_IF_LLE:
					emit_ble(cd, iptr->dst.block);
					emit_label(cd, LABEL_OUT);
					break;
				case ICMD_IF_LGT:
					emit_bgt(cd, iptr->dst.block);
					emit_label(cd, LABEL_OUT);
					break;
				case ICMD_IF_LGE:
					emit_bge(cd, iptr->dst.block);
					emit_label(cd, LABEL_OUT);
					break;
				case ICMD_IF_LEQ:
					emit_beq(cd, iptr->dst.block);
					emit_label(cd, LABEL_OUT);
					break;
				case ICMD_IF_LNE:
					emit_bne(cd, iptr->dst.block);
					break;
				default:
					assert(0);
			}

#			undef LABEL_OUT
			break;

		case ICMD_IF_ACMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ACMPNE:    /* op1 = target JavaVM pc                     */

			/* Compare addresses as 31 bit unsigned integers */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			M_LDA(REG_ITMP1, s1, 0);

			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_LDA(REG_ITMP2, s2, 0);

			M_CMP(REG_ITMP1, REG_ITMP2);

			switch (iptr->opc) {
				case ICMD_IF_ACMPEQ:
					emit_beq(cd, iptr->dst.block);
					break;
				case ICMD_IF_ACMPNE:
					emit_bne(cd, iptr->dst.block);
					break;
			}

			break;

		case ICMD_IF_ICMPEQ:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ICMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ICMPLT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ICMPGT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ICMPLE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_ICMPGE:    /* ..., value, value ==> ...                  */

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			s2 = emit_load_s2(jd, iptr, REG_ITMP2);
			M_CMP(s1, s2);
			switch (iptr->opc) {
				case ICMD_IF_ICMPEQ:
					emit_beq(cd, iptr->dst.block);
					break;
				case ICMD_IF_ICMPNE:
					emit_bne(cd, iptr->dst.block);
					break;
				case ICMD_IF_ICMPLT:
					emit_blt(cd, iptr->dst.block);
					break;
				case ICMD_IF_ICMPGT:
					emit_bgt(cd, iptr->dst.block);
					break;
				case ICMD_IF_ICMPLE:
					emit_ble(cd, iptr->dst.block);
					break;
				case ICMD_IF_ICMPGE:
					emit_bge(cd, iptr->dst.block);
					break;
			}

			break;

		case ICMD_IF_LCMPLE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPLT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGT:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPGE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPNE:    /* ..., value, value ==> ...                  */
		case ICMD_IF_LCMPEQ:    /* ..., value, value ==> ...                  */
			{

				u1 *out_ref = NULL;

				/* ATTENTION: compare high words signed and low words unsigned */
	
				s1 = emit_load_s1_high(jd, iptr, REG_ITMP1);
				s2 = emit_load_s2_high(jd, iptr, REG_ITMP2);

				M_ICMP(s1, s2);

				switch(iptr->opc) {
				case ICMD_IF_LCMPLT:
				case ICMD_IF_LCMPLE:
					emit_blt(cd, iptr->dst.block);
					/* EQ ... fall through */
					out_ref = cd->mcodeptr;
					M_BGT(0);
					break;
				case ICMD_IF_LCMPGT:
				case ICMD_IF_LCMPGE:
					emit_bgt(cd, iptr->dst.block);
					/* EQ ... fall through */
					out_ref = cd->mcodeptr;
					M_BLT(0);
					break;
				case ICMD_IF_LCMPEQ: 
					/* EQ ... fall through */
					out_ref = cd->mcodeptr;
					M_BNE(0);
					break;
				case ICMD_IF_LCMPNE:
					/* EQ ... fall through */
					emit_bne(cd, iptr->dst.block);
					break;
				default:
					assert(0);
				}

				s1 = emit_load_s1_low(jd, iptr, REG_ITMP1);
				s2 = emit_load_s2_low(jd, iptr, REG_ITMP2);
	
				M_ICMPU(s1, s2);

				switch(iptr->opc) {
				case ICMD_IF_LCMPLT:
					emit_blt(cd, iptr->dst.block);
					break;
				case ICMD_IF_LCMPLE:
					emit_ble(cd, iptr->dst.block);
					break;
				case ICMD_IF_LCMPGT:
					emit_bgt(cd, iptr->dst.block);
					break;
				case ICMD_IF_LCMPGE:
					emit_bge(cd, iptr->dst.block);
					break;
				case ICMD_IF_LCMPEQ:
					emit_beq(cd, iptr->dst.block);
					break;
				case ICMD_IF_LCMPNE:
					emit_bne(cd, iptr->dst.block);
					break;
				default:
					assert(0);
				}

				if (out_ref != NULL) {
					N_BRC_BACK_PATCH(out_ref);
				}

			}
			break;

		case ICMD_IRETURN:      /* ..., retvalue ==> ...                      */

			REPLACEMENT_POINT_RETURN(cd, iptr);
			s1 = emit_load_s1(jd, iptr, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);
			goto nowperformreturn;

		case ICMD_ARETURN:      /* ..., retvalue ==> ...                      */

			REPLACEMENT_POINT_RETURN(cd, iptr);
			s1 = emit_load_s1(jd, iptr, REG_RESULT);
			M_INTMOVE(s1, REG_RESULT);

#ifdef ENABLE_VERIFIER
			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				unresolved_class *uc = iptr->sx.s23.s2.uc;

				PROFILE_CYCLE_STOP;
				patcher_add_patch_ref(jd, PATCHER_resolve_class, uc, 0);
				PROFILE_CYCLE_START;
			}
#endif /* ENABLE_VERIFIER */
			goto nowperformreturn;

		case ICMD_LRETURN:      /* ..., retvalue ==> ...                      */

			REPLACEMENT_POINT_RETURN(cd, iptr);
			s1 = emit_load_s1(jd, iptr, REG_RESULT_PACKED);
			M_LNGMOVE(s1, REG_RESULT_PACKED);
			goto nowperformreturn;

		case ICMD_FRETURN:      /* ..., retvalue ==> ...                      */
		case ICMD_DRETURN:

			REPLACEMENT_POINT_RETURN(cd, iptr);
			s1 = emit_load_s1(jd, iptr, REG_FRESULT);
			M_FLTMOVE(s1, REG_FRESULT);
			goto nowperformreturn;

		case ICMD_RETURN:      /* ...  ==> ...                                */

			REPLACEMENT_POINT_RETURN(cd, iptr);

nowperformreturn:
			{
			s4 i, p;
			
			p = cd->stackframesize;

			/* call trace function */

#if !defined(NDEBUG)
			if (JITDATA_HAS_FLAG_VERBOSECALL(jd))
				emit_verbosecall_exit(jd);
#endif /* !defined(NDEBUG) */

#if defined(ENABLE_THREADS)
			if (checksync && code_is_synchronized(code)) {
				/* we need to save the proper return value */

				switch (iptr->opc) {
				case ICMD_LRETURN:
					M_IST(REG_RESULT2, REG_SP, ((rd->memuse + 1) * 8) + 4);
					/* fall through */
				case ICMD_IRETURN:
				case ICMD_ARETURN:
					M_IST(REG_RESULT , REG_SP, (rd->memuse + 1) * 8);
					break;
				case ICMD_FRETURN:
					M_FST(REG_FRESULT, REG_SP, (rd->memuse + 1) * 8);
					break;
				case ICMD_DRETURN:
					M_DST(REG_FRESULT, REG_SP, (rd->memuse + 1) * 8);
					break;
				}

				M_ALD(REG_A0, REG_SP, rd->memuse * 8);

				disp = dseg_add_functionptr(cd, LOCK_monitor_exit);
				M_ALD_DSEG(REG_ITMP2, disp);

				M_ASUB_IMM(96, REG_SP);
				M_CALL(REG_ITMP2);
				M_AADD_IMM(96, REG_SP);

				/* and now restore the proper return value */

				switch (iptr->opc) {
				case ICMD_LRETURN:
					M_ILD(REG_RESULT2, REG_SP, ((rd->memuse + 1) * 8) + 4);
					/* fall through */
				case ICMD_IRETURN:
				case ICMD_ARETURN:
					M_ILD(REG_RESULT , REG_SP, (rd->memuse + 1) * 8);
					break;
				case ICMD_FRETURN:
					M_FLD(REG_FRESULT, REG_SP, (rd->memuse + 1) * 8);
					break;
				case ICMD_DRETURN:
					M_DLD(REG_FRESULT, REG_SP, (rd->memuse + 1) * 8);
					break;
				}
			}
#endif

			/* restore return address                                         */

			p--; M_ALD(REG_RA, REG_SP, p * 8);

			/* restore saved registers                                        */

			for (i = INT_SAV_CNT - 1; i >= rd->savintreguse; i--) {
				p--; M_ILD(rd->savintregs[i], REG_SP, p * 8);
			}
			for (i = FLT_SAV_CNT - 1; i >= rd->savfltreguse; i--) {
				p--; M_DLD(rd->savfltregs[i], REG_SP, p * 8);
			}

			/* deallocate stack                                               */

			if (cd->stackframesize)
				M_AADD_IMM(cd->stackframesize * 8, REG_SP);

			/* generate method profiling code */

			PROFILE_CYCLE_STOP;

			M_RET;
			}
			break;

		case ICMD_TABLESWITCH:  /* ..., index ==> ...                         */
			{
				s4 i, l;
				branch_target_t *table;

				table = iptr->dst.table;

				l = iptr->sx.s23.s2.tablelow;
				i = iptr->sx.s23.s3.tablehigh;

				s1 = emit_load_s1(jd, iptr, REG_ITMP1);
				M_INTMOVE(s1, REG_ITMP1);

				if (l == 0) {
					/* do nothing */
				} else if (N_VALID_IMM(-l)) {
					M_ISUB_IMM(l, REG_ITMP1);
				} else {
					ICONST(REG_ITMP2, l);
					M_ISUB(REG_ITMP2, REG_ITMP1);
				}

				/* number of targets */

				i = i - l + 1;

				/* range check */

				ICONST(REG_ITMP2, i);
				M_ICMPU(REG_ITMP1, REG_ITMP2);
				emit_bge(cd, table[0].block);

				/* build jump table top down and use address of lowest entry */

				table += i;

				while (--i >= 0) {
					dseg_add_target(cd, table->block); 
					--table;
				}
			}

			/* length of dataseg after last dseg_add_target is used by load */

			M_SLL_IMM(2, REG_ITMP1); /* scale by 4 */
			M_ASUB_IMM(cd->dseglen, REG_ITMP1);
			N_L(REG_ITMP1, -N_PV_OFFSET, REG_ITMP1, REG_PV);
			M_JMP(RN, REG_ITMP1);

			break;


		case ICMD_LOOKUPSWITCH: /* ..., key ==> ...                           */
			{
				s4 i;
				lookup_target_t *lookup;

				lookup = iptr->dst.lookup;

				i = iptr->sx.s23.s2.lookupcount;
			
				MCODECHECK(8 + ((7 + 6) * i) + 5);
				s1 = emit_load_s1(jd, iptr, REG_ITMP1);

				while (--i >= 0) {
					if (N_VALID_IMM(lookup->value)) {
						M_ICMP_IMM(s1, lookup->value);
					} else {
						ICONST(REG_ITMP2, lookup->value);
						M_ICMP(REG_ITMP2, s1);
					}
					emit_beq(cd, lookup->target.block);
					lookup++;
				}

				emit_br(cd, iptr->sx.s23.s3.lookupdefault.block);
			}
			break;


		case ICMD_BUILTIN:      /* ..., [arg1, [arg2 ...]] ==> ...            */

			bte = iptr->sx.s23.s3.bte;
			md  = bte->md;
			goto gen_method;

		case ICMD_INVOKESTATIC: /* ..., [arg1, [arg2 ...]] ==> ...            */
		case ICMD_INVOKESPECIAL:/* ..., objectref, [arg1, [arg2 ...]] ==> ... */
		case ICMD_INVOKEVIRTUAL:/* op1 = arg count, val.a = method pointer    */
		case ICMD_INVOKEINTERFACE:

			REPLACEMENT_POINT_INVOKE(cd, iptr);

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				lm = NULL;
				um = iptr->sx.s23.s3.um;
				md = um->methodref->parseddesc.md;
			}
			else {
				lm = iptr->sx.s23.s3.fmiref->p.method;
				um = NULL;
				md = lm->parseddesc;
			}

gen_method:
			s3 = md->paramcount;

			MCODECHECK((s3 << 1) + 64);

			/* copy arguments to registers or stack location */

			for (s3 = s3 - 1; s3 >= 0; s3--) {
				var = VAR(iptr->sx.s23.s2.args[s3]);

				/* Already Preallocated? */
				if (var->flags & PREALLOC)
					continue;

				if (IS_INT_LNG_TYPE(var->type)) {
					if (!md->params[s3].inmemory) {
						if (IS_2_WORD_TYPE(var->type)) {
							s1 = PACK_REGS(
								GET_LOW_REG(md->params[s3].regoff),
								GET_HIGH_REG(md->params[s3].regoff)
							);
							d = emit_load(jd, iptr, var, s1);
							M_LNGMOVE(d, s1);
						}
						else {
							s1 = md->params[s3].regoff;
							d = emit_load(jd, iptr, var, s1);
							M_INTMOVE(d, s1);
						}
					}
					else {
						if (IS_2_WORD_TYPE(var->type)) {
							d = emit_load(jd, iptr, var, REG_ITMP12_PACKED);
							M_LST(d, REG_SP, md->params[s3].regoff);
						}
						else {
							d = emit_load(jd, iptr, var, REG_ITMP1);
							M_IST(d, REG_SP, md->params[s3].regoff);
						}
					}
				}
				else {
					if (!md->params[s3].inmemory) {
						s1 = md->params[s3].regoff;
						d = emit_load(jd, iptr, var, s1);
						M_FLTMOVE(d, s1);
					}
					else {
						d = emit_load(jd, iptr, var, REG_FTMP1);
						if (IS_2_WORD_TYPE(var->type))
							M_DST(d, REG_SP, md->params[s3].regoff);
						else
							M_FST(d, REG_SP, md->params[s3].regoff);
					}
				}
			}

			/* generate method profiling code */

			PROFILE_CYCLE_STOP;

			switch (iptr->opc) {
			case ICMD_BUILTIN:
				if (bte->stub == NULL) {
					disp = dseg_add_functionptr(cd, bte->fp);
					M_ASUB_IMM(96, REG_SP); /* register save area as required by C abi */	
				} else {
					disp = dseg_add_functionptr(cd, bte->stub);
				}

				if (N_VALID_DSEG_DISP(disp)) {
					N_L(REG_PV, N_DSEG_DISP(disp), RN, REG_PV);
				} else {
					N_LHI(REG_ITMP1, disp);
					N_L(REG_PV, -N_PV_OFFSET, REG_ITMP1, REG_PV);
				}
				break;

			case ICMD_INVOKESPECIAL:
				/* TODO softnull */
				/* Implicit NULL pointer check */
				M_ILD(REG_ITMP1, REG_A0, 0);

				/* fall through */

			case ICMD_INVOKESTATIC:
				if (lm == NULL) {
					disp = dseg_add_unique_address(cd, um);

					patcher_add_patch_ref(jd, PATCHER_invokestatic_special,
										um, disp);
				}
				else
					disp = dseg_add_address(cd, lm->stubroutine);

				if (N_VALID_DSEG_DISP(disp)) {
					N_L(REG_PV, N_DSEG_DISP(disp), RN, REG_PV);
				} else {
					N_LHI(REG_ITMP1, disp);
					N_L(REG_PV, -N_PV_OFFSET, REG_ITMP1, REG_PV);
				}
				break;

			case ICMD_INVOKEVIRTUAL:
				/* TODO softnull REG_A0 */

				if (lm == NULL) {
					patcher_add_patch_ref(jd, PATCHER_invokevirtual, um, 0);

					s1 = 0;
				}
				else {
					s1 = OFFSET(vftbl_t, table[0]) +
						sizeof(methodptr) * lm->vftblindex;
				}

				/* implicit null-pointer check */

				M_ALD(REG_METHODPTR, REG_A0, OFFSET(java_object_t, vftbl));
				M_ALD(REG_PV, REG_METHODPTR, s1);
				break;

			case ICMD_INVOKEINTERFACE:
				/* TODO softnull REG_A0 */

				/* s1 will be negative here, so use (0xFFF + s1) as displacement
				 * and -0xFFF in index register (itmp1)
				 */

				if (lm == NULL) {
					patcher_add_patch_ref(jd, PATCHER_invokeinterface, um, 0);

					s1 = 0;
					s2 = 0;
				}
				else {
					s1 = OFFSET(vftbl_t, interfacetable[0]) -
						sizeof(methodptr*) * lm->class->index;

					s2 = sizeof(methodptr) * (lm - lm->class->methods);
				}

				/* Implicit null-pointer check */
				M_ALD(REG_METHODPTR, REG_A0, OFFSET(java_object_t, vftbl));
				N_LHI(REG_ITMP2, s1);
				N_L(REG_METHODPTR, 0, REG_ITMP2, REG_METHODPTR);
				M_ALD(REG_PV, REG_METHODPTR, s2);
				break;
			}

			/* generate the actual call */

			M_CALL(REG_PV);
			emit_restore_pv(cd);

			/* post call finalization */

			switch (iptr->opc) {
				case ICMD_BUILTIN:
					if (bte->stub == NULL) {
						M_AADD_IMM(96, REG_SP); /* remove C abi register save area */
					}
					break;
			}

			/* generate method profiling code */

			PROFILE_CYCLE_START;

			/* store size of call code in replacement point */

			REPLACEMENT_POINT_INVOKE_RETURN(cd, iptr);
	
			/* store return value */

			d = md->returntype.type;

			if (d != TYPE_VOID) {
				if (IS_INT_LNG_TYPE(d)) {
					if (IS_2_WORD_TYPE(d)) {
						s1 = codegen_reg_of_dst(jd, iptr, REG_RESULT_PACKED);
						M_LNGMOVE(REG_RESULT_PACKED, s1);
					}
					else {
						s1 = codegen_reg_of_dst(jd, iptr, REG_RESULT);
						M_INTMOVE(REG_RESULT, s1);
					}
				}
				else {
					s1 = codegen_reg_of_dst(jd, iptr, REG_FRESULT);
					M_FLTMOVE(REG_FRESULT, s1);
				}
				emit_store_dst(jd, iptr, s1);
			}

			break;


		case ICMD_CHECKCAST:  /* ..., objectref ==> ..., objectref            */

		                      /* val.a: (classinfo*) superclass               */

			/*  superclass is an interface:
			 *	
			 *  OK if ((sub == NULL) ||
			 *         (sub->vftbl->interfacetablelength > super->index) &&
			 *         (sub->vftbl->interfacetable[-super->index] != NULL));
			 *	
			 *  superclass is a class:
			 *	
			 *  OK if ((sub == NULL) || (0
			 *         <= (sub->vftbl->baseval - super->vftbl->baseval) <=
			 *         super->vftbl->diffval));
			 */

			if (!(iptr->flags.bits & INS_FLAG_ARRAY)) {
				/* object type cast-check */

				classinfo *super;
				vftbl_t   *supervftbl;
				s4         superindex;

#				define LABEL_EXIT_CHECK_NULL BRANCH_LABEL_1
#				define LABEL_CLASS BRANCH_LABEL_2
#				define LABEL_EXIT_INTERFACE_NULL BRANCH_LABEL_3
#				define LABEL_EXIT_INTERFACE_DONE BRANCH_LABEL_4
#				define LABEL_EXIT_CLASS_NULL BRANCH_LABEL_5

				if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
					super = NULL;
					superindex = 0;
					supervftbl = NULL;
				}
				else {
					super = iptr->sx.s23.s3.c.cls;
					superindex = super->index;
					supervftbl = super->vftbl;
				}

				if ((super == NULL) || !(super->flags & ACC_INTERFACE))
					CODEGEN_CRITICAL_SECTION_NEW;

				s1 = emit_load_s1(jd, iptr, REG_ITMP1);

				/* if class is not resolved, check which code to call */

				if (super == NULL) {
					M_TEST(s1);
					emit_label_beq(cd, LABEL_EXIT_CHECK_NULL);

					disp = dseg_add_unique_s4(cd, 0);         /* super->flags */

					patcher_add_patch_ref(jd, PATCHER_resolve_classref_to_flags,
										  iptr->sx.s23.s3.c.ref,
										  disp);

					ICONST(REG_ITMP3, ACC_INTERFACE);

					if (N_VALID_DSEG_DISP(disp)) {
						N_N(REG_ITMP3, N_DSEG_DISP(disp), RN, REG_PV);
					} else {
						ICONST(REG_ITMP2, disp);
						N_N(REG_ITMP3, -N_PV_OFFSET, REG_ITMP2, REG_PV);
					}
					emit_label_beq(cd, LABEL_CLASS);
				}

				/* interface checkcast code */

				if ((super == NULL) || (super->flags & ACC_INTERFACE)) {
					if (super == NULL) {
						patcher_add_patch_ref(jd,
											  PATCHER_checkcast_instanceof_interface,
											  iptr->sx.s23.s3.c.ref,
											  0);
					} else {
						M_TEST(s1);
						emit_label_beq(cd, LABEL_EXIT_INTERFACE_NULL);
					}

					M_ALD(REG_ITMP2, s1, OFFSET(java_object_t, vftbl));
					M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, interfacetablelength));
					M_ISUB_IMM(superindex, REG_ITMP3);
					emit_classcast_check(cd, iptr, BRANCH_LE, RN, s1);
					N_AHI(
						REG_ITMP2,
						(s4) (OFFSET(vftbl_t, interfacetable[0]) -
							superindex * sizeof(methodptr*))
					);
					M_ALD(REG_ITMP2, REG_ITMP2, 0);
					emit_classcast_check(cd, iptr, BRANCH_EQ, REG_ITMP2, s1);

					if (super == NULL) {
						emit_label_br(cd, LABEL_EXIT_INTERFACE_DONE);
					}
				}

				/* class checkcast code */
				
				if (super == NULL) {
					emit_label(cd, LABEL_CLASS);
				}

				if ((super == NULL) || !(super->flags & ACC_INTERFACE)) {
					if (super == NULL) {
						disp = dseg_add_unique_address(cd, NULL);

						patcher_add_patch_ref(jd,
											  PATCHER_resolve_classref_to_vftbl,
											  iptr->sx.s23.s3.c.ref,
											  disp);
					}
					else {
						disp = dseg_add_address(cd, supervftbl);
						M_TEST(s1);
						emit_label_beq(cd, LABEL_EXIT_CLASS_NULL);
					}

#if 1
					CODEGEN_CRITICAL_SECTION_START;

					/* REG_ITMP3 := baseval(s1) */
					M_ALD(REG_ITMP2, s1, OFFSET(java_object_t, vftbl));
					M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, baseval));

					/* REG_ITMP2 := baseval(class) */
					M_ALD_DSEG(REG_ITMP2, disp);
					M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, baseval));

					/* REG_ITMP3 := REG_ITMP3 - REG_ITMP2 */
					M_ISUB(REG_ITMP2, REG_ITMP3);

					/* REG_ITMP2 := diffval(class) */
					M_ALD_DSEG(REG_ITMP2, disp);
					M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, diffval));

					CODEGEN_CRITICAL_SECTION_END;

					M_CMPU(REG_ITMP3, REG_ITMP2); /* Unsigned compare */

					/* M_CMPULE(REG_ITMP2, REG_ITMP3, REG_ITMP3); itmp3 = (itmp2 <= itmp3) */
					/* M_BEQZ(REG_ITMP3, 0); branch if (! itmp) -> branch if > */
					/* Branch if greater then */
#else
					M_ALD(REG_ITMP2, s1, OFFSET(java_object_t, vftbl));
					M_ALD_DSEG(REG_ITMP3, disp);

					CODEGEN_CRITICAL_SECTION_START;

					M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, baseval));
					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, baseval));
					M_ISUB(REG_ITMP3, REG_ITMP2);
					M_ALD_DSEG(REG_ITMP3, disp);
					M_ILD(REG_ITMP3, REG_ITMP3, OFFSET(vftbl_t, diffval));

					CODEGEN_CRITICAL_SECTION_END;
					
					M_CMPU(REG_ITMP2, REG_ITMP3); /* Unsigned compare */
					/* M_CMPULE(REG_ITMP2, REG_ITMP3, REG_ITMP3); itmp3 = (itmp2 <= itmp3) */
					/* M_BEQZ(REG_ITMP3, 0); branch if (! itmp) -> branch if > */
					/* Branch if greater then */
#endif
					emit_classcast_check(cd, iptr, BRANCH_GT, RN, s1);
				}

				if (super == NULL) {
					emit_label(cd, LABEL_EXIT_CHECK_NULL);
					emit_label(cd, LABEL_EXIT_INTERFACE_DONE);
				} else if (super->flags & ACC_INTERFACE) {
					emit_label(cd, LABEL_EXIT_INTERFACE_NULL);
				} else {
					emit_label(cd, LABEL_EXIT_CLASS_NULL);
				}

				d = codegen_reg_of_dst(jd, iptr, s1);

#				undef LABEL_EXIT_CHECK_NULL
#				undef LABEL_CLASS
#				undef LABEL_EXIT_INTERFACE_NULL
#				undef LABEL_EXIT_INTERFACE_DONE
#				undef LABEL_EXIT_CLASS_NULL
			}
			else {
				/* array type cast-check */

				s1 = emit_load_s1(jd, iptr, REG_A0);
				M_INTMOVE(s1, REG_A0);

				if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
					disp = dseg_add_unique_address(cd, NULL);

					patcher_add_patch_ref(jd,
										  PATCHER_resolve_classref_to_classinfo,
										  iptr->sx.s23.s3.c.ref,
										  disp);
				}
				else
					disp = dseg_add_address(cd, iptr->sx.s23.s3.c.cls);

				M_ALD_DSEG(REG_A1, disp);
				disp = dseg_add_functionptr(cd, BUILTIN_arraycheckcast);
				M_ALD_DSEG(REG_ITMP1, disp);
				M_ASUB_IMM(96, REG_SP);
				M_JSR(REG_RA, REG_ITMP1);
				M_AADD_IMM(96, REG_SP);

				s1 = emit_load_s1(jd, iptr, REG_ITMP1);
				emit_classcast_check(cd, iptr, BRANCH_EQ, REG_RESULT, s1);

				d = codegen_reg_of_dst(jd, iptr, s1);
			}

			M_INTMOVE(s1, d);
			emit_store_dst(jd, iptr, d);
			break;

		case ICMD_INSTANCEOF: /* ..., objectref ==> ..., intresult            */
		                      /* val.a: (classinfo*) superclass               */

			/*  superclass is an interface:
			 *	
			 *  return (sub != NULL) &&
			 *         (sub->vftbl->interfacetablelength > super->index) &&
			 *         (sub->vftbl->interfacetable[-super->index] != NULL);
			 *	
			 *  superclass is a class:
			 *	
			 *  return ((sub != NULL) && (0
			 *          <= (sub->vftbl->baseval - super->vftbl->baseval) <=
			 *          super->vftbl->diffvall));
			 *
			 *  If superclass is unresolved, we include both code snippets 
			 *  above, a patcher resolves the class' flags and we select
			 *  the right code at runtime.
			 */

			{
			classinfo *super;
			vftbl_t   *supervftbl;
			s4         superindex;

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				super = NULL;
				superindex = 0;
				supervftbl = NULL;

			} else {
				super = iptr->sx.s23.s3.c.cls;
				superindex = super->index;
				supervftbl = super->vftbl;
			}

#			define LABEL_EXIT_CHECK_NULL BRANCH_LABEL_1
#			define LABEL_CLASS BRANCH_LABEL_2
#			define LABEL_EXIT_INTERFACE_NULL BRANCH_LABEL_3
#			define LABEL_EXIT_INTERFACE_INDEX_NOT_IN_TABLE BRANCH_LABEL_4
#			define LABEL_EXIT_INTERFACE_DONE BRANCH_LABEL_5
#			define LABEL_EXIT_CLASS_NULL BRANCH_LABEL_6

			if ((super == NULL) || !(super->flags & ACC_INTERFACE))
				CODEGEN_CRITICAL_SECTION_NEW;

			s1 = emit_load_s1(jd, iptr, REG_ITMP1);
			d = codegen_reg_of_dst(jd, iptr, REG_ITMP2);
			if (s1 == d) {
				M_MOV(s1, REG_ITMP1);
				s1 = REG_ITMP1;
			}

			/* if class is not resolved, check which code to call */

			if (super == NULL) {
				M_CLR(d);
				
				M_TEST(s1);
				emit_label_beq(cd, LABEL_EXIT_CHECK_NULL);

				disp = dseg_add_unique_s4(cd, 0);             /* super->flags */

				patcher_add_patch_ref(jd, PATCHER_resolve_classref_to_flags,
									  iptr->sx.s23.s3.c.ref, disp);

				ICONST(REG_ITMP3, ACC_INTERFACE);

				if (N_VALID_DSEG_DISP(disp)) {
					N_N(REG_ITMP3, N_DSEG_DISP(disp), RN, REG_PV);
				} else {
					ICONST(REG_ITMP2, disp);
					N_N(REG_ITMP3, -N_PV_OFFSET, REG_ITMP2, REG_PV);
				}

				emit_label_beq(cd, LABEL_CLASS);
			}

			/* interface instanceof code */

			if ((super == NULL) || (super->flags & ACC_INTERFACE)) {
				if (super == NULL) {
					/* If d == REG_ITMP2, then it's destroyed in check
					   code above. */
					if (d == REG_ITMP2)
						M_CLR(d);

					patcher_add_patch_ref(jd,
										  PATCHER_checkcast_instanceof_interface,
										  iptr->sx.s23.s3.c.ref, 0);
				}
				else {
					M_CLR(d);
					M_TEST(s1);
					emit_label_beq(cd, LABEL_EXIT_INTERFACE_NULL);
				}

				M_ALD(REG_ITMP1, s1, OFFSET(java_object_t, vftbl));
				M_ILD(REG_ITMP3, REG_ITMP1, OFFSET(vftbl_t, interfacetablelength));
				M_ISUB_IMM(superindex, REG_ITMP3);

				emit_label_ble(cd, LABEL_EXIT_INTERFACE_INDEX_NOT_IN_TABLE);

				N_AHI(
					REG_ITMP1,
					(s4) (OFFSET(vftbl_t, interfacetable[0]) -
						superindex * sizeof(methodptr*))
				);
				M_ALD(REG_ITMP1, REG_ITMP1, 0);
				
				/* d := (REG_ITMP1 != 0)  */

				N_LTR(d, REG_ITMP1);
				M_BEQ(SZ_BRC + SZ_LHI);
				N_LHI(d, 1);

				if (super == NULL) {
					emit_label_br(cd, LABEL_EXIT_INTERFACE_DONE);
				}
			}

			/* class instanceof code */

			if (super == NULL) {
				emit_label(cd, LABEL_CLASS);
			}

			if ((super == NULL) || !(super->flags & ACC_INTERFACE)) {
				if (super == NULL) {
					disp = dseg_add_unique_address(cd, NULL);

					patcher_add_patch_ref(jd, PATCHER_resolve_classref_to_vftbl,
										  iptr->sx.s23.s3.c.ref,
										  disp);
				}
				else {
					disp = dseg_add_address(cd, supervftbl);

					M_CLR(d);

					M_TEST(s1);
					emit_label_beq(cd, LABEL_EXIT_CLASS_NULL);
				}

				M_ALD(REG_ITMP1, s1, OFFSET(java_object_t, vftbl));
				M_ALD_DSEG(REG_ITMP2, disp);

				CODEGEN_CRITICAL_SECTION_START;

				M_ILD(REG_ITMP1, REG_ITMP1, OFFSET(vftbl_t, baseval));
				M_ILD(REG_ITMP3, REG_ITMP2, OFFSET(vftbl_t, baseval));
				M_ILD(REG_ITMP2, REG_ITMP2, OFFSET(vftbl_t, diffval));

				CODEGEN_CRITICAL_SECTION_END;

				M_ISUB(REG_ITMP3, REG_ITMP1); /* itmp1 :=  itmp1 (sub.baseval) - itmp3 (super.baseval) */

				M_CMPU(REG_ITMP1, REG_ITMP2); /* d := (uint)REG_ITMP1 <= (uint)REG_ITMP2 */
				N_LHI(d, 0);
				M_BGT(SZ_BRC + SZ_LHI);
				N_LHI(d, 1);
			}

			if (super == NULL) {
				emit_label(cd, LABEL_EXIT_CHECK_NULL);
				emit_label(cd, LABEL_EXIT_INTERFACE_DONE);
				emit_label(cd, LABEL_EXIT_INTERFACE_INDEX_NOT_IN_TABLE);
			} else if (super->flags & ACC_INTERFACE) {
				emit_label(cd, LABEL_EXIT_INTERFACE_NULL);
				emit_label(cd, LABEL_EXIT_INTERFACE_INDEX_NOT_IN_TABLE);
			} else {
				emit_label(cd, LABEL_EXIT_CLASS_NULL);
			}

#			undef LABEL_EXIT_CHECK_NULL
#			undef LABEL_CLASS
#			undef LABEL_EXIT_INTERFACE_NULL
#			undef LABEL_EXIT_INTERFACE_INDEX_NOT_IN_TABLE
#			undef LABEL_EXIT_INTERFACE_DONE
#			undef LABEL_EXIT_CLASS_NULL
				
			emit_store_dst(jd, iptr, d);

			}

			break;

		case ICMD_MULTIANEWARRAY:/* ..., cnt1, [cnt2, ...] ==> ..., arrayref  */

			/* check for negative sizes and copy sizes to stack if necessary  */

  			/*MCODECHECK((10 * 4 * iptr->s1.argcount) + 5 + 10 * 8);*/
			MCODECHECK(512);

			for (s1 = iptr->s1.argcount; --s1 >= 0; ) {

				/* copy SAVEDVAR sizes to stack */
				var = VAR(iptr->sx.s23.s2.args[s1]);

				/* Already Preallocated? */
				if (!(var->flags & PREALLOC)) {
					s2 = emit_load(jd, iptr, var, REG_ITMP1);
					M_IST(s2, REG_SP, s1 * 4);
				}
			}

			/* is a patcher function set? */

			if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
				disp = dseg_add_unique_address(cd, 0);

				patcher_add_patch_ref(jd, PATCHER_resolve_classref_to_classinfo,
									  iptr->sx.s23.s3.c.ref,
									  disp);
			}
			else
				disp = dseg_add_address(cd, iptr->sx.s23.s3.c.cls);

			/* a0 = dimension count */

			ICONST(REG_A0, iptr->s1.argcount);

			/* a1 = classinfo */

			M_ALD_DSEG(REG_A1, disp);

			/* a2 = pointer to dimensions = stack pointer */

			M_MOV(REG_SP, REG_A2);

			disp = dseg_add_functionptr(cd, BUILTIN_multianewarray);
			M_ALD_DSEG(REG_ITMP1, disp);
			M_ASUB_IMM(96, REG_SP);
			M_JSR(REG_RA, REG_ITMP1);
			M_AADD_IMM(96, REG_SP);

			/* check for exception before result assignment */

			emit_exception_check(cd, iptr);

			s1 = codegen_reg_of_dst(jd, iptr, REG_RESULT);
			M_INTMOVE(REG_RESULT, s1);
			emit_store_dst(jd, iptr, s1);

			break;

		default:
			exceptions_throw_internalerror("Unknown ICMD %d", iptr->opc);
			return false;
	} /* switch */

	} /* for instruction */
		
	MCODECHECK(512); /* XXX require a lower number? */

	/* At the end of a basic block we may have to append some nops,
	   because the patcher stub calling code might be longer than the
	   actual instruction. So codepatching does not change the
	   following block unintentionally. */

	if (cd->mcodeptr < cd->lastmcodeptr) {
		while (cd->mcodeptr < cd->lastmcodeptr) {
			M_NOP;
		}
	}

	} /* if (bptr -> flags >= BBREACHED) */
	} /* for basic block */

	/* generate stubs */

	emit_patcher_traps(jd);

	/* everything's ok */

	return true;
}

/* codegen_emit_stub_native ****************************************************

   Emits a stub routine which calls a native method.

*******************************************************************************/

/*
           arguments on stack                   \
-------------------------------------------------| <- SP on nativestub entry
           return address                        |
           callee saved int regs (none)          |
           callee saved float regs (none)        | stack frame like in cacao
           local variable slots (none)           |
           arguments for calling methods (none) /
------------------------------------------------------------------ <- datasp 
           stackframe info
           locaref table
           integer arguments
           float arguments
96 - ...   on stack parameters (nmd->memuse slots) \ stack frame like in ABI         
0 - 96     register save area for callee           /
-------------------------------------------------------- <- SP native method
                                                            ==
                                                            SP after method entry
*/

void codegen_emit_stub_native(jitdata *jd, methoddesc *nmd, functionptr f, int skipparams)
{
	methodinfo  *m;
	codeinfo    *code;
	codegendata *cd;
	methoddesc  *md;
	int          i, j;
	int          t;
	int          s1, s2;
	int          disp;

	/* get required compiler data */

	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;

	/* set some variables */

	md = m->parseddesc;

	/* calculate stackframe size */

	cd->stackframesize =
		1 + /* return address */
		sizeof(stackframeinfo_t) / 8 +
		sizeof(localref_table) / 8 +
		nmd->paramcount +
		nmd->memuse +
		(96 / 8); /* linkage area */

	/* keep stack 8-byte aligned */

	/*ALIGN_2(cd->stackframesize);*/

	/* create method header */

	(void) dseg_add_unique_address(cd, code);              /* CodeinfoPointer */
	(void) dseg_add_unique_s4(cd, cd->stackframesize * 8); /* FrameSize       */
	(void) dseg_add_unique_s4(cd, 0);                      /* IsLeaf          */
	(void) dseg_add_unique_s4(cd, 0);                      /* IntSave         */
	(void) dseg_add_unique_s4(cd, 0);                      /* FltSave         */

	/* generate code */

	M_ASUB_IMM(cd->stackframesize * 8, REG_SP);
	M_AADD_IMM(N_PV_OFFSET, REG_PV);

	/* store return address */

	M_AST(REG_RA, REG_SP, (cd->stackframesize - 1) * 8);

#if defined(ENABLE_GC_CACAO)
	/* Save callee saved integer registers in stackframeinfo (GC may
	   need to recover them during a collection). */

	disp = cd->stackframesize * 8 - sizeof(stackframeinfo_t) +
		OFFSET(stackframeinfo_t, intregs);

	for (i = 0; i < INT_SAV_CNT; i++)
		M_AST(abi_registers_integer_saved[i], REG_SP, disp + i * 4);
#endif

	/* save integer and float argument registers */

	for (i = 0; i < md->paramcount; i++) {
		if (!md->params[i].inmemory) {
			s1 = md->params[i].regoff;

			switch (md->paramtypes[i].type) {
			case TYPE_INT:
			case TYPE_ADR:
				M_IST(s1, REG_SP, 96 + i * 8);
				break;
			case TYPE_LNG:
				M_LST(s1, REG_SP, 96 + i * 8);
				break;
			case TYPE_FLT:
			case TYPE_DBL:
				M_DST(s1, REG_SP, 96 + i * 8);
				break;
			}
		}
	}

	/* create native stack info */

	M_MOV(REG_SP, REG_A0);
	M_LDA(REG_A1, REG_PV, -N_PV_OFFSET);
	disp = dseg_add_functionptr(cd, codegen_start_native_call);
	M_ALD_DSEG(REG_ITMP2, disp);
	M_CALL(REG_ITMP2);

	/* remember class argument */

	if (m->flags & ACC_STATIC)
		M_MOV(REG_RESULT, REG_ITMP3);

	/* restore integer and float argument registers */

	for (i = 0; i < md->paramcount; i++) {
		if (!md->params[i].inmemory) {
			s1 = md->params[i].regoff;

			switch (md->paramtypes[i].type) {
			case TYPE_INT:
			case TYPE_ADR:
				M_ILD(s1, REG_SP, 96 + i * 8);
				break;
			case TYPE_LNG:
				M_LLD(s1, REG_SP, 96 + i * 8);
				break;
			case TYPE_FLT:
			case TYPE_DBL:
				M_DLD(s1, REG_SP, 96 + i * 8);
				break;
			}
		}
	}

	/* copy or spill arguments to new locations */

	for (i = md->paramcount - 1, j = i + skipparams; i >= 0; i--, j--) {
		t = md->paramtypes[i].type;

		if (IS_INT_LNG_TYPE(t)) {
			if (!md->params[i].inmemory) {
				s1 = md->params[i].regoff;
				s2 = nmd->params[j].regoff;

				if (!nmd->params[j].inmemory) {
					if (IS_2_WORD_TYPE(t))
						M_LNGMOVE(s1, s2);
					else
						M_INTMOVE(s1, s2);
				}
				else {
					if (IS_2_WORD_TYPE(t))
						M_LST(s1, REG_SP, s2);
					else
						M_IST(s1, REG_SP, s2);
				}
			}
			else {
				s1 = md->params[i].regoff + cd->stackframesize * 8;
				s2 = nmd->params[j].regoff;

				if (IS_2_WORD_TYPE(t)) {
					N_MVC(s2, 8, REG_SP, s1, REG_SP);
				} else {
					N_MVC(s2, 4, REG_SP, s1, REG_SP);
				}
			}
		}
		else {
			/* We only copy spilled float arguments, as the float
			   argument registers keep unchanged. */

			if (md->params[i].inmemory) {
				s1 = md->params[i].regoff + cd->stackframesize * 8;
				s2 = nmd->params[j].regoff;

				if (IS_2_WORD_TYPE(t)) {
					N_MVC(s2, 8, REG_SP, s1, REG_SP);
				} else {
					N_MVC(s2, 4, REG_SP, s1, REG_SP);
				}
			}
		}
	}

	/* Handle native Java methods. */

	if (m->flags & ACC_NATIVE) {
		/* put class into second argument register */

		if (m->flags & ACC_STATIC)
			M_MOV(REG_ITMP3, REG_A1);

		/* put env into first argument register */

		disp = dseg_add_address(cd, _Jv_env);
		M_ALD_DSEG(REG_A0, disp);
	}

	/* Call native function. */

	disp = dseg_add_functionptr(cd, f);
	M_ALD_DSEG(REG_ITMP2, disp);
	M_CALL(REG_ITMP2);

	/* save return value */

	switch (md->returntype.type) {
	case TYPE_INT:
	case TYPE_ADR:
		M_IST(REG_RESULT, REG_SP, 96);
		break;
	case TYPE_LNG:
		M_LST(REG_RESULT_PACKED, REG_SP, 96);
		break;
	case TYPE_FLT:
	case TYPE_DBL:
		M_DST(REG_FRESULT, REG_SP, 96);
		break;
	case TYPE_VOID:
		break;
	}

	/* remove native stackframe info */

	M_MOV(REG_SP, REG_A0);
	M_LDA(REG_A1, REG_PV, -N_PV_OFFSET);
	disp = dseg_add_functionptr(cd, codegen_finish_native_call);
	M_ALD_DSEG(REG_ITMP1, disp);
	M_CALL(REG_ITMP1);

	M_MOV(REG_RESULT, REG_ITMP3_XPTR);

	/* restore return value */

	switch (md->returntype.type) {
	case TYPE_INT:
	case TYPE_ADR:
		M_ILD(REG_RESULT, REG_SP, 96);
		break;
	case TYPE_LNG:
		M_LLD(REG_RESULT_PACKED, REG_SP, 96);
		break;
	case TYPE_FLT:
	case TYPE_DBL:
		M_DLD(REG_FRESULT, REG_SP, 96);
		break;
	case TYPE_VOID:
		break;
	}

#if defined(ENABLE_GC_CACAO)
	/* Restore callee saved integer registers from stackframeinfo (GC
	   might have modified them during a collection). */
  	 
	disp = cd->stackframesize * 8 - sizeof(stackframeinfo_t) +
		OFFSET(stackframeinfo_t, intregs);

	for (i = 0; i < INT_SAV_CNT; i++)
		M_ALD(abi_registers_integer_saved[i], REG_SP, disp + i * 4);
#endif

	/* load return address */

	M_ALD(REG_RA, REG_SP, (cd->stackframesize - 1) * 8);

	/* remove stackframe       */

	M_AADD_IMM(cd->stackframesize * 8, REG_SP);

	/* check for exception */

	M_TEST(REG_ITMP3_XPTR);
	M_BNE(SZ_BRC + SZ_BCR);                     /* if no exception then return        */

	M_RET;

	/* handle exception */

	M_MOV(REG_RA, REG_ITMP1_XPC);
	M_ASUB_IMM(2, REG_ITMP1_XPC);

	disp = dseg_add_functionptr(cd, asm_handle_nat_exception);
	M_ALD_DSEG(REG_ITMP2, disp);
	M_JMP(RN, REG_ITMP2);
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
 * vim:noexpandtab:sw=4:ts=4:
 */
