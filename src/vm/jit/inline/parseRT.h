/* jit/parseRT.h - RTA parser header

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

   Authors: Carolyn Oates

   $Id: parseRT.h 1735 2004-12-07 14:33:27Z twisti $

*/

#ifndef _PARSERT_H
#define _PARSERT_H

#include "vm/global.h"


extern FILE *rtMissed;   /* Methods missed during RTA parse of Main  */

typedef struct {
        listnode linkage;
        methodinfo *method;
        } rtaNode ;


extern int RT_jit_parse(methodinfo *m);

#define METHINFOx(mm) \
    { \
	printf("<c%i/m%i/p%i>\t", \
		mm->class->classUsed,mm->methodUsed, mm->monoPoly); \
  	method_display_w_class(mm); }

#define METHINFO(mm,flg) \
if (flg) { \
	printf("<c%i/m%i/p%i>\t", \
		mm->class->classUsed,mm->methodUsed, mm->monoPoly); \
  	method_display_w_class(mm); }

#define METHINFOtx(mm,TXT) \
		 { \
                printf(TXT); \
		METHINFOx(mm) \
		}

#define METHINFOt(mm,TXT,flg) \
if (flg) { \
                printf(TXT); \
		METHINFO(mm,flg) \
		}

#define CLASSNAME1(cls,TXT,flg) \
if (flg) {printf(TXT); \
	printf("<c%i>\t",cls->classUsed); \
	utf_display(cls->name); fflush(stdout);}

#define CLASSNAMEop(cls,flg) \
if (flg) {printf("\t%s: ",opcode_names[opcode]);\
	printf("<c%i>\t",cls->classUsed); \
  	utf_display(cls->name); printf("\n");fflush(stdout);}

#define CLASSNAME(cls,TXT,flg) \
if (flg) { printf(TXT); \
		printf("<c%i>\t",cls->classUsed); \
  		utf_display(cls->name); printf("\n");fflush(stdout);} 

#define SHOWOPCODE \
if (DEBUGopcodes== true) {printf("Parse p=%i<%i<   opcode=<%i> %s\n", \
	                   p, m->jcodelength,opcode,opcode_names[opcode]);}



#endif /* _PARSERT_H */

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

