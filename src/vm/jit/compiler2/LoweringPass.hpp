/* src/vm/jit/compiler2/LoweringPass.hpp - LoweringPass

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

#ifndef _JIT_COMPILER2_LOWERINGPASS
#define _JIT_COMPILER2_LOWERINGPASS

#include "vm/jit/compiler2/Pass.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {

class Instruction;
class LoweredInstDAG;

/**
 * LoweringPass
 * TODO: more info
 */
class LoweringPass : public Pass {
private:
	typedef std::map<Instruction*,LoweredInstDAG*> LoweringMapTy;
	LoweringMapTy lowering_map;
public:
	static char ID;
	LoweringPass() : Pass() {}
	bool run(JITData &JD);
	PassUsage& get_PassUsage(PassUsage &PA) const;
	virtual bool verify() const;
	LoweredInstDAG* get_LoweredInstDAG(Instruction *I) const {
		LoweringMapTy::const_iterator i = lowering_map.find(I);
		if (i == lowering_map.end()) {
			return NULL;
		}
		return i->second;
	}
};

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LOWERINGPASS */


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
