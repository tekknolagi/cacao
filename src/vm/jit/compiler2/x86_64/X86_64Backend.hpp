/* src/vm/jit/compiler2/X86_64Backend.hpp - X86_64Backend

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

#ifndef _JIT_COMPILER2_X86_64BACKEND
#define _JIT_COMPILER2_X86_64BACKEND

#include "vm/jit/compiler2/x86_64/X86_64.hpp"
#include "vm/jit/compiler2/Backend.hpp"

namespace cacao {
namespace jit {
namespace compiler2 {
namespace x86_64 {


class X86_64LoweringVisitor : public LoweringVisitorBase {
public:
	X86_64LoweringVisitor(Backend *backend, MachineBasicBlock* current,
		MapTy &map, InstructionMapTy &inst_map, MachineInstructionSchedule *schedule)
			: LoweringVisitorBase(backend, current, map, inst_map, schedule) {}

	// make LoweringVisitorBases visit visible
	using LoweringVisitorBase::visit;

	virtual void visit(LOADInst *I, bool copyOperands);
	virtual void visit(IFInst *I, bool copyOperands);
	virtual void visit(ADDInst *I, bool copyOperands);
	virtual void visit(ANDInst *I, bool copyOperands);
	virtual void visit(SUBInst *I, bool copyOperands);
	virtual void visit(MULInst *I, bool copyOperands);
	virtual void visit(DIVInst *I, bool copyOperands);
	virtual void visit(REMInst *I, bool copyOperands);
	virtual void visit(RETURNInst *I, bool copyOperands);
	virtual void visit(CASTInst *I, bool copyOperands);
	virtual void visit(INVOKESTATICInst *I, bool copyOperands);
	virtual void visit(GETSTATICInst *I, bool copyOperands);
	virtual void visit(LOOKUPSWITCHInst *I, bool copyOperands);
	virtual void visit(TABLESWITCHInst *I, bool copyOperands);
	virtual void visit(ARRAYLENGTHInst *I, bool copyOperands);
	virtual void visit(ALOADInst *I, bool copyOperands);
	virtual void visit(ASTOREInst *I, bool copyOperands);
	virtual void visit(ARRAYBOUNDSCHECKInst *I, bool copyOperands);
	virtual void visit(CMPInst *I, bool copyOperands);
	virtual void visit(NEGInst *I, bool copyOperands);

	virtual void lowerComplex(Instruction* I, int ruleId);

private:
	void setupSrcDst(MachineOperand*& src_op1, MachineOperand*& src_op2, VirtualRegister*& dst, 
		Type::TypeID type, bool copyOperands, bool isCommutable);

};

} // end namespace x86_64
} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_X86_64BACKEND */


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
