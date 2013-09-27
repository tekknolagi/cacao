/* src/vm/jit/compiler2/MachineInstructionSchedulingPass.cpp - MachineInstructionSchedulingPass

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

#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"

#include "vm/jit/compiler2/ListSchedulingPass.hpp"
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/LoweringPass.hpp"
#include "vm/jit/compiler2/MachineBasicBlock.hpp"

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/MachineInstructionSchedulingPass"

namespace cacao {
namespace jit {
namespace compiler2 {

void MachineInstructionSchedulingPass::initialize() {
#if 0
	list.clear();
	map.clear();
#endif
}


namespace {

class UpdateCurrentVisitor : public MachineStubVisitor {
private:
	MachineBasicBlock *MBB;
public:
	UpdateCurrentVisitor(MachineBasicBlock *MBB) : MBB(MBB) {}

	using MachineStubVisitor::visit;

	virtual void visit(MachineJumpStub *MS) {
		MS->set_current(MBB);
	}
};

class UpdateTargetVisitor : public MachineStubVisitor {
private:
	typedef std::map<BeginInst*,MachineBasicBlock*> MapTy;
	struct LookupMap : public MachineJumpStub::LookupFn {
		MapTy &map;
		/// constructor
		LookupMap(MapTy &map) : map(map) {}
		/// lookup operator
		virtual MachineBasicBlock* operator()(BeginInst* BI) const {
			MapTy::iterator it = map.find(BI);
			assert(it != map.end());
			return it->second;
		}
	};

	MachineBasicBlock::iterator i;
	LookupMap lookup;
public:
	UpdateTargetVisitor(MachineBasicBlock::iterator i, MapTy &map) : i(i), lookup(map) {}

	using MachineStubVisitor::visit;

	virtual void visit(MachineLabelStub *MS) {
		MachineBasicBlock *MBB = lookup(MS->get_BeginInst());
		*i = MS->transform(MBB);
	}
	virtual void visit(MachineJumpStub *MS) {
		*i = MS->transform(lookup);
	}
};

} // end anonymous namespace

bool MachineInstructionSchedulingPass::run(JITData &JD) {
	BasicBlockSchedule *BS = get_Pass<BasicBlockSchedulingPass>();
	InstructionSchedule<Instruction> *IS = get_Pass<ListSchedulingPass>();
	LoweringPass *LP = get_Pass<LoweringPass>();

	// store BeginInst <=> MachineBasicBlock
	//std::map<BeginInst*,MBBIterator> map;
	std::map<BeginInst*,MachineBasicBlock*> map;

	// for all basic blocks
	for (BasicBlockSchedule::const_bb_iterator i = BS->bb_begin(),
			e = BS->bb_end(); i != e ; ++i) {
		BeginInst *BI = *i;
		assert(BI);
		// create MachineBasicBlock
		MachineBasicBlock *MBB = *push_back(MBBBuilder());
		//MBBIterator iMBB = push_front(MBBBuilder());
		//MachineBasicBlock *MBB = *iMBB;
		//map.insert(std::make_pair(BI,iMBB)); //map[BI] = iMBB;
		map[BI] = MBB;
		// for all instructions in the current basic block
		for (InstructionSchedule<Instruction>::const_inst_iterator i = IS->inst_begin(BI),
				e = IS->inst_end(BI); i != e; ++i) {
			Instruction *I = *i;
			const LoweredInstDAG *dag = LP->get_LoweredInstDAG(I);
			assert(dag);
			for (LoweredInstDAG::const_mi_iterator i = dag->mi_begin() ,
					e = dag->mi_end(); i != e ;) {
				MachineInstruction *MI = *i;
				MBB->push_back(MI);
				// start new basic block if there are several jump instructions
				++i;
				if (MI->is_stub()) {
					MachineInstStub *stub = MI->to_MachineInstStub();
					UpdateCurrentVisitor v(MBB);
					stub->accepts(v);
				}
				if (MI->is_jump() && i != e) {
					MBB = *push_back(MBBBuilder());
					MachineInstruction *label = new MachineLabelInst(MBB);
					MBB->push_back(label);
				}
			}
		}
	}
	// stubs!
	for (MachineInstructionSchedule::const_iterator i = begin(), e = end();
			i != e; ++i) {
		MachineBasicBlock *MBB = *i;
		for (MachineBasicBlock::iterator i = MBB->begin(), e = MBB->end();
				i != e ; ++i) {
			MachineInstruction *MI = *i;
			if (MI->is_stub()) {
				MachineInstStub *stub = MI->to_MachineInstStub();
				UpdateTargetVisitor v(i,map);
				stub->accepts(v);
			}
		}
	}
	return true;
}

// verify
bool MachineInstructionSchedulingPass::verify() const {
	for (MachineInstructionSchedule::const_iterator i = begin(), e = end();
			i != e; ++i) {
		MachineBasicBlock *MBB = *i;
		MachineInstruction *front = MBB->front();
		MachineInstruction *back = MBB->back();
		// check for label
		if(!front->is_label()) {
			ERROR_MSG("verification failed", "first Instruction ("
				<< *front << ") not a label");
			return false;
		}
		// check for end
		if(!back->is_end()) {
			ERROR_MSG("verification failed", "last Instruction ("
				<< *back << ") not a control flow transfer instruction");
			return false;
		}
		// check for stub
		for (MachineBasicBlock::const_iterator i = MBB->begin(), e = MBB->end();
				i != e ; ++i) {
			MachineInstruction *MI = *i;
			if(MI->is_stub()) {
				LOG(BoldRed << "error " << BoldWhite << "stub Instruction ("
					<< *MI << ") " << reset_color << nl);
				return false;
			}
		}
	}
	return true;
}

// pass usage
PassUsage& MachineInstructionSchedulingPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires(BasicBlockSchedulingPass::ID);
	PU.add_requires(ListSchedulingPass::ID);
	PU.add_requires(LoweringPass::ID);
	return PU;
}

// the address of this variable is used to identify the pass
char MachineInstructionSchedulingPass::ID = 0;

// register pass
static PassRegistery<MachineInstructionSchedulingPass> X("MachineInstructionSchedulingPass");

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
