/* src/vm/jit/mips/linux/md-os.c - machine dependent MIPS Linux functions

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
            Reinhard Grafl

   Changes: Christian Thalinger

   $Id: md-os.c 4357 2006-01-22 23:33:38Z twisti $

*/


#include "config.h"

#include <assert.h>
#include <signal.h>
#include <ucontext.h>

#include "vm/types.h"

#include "vm/jit/mips/md-abi.h"

#include "mm/boehm.h"
#include "vm/exceptions.h"
#include "vm/signallocal.h"
#include "vm/stringlocal.h"
#include "vm/jit/asmpart.h"
#include "vm/jit/stacktrace.h"


/* md_init *********************************************************************

   Do some machine dependent initialization.

*******************************************************************************/

void md_init(void)
{
	/* The Boehm GC initialization blocks the SIGSEGV signal. So we do a      */
	/* dummy allocation here to ensure that the GC is initialized.            */

	heap_allocate(1, 0, NULL);

#if 0
	/* Turn off flush-to-zero */

	{
		union fpc_csr n;
		n.fc_word = get_fpc_csr();
		n.fc_struct.flush = 0;
		set_fpc_csr(n.fc_word);
	}
#endif
}


/* md_signal_handler_sigsegv ***************************************************

   NullPointerException signal handler for hardware null pointer
   check.

*******************************************************************************/

void md_signal_handler_sigsegv(int sig, siginfo_t *siginfo, void *_p)
{
	ucontext_t  *_uc;
	mcontext_t  *_mc;
	u4           instr;
	ptrint       addr;
	u1          *pv;
	u1          *sp;
	u1          *ra;
	u1          *xpc;

	_uc = (struct ucontext *) _p;
	_mc = &_uc->uc_mcontext;

	/* in ucontext.h the registers are defined as long long, even for
	   MIPS32, so we cast them */
	
	instr = *((u4 *) ((ptrint) _mc->pc));
	addr = _mc->gregs[(instr >> 21) & 0x1f];

	if (addr == 0) {
		pv  = (u1 *) (ptrint) _mc->gregs[REG_PV];
		sp  = (u1 *) (ptrint) _mc->gregs[REG_SP];
		ra  = (u1 *) (ptrint) _mc->gregs[REG_RA]; /* this is correct for leafs*/
		xpc = (u1 *) (ptrint) _mc->pc;

		_mc->gregs[REG_ITMP1_XPTR] =
			(ptrint) stacktrace_hardware_nullpointerexception(pv, sp, ra, xpc);

		_mc->gregs[REG_ITMP2_XPC] = (ptrint) xpc;
		_mc->pc = (ptrint) asm_handle_exception;

	} else {
		addr += (long) ((instr << 16) >> 16);

		throw_cacao_exception_exit(string_java_lang_InternalError,
								   "faulting address: 0x%lx at 0x%lx\n",
								   addr, _mc->pc);
	}
}


#if defined(USE_THREADS) && defined(NATIVE_THREADS)
void thread_restartcriticalsection(ucontext_t *_uc)
{
	mcontext_t *_mc;
	void *critical;

	_mc = &_uc->uc_mcontext;

	critical = thread_checkcritical((void *) (ptrint) _mc->pc);

	if (critical)
		_mc->pc = (ptrint) critical;
}
#endif


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
