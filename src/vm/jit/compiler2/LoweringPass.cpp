/* src/vm/jit/compiler2/LoweringPass.cpp - LoweringPass

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

#include "vm/jit/compiler2/LoweringPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/LoweredInstDAG.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/lowering"

namespace cacao {
namespace jit {
namespace compiler2 {


bool LoweringPass::run(JITData &JD) {
	Method *M = JD.get_Method();
	Backend *BE = JD.get_Backend();

	LOG("Lowering to target: " << BE->get_name() << nl);

	for(Method::const_iterator i = M->begin(), e = M->end(); i != e ; ++i ) {
		Instruction *I = *i;
		LoweredInstDAG* dag = BE->lower(I);
		if (!dag)
			return false;
		lowering_map[I] = dag;
	}
	// fix unresolved registers
	for(Method::const_iterator i = M->begin(), e = M->end(); i != e ; ++i ) {
		Instruction *I = *i;
		LoweredInstDAG* dag = lowering_map[I];
		assert(dag);
		for(unsigned i = 0, e = dag->input_size(); i < e; ++i) {
			LoweredInstDAG::InputParameterTy para = (*dag)[i];
			Register* reg = (*para.first)[para.second]->to_Register();
			if (reg && reg->to_UnassignedReg()) {
				// if operand is unassigned
				Instruction *op = I->get_operand(i)->to_Instruction();
				assert(op);
				LoweredInstDAG *op_dag = lowering_map[op];
				assert(op_dag);
				MachineOperand *m_op = op_dag->get_result()->get_result();
				assert(m_op);
				(*para.first)[para.second] = m_op;
			}
		}
	}


	return true;
}

// pass usage
PassUsage& LoweringPass::get_PassUsage(PassUsage &PU) const {
	//PU.add_requires(YyyPass::ID);
	return PU;
}

bool LoweringPass::verify() const {
	for(LoweringMapTy::const_iterator i = lowering_map.begin(),
			e = lowering_map.end(); i != e; ++i) {
		LoweredInstDAG *dag = i->second;
		for(unsigned i = 0, e = dag->input_map.size(); i < e ; ++i) {
			MachineInstruction *MI = dag->input_map[i].first;
			if (!MI) {
				LOG("LoweredInstDAG input " << i << " not set! "
				    "(lowered " << dag->get_Instruction() << ")" << nl);
				return false;
			}
		}
	}
	return true;
}

// the address of this variable is used to identify the pass
char LoweringPass::ID = 0;

// register pass
static PassRegistery<LoweringPass> X("LoweringPass");

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
