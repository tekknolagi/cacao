/* src/vm/jit/parisc/arch.h - architecture defines for PA-RISC

   Copyright (C) 2006 R. Grafl, A. Krall, C. Kruegel,
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

   $Id: arch.h 5173 2006-07-25 15:57:11Z twisti $

*/


#ifndef _ARCH_H
#define _ARCH_H

/* define architecture features ***********************************************/

#define U8_AVAILABLE                     1

/*  #define USEBUILTINTABLE */

#define SUPPORT_DIVISION                 0
#define SUPPORT_LONG                     1
#define SUPPORT_FLOAT                    1
#define SUPPORT_DOUBLE                   1

#define SUPPORT_I2F                      1
#define SUPPORT_I2D                      1
#define SUPPORT_L2F                      1
#define SUPPORT_L2D                      1

#define SUPPORT_F2I                      0
#define SUPPORT_F2L                      0
#define SUPPORT_D2I                      0
#define SUPPORT_D2L                      0

#define SUPPORT_LONG_ADD                 1
#define SUPPORT_LONG_CMP                 1
#define SUPPORT_LONG_CMP_CONST           1
#define SUPPORT_LONG_LOGICAL             1
#define SUPPORT_LONG_SHIFT               1
#define SUPPORT_LONG_MUL                 1
#define SUPPORT_LONG_DIV                 0

#define SUPPORT_LONG_DIV_POW2            1
#define SUPPORT_LONG_REM_POW2            1

#define SUPPORT_CONST_LOGICAL            1  /* AND, OR, XOR with immediates   */
#define SUPPORT_CONST_MUL                1  /* mutiply with immediate         */

#define SUPPORT_CONST_STORE              1  /* do we support const stores     */
#define SUPPORT_CONST_STORE_ZERO_ONLY    1  /* on some risc machines we can   */
                                            /* only store REG_ZERO            */

#define USE_FAKE_ATOMIC_INSTRUCTIONS     1

#endif /* _ARCH_H */


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