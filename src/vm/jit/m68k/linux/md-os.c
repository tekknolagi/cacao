/* src/vm/jit/m68k/linux/md-os.c - linux specific functions

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
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

   $Id: arch.h 5330 2006-09-05 18:43:12Z edwin $

*/

#include "config.h"

#include "md-os.h"
#include "md-abi.h"

#include "vm/vm.h"
#include "vm/exceptions.h"
#include "vm/jit/asmpart.h"

#include <assert.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include <sys/ucontext.h> /* has br0ken ucontext_t*/

/*
 *  The glibc bundeled with my devels system ucontext.h differs with kernels ucontext.h. Not good.
 *  The following stuff is taken from 2.6.10 + freescale coldfire patches:
 */
typedef struct actual_fpregset {
	int f_fpcntl[3];
	int f_fpregs[8*3];
} actual_fpregset_t;

typedef struct actual_mcontext {
	int version;
	gregset_t gregs;	/* 0...7 = %d0-%d7, 8...15 = %a0-%a7 */
	actual_fpregset_t fpregs;
} actual_mcontext_t;

typedef struct actual_ucontext {
	unsigned long     uc_flags;
	struct actual_ucontext  *uc_link;
	stack_t           uc_stack;
	struct actual_mcontext   uc_mcontext;
	unsigned long     uc_filler[80];
	sigset_t          uc_sigmask;   /* mask last for extensibility */
} actual_ucontext_t;


/*
 *	linux specific initializations
 */
void md_init_linux()
{
	struct sigaction act;
	
	act.sa_sigaction = md_signal_handler_sigill;
	act.sa_flags     = SA_NODEFER | SA_SIGINFO;

	if (sigaction(SIGILL, &act, NULL) == -1)	{
		vm_abort("md_linux_init: Error registering signal handler");
	}
}

/* md_signal_handler_sigsegv ******************************************
 *
 * BLA BLA
 */
void md_signal_handler_sigsegv(/*int sig, siginfo_t *siginfo, void *_p*/) 
{ 	
	assert(0); 
}

/*
 * This handler is used to generate hardware exceptions.
 * Type of exception derived from trap number.
 * If an object is needed a tst instruction (2 byte long) has
 * been created directly before the trap instruction (2 bytes long).
 * the last 3 bit of this tst instruction contain the register number.
 */
void md_signal_handler_sigill(int sig, siginfo_t *siginfo, actual_ucontext_t *_uc) 
{
	uint32_t	xpc, sp;
	uint16_t	opc;
	uint32_t	type;
	uint32_t	val, regval;
	java_objectheader *e;
	mcontext_t 	*_mc;

	if (siginfo->si_code != ILL_ILLTRP)	{
		vm_abort("md_signal_handler_sigill: Caught something not a trap");
	}

	xpc = siginfo->si_addr;
	opc = *(uint16_t*)(xpc-2);

	if (opc == 0x4afc)	{
		vm_abort("md_signal_handler_sigill: the illegal instruction @ 0x%x, aborting", xpc);
	}

	assert( (opc&0xfff0) == 0x4e40 );
	type = opc & 0x000f;		/* filter trap number */

	_mc = &_uc->uc_mcontext;
	sp = _mc->gregs[R_SP];

	/* Figure out in which register the object causing the exception resides for exceptions
	 * appropiate.
	 */
	switch (type)	{
		case EXCEPTION_HARDWARE_CLASSCAST: 
			regval = *(uint16_t*)(xpc-4);
			assert( (regval&0xfff0) == 0x4a00 );
			/* was in a address register */
			regval = _mc->gregs[ 8 + (regval & 0x7) ];
			break;
		default: assert(0);
	}

	fprintf(stderr, "NEW HWE: sp=%x, xpc=%x, tpye=%x, regval=%x\n", sp, xpc, type, regval);
	e = exceptions_new_hardware_exception(0, sp, xpc, xpc, type, regval);

	_mc->gregs[REG_ATMP1]     = (ptrint) e;
	_mc->gregs[REG_ATMP2_XPC] = (ptrint) xpc;
	_mc->gregs[R_PC]          = (ptrint) asm_handle_exception;
}