/* src/vm/jit/reg.h - register allocator header

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

   Changes: Christian Ullrich

   $Id: reg.h 4014 2005-12-30 14:20:25Z twisti $

*/


#ifndef _REG_H
#define _REG_H

/* #define INVOKE_NEW_DEBUG */


/* preliminary define for testing of the new creation of ARGVAR Stackslots in stack.c */
/* Changes affect handling of ARGVAR Stackslots in reg_of_var in codegen.inc          */
/* and calculation of rd->ifmemuse in reg.inc                                         */

/* We typedef these structures before #includes to resolve circular           */
/* dependencies.                                                              */

typedef struct varinfo varinfo;
typedef struct registerdata registerdata;


#include "config.h"
#include "vm/types.h"

#include "arch.h"

#include "vm/jit/codegen-common.h"
#include "vm/jit/jit.h"
#include "vm/jit/inline/inline.h"


/************************* pseudo variable structure **************************/

struct varinfo {
	int type;                   /* basic type of variable                     */
	int flags;                  /* flags (SAVED, INMEMORY)                    */
	int regoff;                 /* register number or memory offset           */
};

typedef struct varinfo varinfo5[5];


struct registerdata {
	varinfo5 *locals;
	varinfo5 *interfaces;


	int intreg_ret;                 /* register to return integer values      */
	int fltreg_ret;                 /* register for return float values       */

	int *argintregs;                /* argument integer registers             */
	int *tmpintregs;                /* scratch integer registers              */
	int *savintregs;                /* saved integer registers                */
	int *argfltregs;                /* argument float registers               */
	int *tmpfltregs;                /* scratch float registers                */
	int *savfltregs;                /* saved float registers                  */
	int *freeargintregs;            /* free argument integer registers        */
	int *freetmpintregs;            /* free scratch integer registers         */
	int *freesavintregs;            /* free saved integer registers           */
	int *freeargfltregs;            /* free argument float registers          */
	int *freetmpfltregs;            /* free scratch float registers           */
	int *freesavfltregs;            /* free saved float registers             */

#ifdef HAS_ADDRESS_REGISTER_FILE
	int adrreg_ret;                 /* register to return address values      */

	int *argadrregs;                /* argument address registers             */
	int *tmpadrregs;                /* scratch address registers              */
	int *savadrregs;                /* saved address registers                */
	int *freeargadrregs;            /* free argument address registers        */
	int *freetmpadrregs;            /* free scratch address registers         */
	int *freesavadrregs;            /* free saved address registers           */

	int argadrreguse;               /* used argument address register count   */
	int tmpadrreguse;               /* used scratch address register count    */
	int savadrreguse;               /* used saved address register count      */

	int freetmpadrtop;              /* free scratch address register count    */
	int freesavadrtop;              /* free saved address register count      */
	int freeargadrtop;              /* free argument address register count   */
#endif

#if defined(HAS_4BYTE_STACKSLOT)
	int *freemem_2;
	int freememtop_2;
#endif
	int *freemem;                   /* free scratch memory                    */
	int freememtop;                 /* free memory count                      */

	int memuse;                     /* used memory count                      */

	int argintreguse;               /* used argument integer register count   */
	int tmpintreguse;               /* used scratch integer register count    */
	int savintreguse;               /* used saved integer register count      */
	int argfltreguse;               /* used argument float register count     */
	int tmpfltreguse;               /* used scratch float register count      */
	int savfltreguse;               /* used saved float register count        */

	int freearginttop;              /* free argument integer register count   */
	int freeargflttop;              /* free argument float register count     */
	int freetmpinttop;              /* free scratch integer register count    */
	int freesavinttop;              /* free saved integer register count      */
	int freetmpflttop;              /* free scratch float register count      */
	int freesavflttop;              /* free saved float register count        */
};


/* function prototypes ********************************************************/

void reg_init(void);
void reg_setup(methodinfo *m, registerdata *rd, t_inlining_globals *id);
void reg_free(methodinfo *m, registerdata *rd);
void reg_close(void);
void regalloc(methodinfo *m, codegendata *cd, registerdata *rd);

#if defined(ENABLE_STATISTICS)
void reg_make_statistics( methodinfo *, codegendata *, registerdata *);
#endif

#endif /* _REG_H */


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
