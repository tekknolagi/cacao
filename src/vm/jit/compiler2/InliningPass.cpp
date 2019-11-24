/* src/vm/jit/compiler2/InliningPass.cpp - InliningPass

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

#include <algorithm>
#include <cassert>
#include <iostream>

#include "vm/jit/compiler2/InliningPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/MethodC2.hpp"
#include "vm/jit/compiler2/Instruction.hpp"
#include "vm/jit/compiler2/Instructions.hpp"
#include "vm/jit/compiler2/SSAConstructionPass.hpp"
#include "vm/jit/compiler2/SourceStateAttachmentPass.hpp"
#include "vm/jit/compiler2/alloc/queue.hpp"
#include "vm/jit/jit.hpp"

#include "toolbox/logging.hpp"

// define name for debugging (see logging.hpp)
#define DEBUG_NAME "compiler2/InliningPass"

namespace cacao {
namespace jit {
namespace compiler2 {

bool InliningPass::run(JITData &JD) {
    log_start();
    log_println("Start of inlining pass.");
    Method* M = JD.get_Method();
    log_message_utf("Inlining for class: ", M->get_class_name_utf8());
    log_message_utf("Inlining for method: ", M->get_name_utf8());
    log_println("begin(): %p", M->begin());
    log_println("end(): %p", M->end());

	for (auto it = M->begin(); it != M->end(); it++) {
		Instruction *I = *it;
		if (can_inline(I)) {
            inline_instruction(I);
		}
	}

    return true;
}

bool InliningPass::can_inline(Instruction* I){
    return I->get_opcode() == Instruction::INVOKESTATICInstID;
}

void InliningPass::inline_instruction(Instruction* I){
    log_println("Inlining instruction (%p)", I);
    switch (I->get_opcode())
    {
        case Instruction::INVOKESTATICInstID:
            inline_invoke_static_instruction(I->to_INVOKESTATICInst());
            break;
        default:
            break;
    }
}

void InliningPass::inline_invoke_static_instruction(INVOKESTATICInst* I){
    log_println("Inlining static invoke instruction (%p)", I);
    methodinfo* callee = I->get_fmiref()->p.method;
    jitdata *jd = jit_jitdata_new(callee);
	jit_jitdata_init_for_recompilation(jd);
    JITData JD(jd);
    
    PassRunner runner;
	runner.runPasses(JD);
    Method* callee_method = JD.get_Method();
}

// pass usage
PassUsage& InliningPass::get_PassUsage(PassUsage &PU) const {
    PU.requires<HIRInstructionsArtifact>();
	return PU;
}

// register pass
static PassRegistry<InliningPass> X("InliningPass");

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
