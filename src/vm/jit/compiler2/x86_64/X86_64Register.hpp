/* src/vm/jit/compiler2/X86_64Register.hpp - X86_64Register

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

#ifndef _JIT_COMPILER2_X86_64REGISTER
#define _JIT_COMPILER2_X86_64REGISTER

#include "vm/jit/compiler2/x86_64/X86_64.hpp"
#include "vm/jit/compiler2/MachineRegister.hpp"
#include "vm/jit/compiler2/RegisterFile.hpp"

#include <vector>

namespace cacao {
namespace jit {
namespace compiler2 {

/**
 * x86_64 Register
 */

class X86_64Register : public MachineRegister {
public:
	X86_64Register(const char* name) : MachineRegister(name) {}
};

extern X86_64Register RAX;
extern X86_64Register RCX;
extern X86_64Register RDX;
extern X86_64Register RBX;
extern X86_64Register RSP;
extern X86_64Register RBP;
extern X86_64Register RSI;
extern X86_64Register RDI;
extern X86_64Register R8;
extern X86_64Register R9;
extern X86_64Register R10;
extern X86_64Register R11;
extern X86_64Register R12;
extern X86_64Register R13;
extern X86_64Register R14;
extern X86_64Register R15;

const unsigned X86_64IntegerArgumentRegisterSize = 6;
extern X86_64Register* X86_64IntegerArgumentRegisters[];


class X86_64RegisterFile : public RegisterFile {
private:
	X86_64RegisterFile() {
		for(unsigned i = 0; i < X86_64IntegerArgumentRegisterSize ; ++i) {
			regs.push_back(X86_64IntegerArgumentRegisters[i]);
		}
	}
public:
	static RegisterFile* factory() {
		static X86_64RegisterFile rf;
		return &rf;
	}
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_X86_64REGISTER */


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
