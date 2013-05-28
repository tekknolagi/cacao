/* src/vm/jit/compiler2/X86_64Register.cpp - X86_64Register

   Copyright (C) 2013
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

#include "vm/jit/compiler2/x86_64/X86_64Register.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

X86_64Register RAX("RAX");
X86_64Register RCX("RCX");
X86_64Register RDX("RDX");
X86_64Register RBX("RBX");
X86_64Register RSP("RSP");
X86_64Register RBP("RBP");
X86_64Register RSI("RSI");
X86_64Register RDI("RDI");
X86_64Register R8("R8");
X86_64Register R9("R9");
X86_64Register R10("R10");
X86_64Register R11("R11");
X86_64Register R12("R12");
X86_64Register R13("R13");
X86_64Register R14("R14");
X86_64Register R15("R15");


X86_64Register* X86_64IntegerArgumentRegisters[] = {
&RDI, &RSI, &RDX, &RCX, &R8, &R9
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */

