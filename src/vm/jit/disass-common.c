/* src/vm/jit/disass-common.c - common functions for GNU binutils disassembler

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

   Authors: Christian Thalinger

   Changes:

   $Id: disass-common.c 4109 2006-01-09 16:30:12Z twisti $

*/


#include "config.h"

#if defined(WITH_BINUTILS_DISASSEMBLER)
# include <dis-asm.h>
#endif

#include <stdarg.h>
#include <stdio.h>

#include "vm/types.h"

#include "mm/memory.h"


/* We need this on i386 and x86_64 since we don't know the byte length
   of currently printed instructions.  512 bytes should be enough. */

#if defined(__I386__) || defined(__X86_64__)
char disass_buf[512];
s4   disass_len;
#endif


/* disass_printf ***************************************************************

   Required by binutils disassembler.  This just prints the
   disassembled instructions to stdout.

*******************************************************************************/

#if defined(WITH_BINUTILS_DISASSEMBLER)
void disass_printf(PTR p, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

#if defined(__I386__) || defined(__X86_64__)
	vprintf(fmt, ap);
	fflush(stdout);
#else
	disass_len += vsprintf(disass_buf + disass_len, fmt, ap);
#endif

	va_end(ap);
}


/* buffer_read_memory **********************************************************

   We need to replace the buffer_read_memory from binutils.

*******************************************************************************/

int buffer_read_memory(bfd_vma memaddr, bfd_byte *myaddr, unsigned int length, struct disassemble_info *info)
{
	MCOPY(myaddr, (void *) memaddr, u1, length);

	return 0;
}
#endif /* defined(WITH_BINUTILS_DISASSEMBLER) */


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
