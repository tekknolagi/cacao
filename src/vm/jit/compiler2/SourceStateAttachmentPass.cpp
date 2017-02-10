/* src/vm/jit/compiler2/SourceStateAttachmentPass.cpp - SourceStateAttachmentPass

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

#include "vm/jit/compiler2/SourceStateAttachmentPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/MethodC2.hpp"
#include "vm/jit/compiler2/InstructionMetaPass.hpp"
#include "vm/jit/compiler2/ScheduleClickPass.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/GlobalSchedule.hpp"
#include "toolbox/logging.hpp"

// define name for debugging (see logging.hpp)
#define DEBUG_NAME "compiler2/SourceStateAttachmentPass"

namespace cacao {
namespace jit {
namespace compiler2 {

namespace {

SourceStateInst *find_associated_source_state(Instruction *I) {
	Instruction::const_dep_iterator i = I->rdep_begin();
	Instruction::const_dep_iterator e = I->rdep_end();

	for (; i != e; i++) {
		SourceStateInst *source_state = (*i)->to_SourceStateInst();
		if (source_state) {
			return source_state;
		}
	}

	return NULL;
}

} // end anonymous namespace

/**
 * TODO more info
 */
SourceStateInst *SourceStateAttachmentPass::find_nearest_dominating_source_state(BeginInst *block) {
	BeginInst *current_block = block;
	SourceStateInst *source_state = NULL;

	while (current_block != NULL) {
		// find the last side-effecting node in this block
		Instruction *last_side_effect = NULL;

		if (last_side_effect) {
			// use the SourceStateInst of the last side-effecting node
			return find_associated_source_state(last_side_effect);
		} else if (current_block->pred_size() > 1 || current_block == M->get_init_bb()) {
			// when there is no side-effecting node within this block then let's
			// look if the current block's BeginInst has an associated
			// SourceStateInst, which is the case iff it is a control-flow merge
			// (i.e. it has more than one predecessors)
			return find_associated_source_state(current_block);
		} else {
			// when there is no side-effecting node within this block and
			// the current block is no control-flow merge, we have to expand our
			// search of a SourceStateInst to the preceding blocks
			current_block = current_block->get_predecessor(0);
		}
	}

	assert(source_state);
	return source_state;
}

/**
 * TODO more info
 */
void SourceStateAttachmentPass::attach_source_state(DeoptInst *deopt) {
	assert(deopt);

	Instruction *guarded_inst = deopt->get_guarded_inst();;
	assert(guarded_inst->has_side_effects() || guarded_inst->to_BeginInst());

	// get the blocks that contain the DeoptInst and its guarded instruction
	BeginInst *deopt_block = schedule->get(deopt);
	BeginInst *guarded_block = schedule->get(guarded_inst);

	SourceStateInst *source_state;
	if (deopt_block == guarded_block) {
		Instruction::const_dep_iterator i = guarded_inst->dep_begin();
		Instruction::const_dep_iterator e = guarded_inst->dep_end();

		// find the preceding side-effecting node in the same block
		Instruction *preceding_side_effect = NULL;
		for (; i != e; i++) {
			Instruction *I = *i;
			if (!I->to_BeginInst() && I->has_side_effects()) {
				preceding_side_effect = I;
			}
		}

		if (preceding_side_effect) {
			// use the SourceStateInst of the preceding side-effecting node
			source_state = find_associated_source_state(preceding_side_effect);
		} else if (deopt_block->pred_size() > 1 || deopt_block == M->get_init_bb()) {
			// when there is no side-effecting node within this block that
			// precedes the guarded instruction then let's look if the current
			// block's BeginInst has an associated SourceStateInst, which is the
			// case iff it is a control-flow merge (i.e. it has more than one
			// predecessors)
			source_state = find_associated_source_state(deopt_block);
		} else {
			// when there is no side-effecting node preceding the guarded
			// instruction and the current block's BeginInst is no control-flow
			// merge, we have to expand our search of a SourceStateInst to the
			// preceding blocks
			assert(deopt_block->pred_size() == 1);
			BeginInst *pred = deopt_block->get_predecessor(0);
			source_state = find_nearest_dominating_source_state(pred);
		}
	} else {
		// when the DeoptInst has been scheduled to another block than the node
		// that it guards, we immediately search for an appropriate
		// SourceStateInst within the dominating blocks of the DeoptInst
		source_state = find_nearest_dominating_source_state(deopt_block);
	}

	assert(source_state);
	LOG("attach " << source_state << " to " << deopt << nl);
	deopt->set_source_state(source_state);
}

bool SourceStateAttachmentPass::run(JITData &JD) {
	M = JD.get_Method();
	schedule = get_Pass<ScheduleClickPass>();
	
	for (Method::const_iterator i = M->begin(), e = M->end(); i != e; ++i) {
		Instruction *I = *i;
		if (I->to_DeoptInst()) {
			attach_source_state(I->to_DeoptInst());
		}
	}
	return true;
}

// pass usage
PassUsage& SourceStateAttachmentPass::get_PassUsage(PassUsage &PU) const {
	PU.add_requires<InstructionMetaPass>();
	PU.add_requires<ScheduleClickPass>();
	PU.add_schedule_before<MachineInstructionSchedulingPass>();
//	PU.add_schedule_before<DeadCodeEliminationPass>();
	return PU;
}

// the address of this variable is used to identify the pass
char SourceStateAttachmentPass::ID = 0;

// register pass
#if defined(ENABLE_REPLACEMENT)
static PassRegistry<SourceStateAttachmentPass> X("SourceStateAttachmentPass");
#endif

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