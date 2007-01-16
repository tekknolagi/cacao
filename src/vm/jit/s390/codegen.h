/* src/vm/jit/x86_64/codegen.h - code generation macros for x86_64

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

   Authors: Andreas Krall
            Christian Thalinger

   $Id: codegen.h 7219 2007-01-16 22:18:57Z pm $

*/


#ifndef _CODEGEN_H
#define _CODEGEN_H

#include "config.h"

#include <ucontext.h>

#include "vm/types.h"

#include "vm/jit/jit.h"


/* additional functions and macros to generate code ***************************/

#define CALCOFFSETBYTES(var, reg, val) \
    if ((s4) (val) < -128 || (s4) (val) > 127) (var) += 4; \
    else if ((s4) (val) != 0) (var) += 1; \
    else if ((reg) == RBP || (reg) == RSP || (reg) == R12 || (reg) == R13) (var) += 1;


#define CALCIMMEDIATEBYTES(var, val) \
    if ((s4) (val) < -128 || (s4) (val) > 127) (var) += 4; \
    else (var) += 1;


/* gen_nullptr_check(objreg) */

#define gen_nullptr_check(objreg) \
	if (checknull) { \
        M_TEST(objreg); \
        M_BEQ(0); \
 	    codegen_add_nullpointerexception_ref(cd); \
	}


#define gen_bound_check \
    if (checkbounds) { \
        M_ILD(REG_ITMP3, s1, OFFSET(java_arrayheader, size));\
        M_ICMP(REG_ITMP3, s2); \
        M_BAE(0); \
        codegen_add_arrayindexoutofboundsexception_ref(cd, s2); \
    }


/* MCODECHECK(icnt) */

#define MCODECHECK(icnt) \
    do { \
        if ((cd->mcodeptr + (icnt)) > cd->mcodeend) \
            codegen_increase(cd); \
    } while (0)


#define ALIGNCODENOP \
    if ((s4) (((ptrint) cd->mcodeptr) & 7)) { \
        M_NOP; \
    }


/* M_INTMOVE:
    generates an integer-move from register a to b.
    if a and b are the same int-register, no code will be generated.
*/ 

#define M_INTMOVE(reg,dreg) \
    do { \
        if ((reg) != (dreg)) { \
            M_MOV(reg, dreg); \
        } \
    } while (0)


/* M_FLTMOVE:
    generates a floating-point-move from register a to b.
    if a and b are the same float-register, no code will be generated
*/ 

#define M_FLTMOVE(reg,dreg) \
    do { \
        if ((reg) != (dreg)) { \
            M_FMOV(reg, dreg); \
        } \
    } while (0)


#define ICONST(r,c) \
    do { \
        if ((c) == 0) \
            M_CLR((d)); \
        else \
            M_IMOV_IMM((c), (d)); \
    } while (0)
/*     do { \ */
/*        M_IMOV_IMM((c), (d)); \ */
/*     } while (0) */


#define LCONST(r,c) \
    do { \
        if ((c) == 0) \
            M_CLR((d)); \
        else \
            M_MOV_IMM((c), (d)); \
    } while (0)


/* some patcher defines *******************************************************/

#define PATCHER_CALL_SIZE    5          /* size in bytes of a patcher call    */

#define PATCHER_NOPS \
    do { \
        M_NOP; \
        M_NOP; \
        M_NOP; \
        M_NOP; \
        M_NOP; \
    } while (0)


/* macros to create code ******************************************************/

/* Conventions:
 * N_foo:   defines the instrucition foo as in `ESA/390 Principles of operations'
 * SZ_foo:  defines the size of the instruction N_foo
 * DD_foo:  defines a condition code as used by s390 GCC
 * M_foo:   defines the alpha like instruction used in cacao
 *          the instruction is defined by an equivalent N_ instruction
 * CC_foo:  defines a condition code as used
 *          the instruction is defined as an equivalent DD_ condition code
 */

/* S390 specific code */

/* Instruction formats */

#define _CODE(t, code) \
	do { \
		*((t *) cd->mcodeptr) = (code); \
		cd->mcodeptr += sizeof(t); \
	} while (0);

#define _CODE2(code) _CODE(u2, code)
#define _CODE4(code) _CODE(u4, code)

#define _IFNEG(val, neg, pos) \
	do { if ((val) < 0) { neg ; } else { pos ; } } while (0)

#define N_RR(op, r1, r2) \
	_CODE2( (op << 8) | (r1 << 4) | (r2) )

#define SZ_RR 2

#define N_RR2(op, i) \
	_CODE2( (op << 8) | (i) )

#define N_RX(op, r1, d2, x2, b2) \
	_CODE4( ((op) << 24) | ((r1) << 20) | ((x2) << 16) | ((b2) << 12) | ((d2) << 0) )

#define SZ_RX 4

#define N_RI(op1, op2, r1, i2) \
	_CODE4( ((op1) << 24) | ((r1) << 20) | ((op2) << 16) | ((u2) i2) )

#define SZ_RI 4

#define N_SI(op, d1, b1, i2) \
	_CODE4( ((op) << 24) | ((i2) << 16) | ((b1) << 12) | (d1) )

#define SZ_SI 4

#define N_SS(op, d1, l, b1, d2, b2) \
	_CODE4( ((op) << 24) | ((l) << 16) | ((b1) << 12) | (d1) ) \
	_CODE2( ((b2) << 12) | (d2) )

#define SZ_SS 6

#define N_SS2(op, d1, l1, b1, d2, l2, b2) \
	N_SS(op, d1, ((l1) << 4) | (l2), b1, d2, l2)

#define N_RS(op, r1, r3, d2, b2) \
	_CODE4( ((op) << 24) | ((r1) << 20) | ((r3) << 16) | ((b2) << 12) | (d2) )

#define SZ_RS 4

#define N_RSI(op, r1, r2, i2) \
	_CODE4( ((op) << 24) | ((r1) << 20) | ((r3) << 16) | ((u2)i2) )

#define SZ_RSI 4

#define N_RRE(op, r1, r2) \
	_CODE4( ((op) << 16) | ((r1) << 4) | (r2) )

#define SZ_RRE 4

#define N_S2(d2, b2) \
	_CODE4( ((op) << 16) | ((b2) << 12) | (d2)  )

#define SZ_S2 4

#define N_E(op) \
	_CODE2( (op) )

#define SZ_E 2

/* Condition codes */

#define DD_O 1
#define DD_H 2
#define DD_NLE 3
#define DD_L 4
#define DD_NHE 5
#define DD_LH 6
#define DD_NE 7
#define DD_E 8
#define DD_NLH 9
#define DD_HE 10
#define DD_NL 11
#define DD_LE 12
#define DD_NH 13
#define DD_NO 14
#define DD_ANY 15

/* Misc */

#define N_LONG_0() _CODE4(0)

/* Chapter 7. General instructions */

#define N_AR(r1, r2) N_RR(0x1A, r1, r2)
#define N_A(r1, d2, x2, b2) N_RX(0x5A, r1, d2, x2, b2)
#define N_AH(r1, d2, x2, b2) N_RX(0x4A, r1, d2, x2, b2)
#define N_AHI(r1, i2) N_RI(0xA7, 0xA, r1, i2)
#	define SZ_AHI SZ_RI
#define N_ALR(r1, r2) N_RR(0x1E, r1, r2)
#define N_AL(r1, d2, x2, b2) N_RX(0x5E, r1, d2, x2, b2)
#define N_NR(r1, r2) N_RR(r1, r2)
#define N_N(r1, d2, x2, b2) N_RX(0x54, r1, d2, x2, b2)
#define N_NI(d1, b1, i2) N_SI(0x94, d1, b1, i2)
#define N_NC(d1, l, b1, d2, b2) N_NC(0xD4, l, b1, d1, b2, d2)
#define N_BALR(r1, r2) N_RR(0x05, r1, r2)
#define N_BAL(r1, d2, x2, b2) N_RX(0x45, r1, d2, x2, b2)
#define N_BASR(r1, r2) N_RR(0x0D, r1, r2)
#define N_BAS(r1, d2, x2, b2) N_RX(0x4D, r1, d2, x2, b2)
#define N_BASSM(r1, r2) N_RR(0x0C, r1, r2)
#define N_BSM(r1, r2) N_RR(0x0B, r1, r2)
#define N_BCR(m1, r2) N_RR(0x07, m1, r2)
#	define SZ_BCR SZ_RR
#	define N_BR(r2) N_BCR(DD_ANY, r2)
#	define SZ_BR SZ_BCR
#define N_BC(m1, d2, x2, b2) N_RS(0x47, m1, d2, x2, b2)
#	define SZ_BC SZ_RS
#define N_BCTR(r1, r2) N_RR(0x06, r1, r2)
#define N_BCT(r1, d2, x2, b2) N_RX(0x46, r1, d2, x2, b2)
#define N_BHX(r1, r2, d2, b2) N_RS(0xB6, r1, r3, d2, b2)
#define N_BXLE(r1, r3, d2, b2) N_RS(0xB7, r1, r3, d2, b2)
#define N_BRAS(r1, i2) N_RI(0xA7, 0x5, r1, (i2) / 2)
#	define SZ_BRAS SZ_RI
#define N_BRC(m1, i2) N_RI(0xA7, 0x4, m1, (i2) / 2)
#	define N_J(i2) N_BRC(DD_ANY, i2)
#	define SZ_BRC SZ_RI
#	define SZ_J SZ_RI
#define N_BRCT(r1, i2) N_RI(0xA7, 0x6, r1, (i2) / 2)
#define N_BRXH(r1, r3, i2) N_RSI(0x84, r1, r3, (i2) / 2)
#define N_BRXLE(r1, r3, i2) N_RSI(0x85, r1, r2, (i2) / 2)
#define N_CKSM(r1, r2) N_RRE(0xB241, r1, r2)
#define N_CR(r1, r2), N_RR(0x19, r1, r2)
#define N_C(r1, d2, x2, b2) N_RX(0x59, r1, d2, x2, b2)
#define N_CFC(d2, b2) N_S2(0xB21A, d2, b2)
#define N_CS(r1, r3, d2, b2) N_RS(0xBA, r1, r3, d2, b2)
#define N_CDS(r1, r3, d2, b2) N_RS(0xBB, r1, r3, d2, b2)
#define N_CH(r1, d2, x2, b2) N_CH(0x49, r1, d2, x2, b2)
#define N_CHI(r1, i2) N_RI(0xA7, 0xE, r1, i2)
#define N_CLR(r1, r2) N_RR(0x15, r1, r2)
#define N_CL(r1, d2, x2, b2) N_RX(0x55, r1, d2, x2, b2)
#define N_CLI(d1, b1, i2) N_SI(0x95, d1, b1, i2)
#define N_CLC(d1, l, b1, d2, b2) N_SS(0xD5, d1, l, b1, d2, b2)
#define N_CLM(r1, m3, d2, b2) N_RS(0xBD, r1, m3, d2, b2)
#define N_CLCL(r1, r2) N_RR(0x0F, r1, r2)
#define N_CLCLE(r1, r3, d2, b2) N_RS(0xA9, r1, r3, d2, b2)
#define N_CLST(r1, r2) N_RRE(0xB25D, r1, r2)
#define N_CUSE(r1, r2) N_RRE(0xB257, r1, r2)
#define N_CVB(r1, d2, x2, b2) N_RX(0x4F, r1, r2, x2, b2)
#define N_CVD(r1, d2, x2, b2) N_RX(0x4E, r1, d2, x2, b2)
#define N_CUUTF(r1, r2) N_RRE(0xB2A6, r1, r2)
#define N_CUTFU(r1, r2) N_RRE(0xB2A7, r1, r2)
#define N_CPYA(r1, r2) N_RRE(0xB240, r1, r2)
#define N_DR(r1, r2) N_RR(0x1D, r1, r2)
#define N_D(r1, d2, x2, b2) N_RX(0x5D, r1, d2, x2, b2)
#define N_XR(r1, r2) N_RR(0x17, r1, r2)
#define N_X(r1, d2, x2, b2) N_RX(0x57, r1, d2, x2, b2)
#define N_XI(d1, b1, i2) N_SI(0x97, d1, b1, i2)
#define N_XC(d1, l, b1, d2, b2) N_SS(0xD7, d1, l, b1, d2, b2)
#define N_EX(r1, d2, x2, b2) N_RX(0x44, r1, d2, x2, b2)
#define N_EAR(r1, r2) N_RRE(0xB24F, r1, r2)
#define N_IC(r1, d2, x2, b2) N_RX(0x43, r1, d2, x2, b2)
#define N_ICM(r1, m3, d2, b2) N_RS(0xBF, r1, m3, d2, b2)
#define N_IPM(r1) N_RRE(0xB222, r1, 0)
#define N_LR(r1, r2) N_RR(0x18, r1, r2)
#define N_L(r1, d2, x2, b2) N_RX(0x58, r1, d2, x2, b2)
#	define SZ_L SZ_RX
#	define N_L2(r1, d2, b2) \
		do { N_LHI(r1, d2); N_L(r1, 0, r1, b2); } while (0)
#define N_LAM(r1, r3, d2, b2) N_RS(0x9A, r1, r3, d2, b2)
#define N_LA(r1, d2, x2, b2) N_RX(0x41, r1, d2, x2, b2)
#	define N_LA2(r1, d2, b2) \
		do { N_LHI(r1, d2); N_LA(r1, 0, r1, b2); } while (0)
#define N_LAE(r1, d2, x2, b2) N_RX(0x51, r1, d2, x2, b2)
#define N_LTR(r1, r2) N_RR(0x12, r1, r2)
#define N_LCR(r1, r2) N_RR(0x13, r1, r2)
#define N_LH(r1, d2, x2, b2) N_RX(0x48, r1, d2, x2, b2)
#define N_LHI(r1, i2) N_RI(0xA7, 0x8, r1, i2)
#define N_LM(r1, r3, d2, b2) N_RS(0x98, r1, r3, d2, b2)
#define N_LNR(r1, r2) N_RR(0x11, r1, r2)
#define N_LPR(r1, r2) N_RR(0x10, r1, r2)
#define N_MC(d1, b1, i2) N_SI(0xAF, d1, b1, i2)
#define N_MVI(d1, b1, i2) N_SI(0x92, d1, b1, i2)
#define N_MVC(d1, l, b1, d2, b2) N_SS(0xD2, d1, l, b1, d2, b2)
#define N_MVCIN(d1, l, b1, d2, b2) N_SS(0xEB, d1, l, b1, d2, b2)
#define N_MVCL(r1, r2) N_RR(0x0E, r1, r2)
#define N_MVCLE(r1, r3, d2, b2)  N_RS(0xAB, r1, r3, d2, b2)
#define N_MVN(d1, l, b1, d2, b2) N_SS(0xD1, d1, l, b1, d2, b2)
#define N_MVPG(r1, r2) N_RRE(0xB254, r1, r2)
#define N_MVST(r1, r2) N_RRE(0xB255, r1, r2)
#define N_MVO(d1, l1, b1, d2, l2, b2) N_SS2(0xF1, d1, l1, b1, d2, l2, b2)
#define N_MVZ(d1, l, b1, d2, b2) N_SS(0xD3, d1, l, b1, d2, b2)
#define N_MR(r1, r2) N_RR(0x1C, r1, r2)
#define N_M(r1, d2, x2, b2) N_RX(0x5C, r1, d2, x2, b2)
#define N_MH(r1, d2, x2, b2) N_RX(0x4C, r1, d2, x2, b2)
#define N_MHI(r1, i2) N_RI(0xA7, 0xC, r1, i2)
#define N_MSR(r1, r2) N_RRE(0xB252, r1, r2)
#define N_MS(r1, d2, x2, b2) N_RX(0x71, r1, d2, x2, b2)
#define N_OR(r1, r2) N_RR(0x16, r1, r2)
#define N_O(r1, d2, x2, b2) N_RX(0x56, r1, d2, x2, b2)
#define N_OI(d1, b1, i2) N_SI(0x96, d1, b1, i2)
#define N_OC(d1, l, b1, d2, b2) N_SS(0xD6, d1, l, b1, d2, b2)
#define N_PACK(d1, l1, b1, d2, l2, b2) N_SS2(0xF2, d1, l1, b1, d2, l2, b2)
#define N_PLO(r1, d2, b2, r3, d4, b4) N_SS2(0xEE, d2, r1, b2, d4, r3, b4)
#define N_SRST(r1, r2) N_RRE(0xB25E, r1, r2)
#define N_SAR(r1, r2) N_RRE(0xB24E, r1, r2)
#define N_SPM(r1) N_RR(0x04, r1, 0x00)
#define N_SLDA(r1, d2, b2) N_RS(0x8F, r1, 0x00, d2, b2)
#define N_SLDL(r1, d2, b2) N_RS(0x8D, r1, 0x00, d2, b2)
#define N_SLA(r1, d2, b2) N_RS(0x8B, r1, 0x00, d2, b2)
#define N_SLL(r1, d2, b2) N_RS(0x89, r1, 0x00, d2, b2)
#define N_SRDA(r1, d2, b2) N_RS(0x8E, r1, 0x00, d2, b2)
#define N_SRDL(r1, d2, b2) N_RS(0x8C, r1, 0x00, d2, b2)
#define N_SRA(r1, d2, b2) N_RS(0x8A, r1, 0x00, d2, b2)
#define N_SRL(r1, d2, b2) N_RS(0x88, r1, 0x00, d2, b2)
#define N_ST(r1, d2, x2, b2) N_RX(0x50, r1, d2, x2, b2)
#define N_STAM(r1, r3, d2, b2) N_RS(0x9B, r1, r3, d2, b2)
#define N_STC(r1, d2, x2, b2) N_RX(0x42, r1, d2, x2, b2)
#define N_STCM(r1, m3, d2, b2) N_RS(0xBE, r1, m3, d2, b2)
#define N_STCK(d2, b2) N_S2(0xB205, d2, b2)
#define N_STCKE(d2, b2) N_S2(0xB278, d2, b2)
#define N_STH(r1, d2, x2, b2) N_RX(0x40, r1, d2, x2, b2)
#define N_STM(r1, r3, d2, b2) N_RS(0x90, r1, r3, d2, b2)
#define N_SR(r1, r2) N_RR(0x1B, r1, r2)
#define N_S(r1, d2, x2, b2) N_RX(0x5B, r1, d2, x2, b2)
#define N_SH(r1, d2, x2, b2) N_RX(0x4B, r1, d2, x2, b2)
#define N_SLR(r1, r2) N_RR(0x1F, r1, r2)
#define N_SL(r1, d2, x2, b2) N_RX(0x5F, r1, d2, x2, b2)
#define N_SVC(i) N_RR2(0x0A, i)
#define N_TS(d2, b2) N_S2(0x93, d2, b2)
#define N_TM(d1, b1, i2) N_SI(0x91, d1, b1, i2)
#define N_TMH(r1, i2) N_RI(0xA7, 0x00, r1, i2)
#define N_TML(r1, i2) N_RI(0xA7, 0x01, r1, i2)
#define N_TR(d1, l, b1, d2, b2) N_SS(0xDC, d1, l, b1, d2, b2)
#define N_TRT(d1, l, b1, d2, b2) N_SS(0xDD, d1, l, b1, d2, b2)
#define N_TRE(r1, r2) N_RRE(0xB2A5, r1, r2)
#define N_UNPK(d1, l1, b1, d2, l2, b2) N_SS2(0xF3, d1, l1, b1, d2, l2, b2)
#define N_UPT() N_E(0x0102)

/* Chapter 9. Floating point instructions */

#define N_LER(r1, r2) N_RR(0x38, r1, r2)
#define N_LDR(r1, r2) N_RR(0x28, r1, r2)
#define N_LXR(r1, r2) N_RRE(0xB365, r1, r2)
#define N_LE(r1, d2, x2, b2) N_RX(0x78, r1, d2, x2, b2)
#define N_LD(r1, d2, x2, b2) N_RX(0x68, r1, d2, x2, b2)
#define N_LZER(r1) N_RRE(0xB374, r1, 0x0)
#define N_LZDR(r1) N_RRE(0xB375, r1, 0x0)
#define N_LZXR(r1) N_RRE(0xB376, r1, 0x0)
#define N_STE(r1, d2, x2, b2) N_RX(0x70, r1, d2, x2, b2)
#define N_STD(r1, d2, x2, b2) N_RX(0x60, r1, d2, x2, b2)

/* chapter 19. Binary floating point instructions */

/* Alpha like instructions */

#define M_CALL(r2) N_BASR(R14, r2)

#define M_ALD(r, b, d) _IFNEG(d, N_L2(r, d, b), N_L(r, d, 0, b))
#define M_ILD(r, b, d) _IFNEG(d, N_LA2(r, d, b), N_LA(r, d, 0, b))
#define M_FLD(r, b, d) _IFNEG(d, assert(0), N_LE(r, d, 0, b))
#define M_DLD(r, b, d) _IFNEG(d, assert(0), N_LD(r, d, 0, b))
/* TODO 3 instead of 4 instrs for d < 0 ! */
#define M_LLD(r, b, d) \
	do { M_ILD(GET_HIGH_REG(r), b, d); M_ILD(GET_LOW_REG(r), b, d + 4); } while(0)

/* ----------------------------------------------- */

#define M_MOV(a,b)              emit_mov_reg_reg(cd, (a), (b))
#define M_MOV_IMM(a,b)          emit_mov_imm_reg(cd, (u8) (a), (b))

#define M_IMOV(a,b)             emit_movl_reg_reg(cd, (a), (b))
#define M_IMOV_IMM(a,b)         emit_movl_imm_reg(cd, (u4) (a), (b))

#define M_FMOV(a,b)             emit_movq_reg_reg(cd, (a), (b))

#define M__ILD(a,b,disp)         emit_movl_membase_reg(cd, (b), (disp), (a))
#define M__LLD(a,b,disp)         emit_mov_membase_reg(cd, (b), (disp), (a))

#define M_ILD32(a,b,disp)       emit_movl_membase32_reg(cd, (b), (disp), (a))
#define M_LLD32(a,b,disp)       emit_mov_membase32_reg(cd, (b), (disp), (a))

#define M_IST(a,b,disp)         emit_movl_reg_membase(cd, (a), (b), (disp))
#define M_LST(a,b,disp)         emit_mov_reg_membase(cd, (a), (b), (disp))

#define M_IST_IMM(a,b,disp)     emit_movl_imm_membase(cd, (a), (b), (disp))
#define M_LST_IMM32(a,b,disp)   emit_mov_imm_membase(cd, (a), (b), (disp))

#define M_IST32(a,b,disp)       emit_movl_reg_membase32(cd, (a), (b), (disp))
#define M_LST32(a,b,disp)       emit_mov_reg_membase32(cd, (a), (b), (disp))

#define M_IST32_IMM(a,b,disp)   emit_movl_imm_membase32(cd, (a), (b), (disp))
#define M_LST32_IMM32(a,b,disp) emit_mov_imm_membase32(cd, (a), (b), (disp))

#define M_IADD(a,b)             emit_alul_reg_reg(cd, ALU_ADD, (a), (b))
#define M_ISUB(a,b)             emit_alul_reg_reg(cd, ALU_SUB, (a), (b))
#define M_IMUL(a,b)             emit_imull_reg_reg(cd, (a), (b))

#define M_IADD_IMM(a,b)         emit_alul_imm_reg(cd, ALU_ADD, (a), (b))
#define M_ISUB_IMM(a,b)         emit_alul_imm_reg(cd, ALU_SUB, (a), (b))
#define M_IMUL_IMM(a,b,c)       emit_imull_imm_reg_reg(cd, (b), (a), (c))

#define M_LADD(a,b)             emit_alu_reg_reg(cd, ALU_ADD, (a), (b))
#define M_LSUB(a,b)             emit_alu_reg_reg(cd, ALU_SUB, (a), (b))
#define M_LMUL(a,b)             emit_imul_reg_reg(cd, (a), (b))

#define M_LADD_IMM(a,b)         emit_alu_imm_reg(cd, ALU_ADD, (a), (b))
#define M_LSUB_IMM(a,b)         emit_alu_imm_reg(cd, ALU_SUB, (a), (b))
#define M_LMUL_IMM(a,b,c)       emit_imul_imm_reg_reg(cd, (b), (a), (c))

#define M_IINC(a)               emit_incl_reg(cd, (a))
#define M_IDEC(a)               emit_decl_reg(cd, (a))

#define M__ALD(a,b,disp)         M_LLD(a,b,disp)
#define M_ALD32(a,b,disp)       M_LLD32(a,b,disp)

#define M_AST(a,b,c)            M_LST(a,b,c)
#define M_AST_IMM32(a,b,c)      M_LST_IMM32(a,b,c)

#define M_AADD(a,b)             M_LADD(a,b)
#define M_AADD_IMM(a,b)         M_LADD_IMM(a,b)
#define M_ASUB_IMM(a,b)         M_LSUB_IMM(a,b)

#define M_LADD_IMM32(a,b)       emit_alu_imm32_reg(cd, ALU_ADD, (a), (b))
#define M_AADD_IMM32(a,b)       M_LADD_IMM32(a,b)
#define M_LSUB_IMM32(a,b)       emit_alu_imm32_reg(cd, ALU_SUB, (a), (b))

#define M_ILEA(a,b,c)           emit_leal_membase_reg(cd, (a), (b), (c))
#define M_LLEA(a,b,c)           emit_lea_membase_reg(cd, (a), (b), (c))
#define M_ALEA(a,b,c)           M_LLEA(a,b,c)

#define M_INEG(a)               emit_negl_reg(cd, (a))
#define M_LNEG(a)               emit_neg_reg(cd, (a))

#define M_IAND(a,b)             emit_alul_reg_reg(cd, ALU_AND, (a), (b))
#define M_IOR(a,b)              emit_alul_reg_reg(cd, ALU_OR, (a), (b))
#define M_IXOR(a,b)             emit_alul_reg_reg(cd, ALU_XOR, (a), (b))

#define M_IAND_IMM(a,b)         emit_alul_imm_reg(cd, ALU_AND, (a), (b))
#define M_IOR_IMM(a,b)          emit_alul_imm_reg(cd, ALU_OR, (a), (b))
#define M_IXOR_IMM(a,b)         emit_alul_imm_reg(cd, ALU_XOR, (a), (b))

#define M_LAND(a,b)             emit_alu_reg_reg(cd, ALU_AND, (a), (b))
#define M_LOR(a,b)              emit_alu_reg_reg(cd, ALU_OR, (a), (b))
#define M_LXOR(a,b)             emit_alu_reg_reg(cd, ALU_XOR, (a), (b))

#define M_LAND_IMM(a,b)         emit_alu_imm_reg(cd, ALU_AND, (a), (b))
#define M_LOR_IMM(a,b)          emit_alu_imm_reg(cd, ALU_OR, (a), (b))
#define M_LXOR_IMM(a,b)         emit_alu_imm_reg(cd, ALU_XOR, (a), (b))

#define M_BSEXT(a,b)            emit_movsbq_reg_reg(cd, (a), (b))
#define M_SSEXT(a,b)            emit_movswq_reg_reg(cd, (a), (b))
#define M_ISEXT(a,b)            emit_movslq_reg_reg(cd, (a), (b))

#define M_CZEXT(a,b)            emit_movzwq_reg_reg(cd, (a), (b))

#define M_ISLL_IMM(a,b)         emit_shiftl_imm_reg(cd, SHIFT_SHL, (a), (b))
#define M_ISRA_IMM(a,b)         emit_shiftl_imm_reg(cd, SHIFT_SAR, (a), (b))
#define M_ISRL_IMM(a,b)         emit_shiftl_imm_reg(cd, SHIFT_SHR, (a), (b))

#define M_LSLL_IMM(a,b)         emit_shift_imm_reg(cd, SHIFT_SHL, (a), (b))
#define M_LSRA_IMM(a,b)         emit_shift_imm_reg(cd, SHIFT_SAR, (a), (b))
#define M_LSRL_IMM(a,b)         emit_shift_imm_reg(cd, SHIFT_SHR, (a), (b))

#define M_TEST(a)               emit_test_reg_reg(cd, (a), (a))
#define M_ITEST(a)              emit_testl_reg_reg(cd, (a), (a))

#define M_LCMP(a,b)             emit_alu_reg_reg(cd, ALU_CMP, (a), (b))
#define M_LCMP_IMM(a,b)         emit_alu_imm_reg(cd, ALU_CMP, (a), (b))
#define M_LCMP_IMM_MEMBASE(a,b,c) emit_alu_imm_membase(cd, ALU_CMP, (a), (b), (c))
#define M_LCMP_MEMBASE(a,b,c)   emit_alu_membase_reg(cd, ALU_CMP, (a), (b), (c))

#define M_ICMP(a,b)             emit_alul_reg_reg(cd, ALU_CMP, (a), (b))
#define M_ICMP_IMM(a,b)         emit_alul_imm_reg(cd, ALU_CMP, (a), (b))
#define M_ICMP_IMM_MEMBASE(a,b,c) emit_alul_imm_membase(cd, ALU_CMP, (a), (b), (c))
#define M_ICMP_MEMBASE(a,b,c)   emit_alul_membase_reg(cd, ALU_CMP, (a), (b), (c))

#define M_BEQ(disp)             emit_jcc(cd, CC_E, (disp))
#define M_BNE(disp)             emit_jcc(cd, CC_NE, (disp))
#define M_BLT(disp)             emit_jcc(cd, CC_L, (disp))
#define M_BLE(disp)             emit_jcc(cd, CC_LE, (disp))
#define M_BGE(disp)             emit_jcc(cd, CC_GE, (disp))
#define M_BGT(disp)             emit_jcc(cd, CC_G, (disp))
#define M_BAE(disp)             emit_jcc(cd, CC_AE, (disp))
#define M_BA(disp)              emit_jcc(cd, CC_A, (disp))

#define M_CMOVEQ(a,b)           emit_cmovcc_reg_reg(cd, CC_E, (a), (b))
#define M_CMOVNE(a,b)           emit_cmovcc_reg_reg(cd, CC_NE, (a), (b))
#define M_CMOVLT(a,b)           emit_cmovcc_reg_reg(cd, CC_L, (a), (b))
#define M_CMOVLE(a,b)           emit_cmovcc_reg_reg(cd, CC_LE, (a), (b))
#define M_CMOVGE(a,b)           emit_cmovcc_reg_reg(cd, CC_GE, (a), (b))
#define M_CMOVGT(a,b)           emit_cmovcc_reg_reg(cd, CC_G, (a), (b))

#define M_CMOVEQ_MEMBASE(a,b,c) emit_cmovcc_reg_membase(cd, CC_E, (a), (b))
#define M_CMOVNE_MEMBASE(a,b,c) emit_cmovcc_reg_membase(cd, CC_NE, (a), (b))
#define M_CMOVLT_MEMBASE(a,b,c) emit_cmovcc_reg_membase(cd, CC_L, (a), (b))
#define M_CMOVLE_MEMBASE(a,b,c) emit_cmovcc_reg_membase(cd, CC_LE, (a), (b))
#define M_CMOVGE_MEMBASE(a,b,c) emit_cmovcc_reg_membase(cd, CC_GE, (a), (b))
#define M_CMOVGT_MEMBASE(a,b,c) emit_cmovcc_reg_membase(cd, CC_G, (a), (b))

#define M_CMOVB(a,b)            emit_cmovcc_reg_reg(cd, CC_B, (a), (b))
#define M_CMOVA(a,b)            emit_cmovcc_reg_reg(cd, CC_A, (a), (b))
#define M_CMOVP(a,b)            emit_cmovcc_reg_reg(cd, CC_P, (a), (b))

#define M_PUSH(a)               emit_push_reg(cd, (a))
#define M_PUSH_IMM(a)           emit_push_imm(cd, (a))
#define M_POP(a)                emit_pop_reg(cd, (a))

#define M_JMP(a)                emit_jmp_reg(cd, (a))
#define M_JMP_IMM(a)            emit_jmp_imm(cd, (a))
#define M__CALL(a)               emit_call_reg(cd, (a))
#define M_CALL_IMM(a)           emit_call_imm(cd, (a))
#define M_RET                   emit_ret(cd)

#define M_NOP                   emit_nop(cd)

#define M_CLR(a)                M_LXOR(a,a)


#define M__FLD(a,b,disp)         emit_movss_membase_reg(cd, (b), (disp), (a))
#define M__DLD(a,b,disp)         emit_movsd_membase_reg(cd, (b), (disp), (a))

#define M_FLD32(a,b,disp)       emit_movss_membase32_reg(cd, (b), (disp), (a))
#define M_DLD32(a,b,disp)       emit_movsd_membase32_reg(cd, (b), (disp), (a))

#define M_FST(a,b,disp)         emit_movss_reg_membase(cd, (a), (b), (disp))
#define M_DST(a,b,disp)         emit_movsd_reg_membase(cd, (a), (b), (disp))

#define M_FST32(a,b,disp)       emit_movss_reg_membase32(cd, (a), (b), (disp))
#define M_DST32(a,b,disp)       emit_movsd_reg_membase32(cd, (a), (b), (disp))

#define M_FADD(a,b)             emit_addss_reg_reg(cd, (a), (b))
#define M_DADD(a,b)             emit_addsd_reg_reg(cd, (a), (b))
#define M_FSUB(a,b)             emit_subss_reg_reg(cd, (a), (b))
#define M_DSUB(a,b)             emit_subsd_reg_reg(cd, (a), (b))
#define M_FMUL(a,b)             emit_mulss_reg_reg(cd, (a), (b))
#define M_DMUL(a,b)             emit_mulsd_reg_reg(cd, (a), (b))
#define M_FDIV(a,b)             emit_divss_reg_reg(cd, (a), (b))
#define M_DDIV(a,b)             emit_divsd_reg_reg(cd, (a), (b))

#define M_CVTIF(a,b)            emit_cvtsi2ss_reg_reg(cd, (a), (b))
#define M_CVTID(a,b)            emit_cvtsi2sd_reg_reg(cd, (a), (b))
#define M_CVTLF(a,b)            emit_cvtsi2ssq_reg_reg(cd, (a), (b))
#define M_CVTLD(a,b)            emit_cvtsi2sdq_reg_reg(cd, (a), (b))
#define M_CVTFI(a,b)            emit_cvttss2si_reg_reg(cd, (a), (b))
#define M_CVTDI(a,b)            emit_cvttsd2si_reg_reg(cd, (a), (b))
#define M_CVTFL(a,b)            emit_cvttss2siq_reg_reg(cd, (a), (b))
#define M_CVTDL(a,b)            emit_cvttsd2siq_reg_reg(cd, (a), (b))

#define M_CVTFD(a,b)            emit_cvtss2sd_reg_reg(cd, (a), (b))
#define M_CVTDF(a,b)            emit_cvtsd2ss_reg_reg(cd, (a), (b))


/* system instructions ********************************************************/

#define M_RDTSC                 emit_rdtsc(cd)

#define M_IINC_MEMBASE(a,b)     emit_incl_membase(cd, (a), (b))

#define M_IADD_MEMBASE(a,b,c)   emit_alul_reg_membase(cd, ALU_ADD, (a), (b), (c))
#define M_IADC_MEMBASE(a,b,c)   emit_alul_reg_membase(cd, ALU_ADC, (a), (b), (c))
#define M_ISUB_MEMBASE(a,b,c)   emit_alul_reg_membase(cd, ALU_SUB, (a), (b), (c))
#define M_ISBB_MEMBASE(a,b,c)   emit_alul_reg_membase(cd, ALU_SBB, (a), (b), (c))

#define PROFILE_CYCLE_START
#define __PROFILE_CYCLE_START \
    do { \
        if (JITDATA_HAS_FLAG_INSTRUMENT(jd)) { \
            M_PUSH(RAX); \
            M_PUSH(RDX); \
            \
            M_MOV_IMM(code, REG_ITMP3); \
            M_RDTSC; \
            M_ISUB_MEMBASE(RAX, REG_ITMP3, OFFSET(codeinfo, cycles)); \
            M_ISBB_MEMBASE(RDX, REG_ITMP3, OFFSET(codeinfo, cycles) + 4); \
            \
            M_POP(RDX); \
            M_POP(RAX); \
        } \
    } while (0)

#define PROFILE_CYCLE_STOP 
#define __PROFILE_CYCLE_STOP \
    do { \
        if (JITDATA_HAS_FLAG_INSTRUMENT(jd)) { \
            M_PUSH(RAX); \
            M_PUSH(RDX); \
            \
            M_MOV_IMM(code, REG_ITMP3); \
            M_RDTSC; \
            M_IADD_MEMBASE(RAX, REG_ITMP3, OFFSET(codeinfo, cycles)); \
            M_IADC_MEMBASE(RDX, REG_ITMP3, OFFSET(codeinfo, cycles) + 4); \
            \
            M_POP(RDX); \
            M_POP(RAX); \
        } \
    } while (0)


/* function gen_resolvebranch **************************************************

    backpatches a branch instruction

    parameters: ip ... pointer to instruction after branch (void*)
                so ... offset of instruction after branch  (s8)
                to ... offset of branch target             (s8)

*******************************************************************************/

#define gen_resolvebranch(ip,so,to) \
    *((s4*) ((ip) - 4)) = (s4) ((to) - (so));

#endif /* _CODEGEN_H */


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
