/* src/vm/jit/powerpc/linux/md-abi.h - defines for PowerPC Linux ABI

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

   $Id: md-abi.h 2576 2005-06-06 21:21:19Z twisti $

*/


#ifndef _MD_ABI_H
#define _MD_ABI_H

/* preallocated registers *****************************************************/

/* integer registers */
  
#define REG_RESULT       3   /* to deliver method results                     */
#define REG_RESULT2      4   /* to deliver long method results                */

#define REG_PV          13   /* procedure vector, must be provided by caller  */
#define REG_METHODPTR   12   /* pointer to the place from where the procedure */
                             /* vector has been fetched                       */
#define REG_ITMP1       11   /* temporary register                            */
#define REG_ITMP2       12   /* temporary register and method pointer         */
#define REG_ITMP3        0   /* temporary register                            */

#define REG_ITMP1_XPTR  11   /* exception pointer = temporary register 1      */
#define REG_ITMP2_XPC   12   /* exception pc = temporary register 2           */

#define REG_SP           1   /* stack pointer                                 */
#define REG_ZERO         0   /* almost always zero: only in address calc.     */

/* floating point registers */

#define REG_FRESULT      1   /* to deliver floating point method results      */
#define REG_FTMP1       16   /* temporary floating point register             */
#define REG_FTMP2       17   /* temporary floating point register             */
#define REG_FTMP3        0   /* temporary floating point register             */

#define REG_IFTMP        0   /* temporary integer and floating point register */


#define INT_REG_CNT     32   /* number of integer registers                   */
#define INT_SAV_CNT     10   /* number of int callee saved registers          */
#define INT_ARG_CNT      8   /* number of int argument registers              */
#define INT_TMP_CNT      8   /* number of integer temporary registers         */
#define INT_RES_CNT      3   /* number of integer reserved registers          */

#define FLT_REG_CNT     32   /* number of float registers                     */
#define FLT_SAV_CNT     10   /* number of float callee saved registers        */
#define FLT_ARG_CNT      8   /* number of float argument registers            */
#define FLT_TMP_CNT     11   /* number of float temporary registers           */
#define FLT_RES_CNT      3   /* number of float reserved registers            */

#define TRACE_ARGS_NUM   8


/* ABI defines ****************************************************************/

#define LA_SIZE          8   /* linkage area size                             */
#define LA_SIZE_ALIGNED 16   /* linkage area size aligned to 16-byte          */
#define LA_WORD_SIZE     2   /* linkage area size in words: 2 * 4 = 8         */

#define LA_LR_OFFSET     4   /* link register offset in linkage area          */

/* #define ALIGN_FRAME_SIZE(sp)       (sp) */

#endif /* _MD_ABI_H */


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
