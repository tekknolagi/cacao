/* src/vm/jit/compiler2/LinearScanAllocatorPass.cpp - LinearScanAllocatorPass

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

#include "vm/jit/compiler2/LinearScanAllocatorPass.hpp"
#include "vm/jit/compiler2/PassManager.hpp"
#include "vm/jit/compiler2/JITData.hpp"
#include "vm/jit/compiler2/PassUsage.hpp"
#include "vm/jit/compiler2/LivetimeAnalysisPass.hpp"
#include "vm/jit/compiler2/MachineRegister.hpp"
#include "vm/jit/compiler2/MachineInstructionSchedulingPass.hpp"
#include "vm/jit/compiler2/MachineInstructions.hpp"
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/statistics.hpp"
#include "vm/options.hpp"

#include <climits>

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/LinearScanAllocator"

STAT_REGISTER_GROUP(lsra_stat,"LSRA","Linear Scan Register Allocator")
STAT_REGISTER_GROUP_VAR(std::size_t,num_hints_followed,0,"hints followed",
	"Number of Hints followed (i.e. no move)",lsra_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_hints_not_followed,0,"hints not followed",
	"Number of Hints not followed (i.e. move needed)",lsra_stat)
STAT_REGISTER_GROUP_VAR_EXTERN(std::size_t,num_remaining_moves,0,"remaining moves","Number of remaining moves",lsra_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_split_moves,0,"spill moves","Number of split moves",lsra_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_spill_loads,0,"spill loads","Number of spill loads",lsra_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_spill_stores,0,"spill stores","Number of spill stores",lsra_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_resolution_moves,0,"resolution moves","Number of move instructions for resolution",lsra_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_allocate_free,0,"allocate free","Number of free register allocated",lsra_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_allocate_blocked,0,"allocate blocked","Number of blocked register allocated (spill)",lsra_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_fixed_intervals,0,"fixed intervals","Number of fixed (preallocated) intervals",lsra_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_spill_blocked_fixed,0,"spilled blocked","Number blocked fixed intervals spilled",lsra_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_resolution_regs,0,"resolution regs","Number registers allocated for resolution (cycles/stack-to-stack moves)",lsra_stat)
STAT_REGISTER_GROUP_VAR(std::size_t,num_resolution_stacks,0,"resolution stacks","Number stackslots allocated for resolution (no free registers)",lsra_stat)

namespace cacao {
namespace jit {
namespace compiler2 {

namespace option {
	Option<bool> hints("Compiler2hints","compiler2: enable register hints",true,::cacao::option::xx_root());
}

bool LinearScanAllocatorPass::StartComparator::operator()(const LivetimeInterval &lhs,
		const LivetimeInterval &rhs) {
	if (rhs.front().start < lhs.front().start) return true;
	if (lhs.front().start < rhs.front().start) return false;
	// rhs.front().start == lhs.front().start;
	return lhs.back().end < rhs.back().end;
}

namespace {
/**
 * @Cpp11 use std::function
 */
struct FreeUntilCompare: public std::binary_function<MachineOperand*,MachineOperand*,bool> {
	bool operator()(MachineOperand *lhs, MachineOperand *rhs) const {
		bool r = lhs->aquivalence_less(*rhs);
		//LOG2("FreeUntilCompare: " << *lhs << " aquivalence_less " << *rhs << "=" << r << nl);
		return r;
	}
};

typedef alloc::map<MachineOperand*,UseDef,FreeUntilCompare>::type FreeUntilMap;

/**
 * @Cpp11 use std::function
 */
struct InitFreeUntilMap: public std::unary_function<MachineOperand*,void> {
	FreeUntilMap &free_until_pos;
	UseDef end;
	/// Constructor
	InitFreeUntilMap(FreeUntilMap &free_until_pos,
		UseDef end) : free_until_pos(free_until_pos), end(end) {}

	void operator()(MachineOperand *MO) {
		free_until_pos.insert(std::make_pair(MO,end));
	}
};


/**
 * @Cpp11 use std::function
 */
struct SetActive: public std::unary_function<LivetimeInterval&,void> {
	FreeUntilMap &free_until_pos;
	UseDef start;
	/// Constructor
	SetActive(FreeUntilMap &free_until_pos,
		UseDef start) : free_until_pos(free_until_pos), start(start) {}

	void operator()(LivetimeInterval& lti) {
		MachineOperand *MO = lti.get_operand();
		//LOG2("SetActive: " << lti << " operand: " << *MO << nl);
		FreeUntilMap::iterator i = free_until_pos.find(MO);
		if (i != free_until_pos.end()) {
			//LOG2("set to zero" << nl);
			LOG2("SetActive: " << lti << " operand: " << *MO << " to " << start << nl);
			i->second = start;
		}
	}
};


/**
 * @Cpp11 use std::function
 */
struct SetIntersection: public std::unary_function<LivetimeInterval&,void> {
	FreeUntilMap &free_until_pos;
	LivetimeInterval &current;
	UseDef pos;
	UseDef end;
	/// Constructor
	SetIntersection(FreeUntilMap &free_until_pos,
		LivetimeInterval &current, UseDef pos, UseDef end)
		: free_until_pos(free_until_pos), current(current), pos(pos), end(end) {}

	void operator()(LivetimeInterval& lti) {
		MachineOperand *MO = lti.get_operand();
		FreeUntilMap::iterator i = free_until_pos.find(MO);
		if (i != free_until_pos.end()) {
			UseDef inter = next_intersection(lti,current,pos,end);
			LOG2("SetIntersection: " << lti << " inter1: " << inter << nl);
			if (inter != end) {
				//UseDef inter = next_intersection(lti,current,pos,end);
				if (inter < i->second) {
					i->second = inter;
					LOG2("SetIntersection: " << lti << " operand: " << *MO << " to " << i->second << nl);
				}
			}
		}
	}
};


/**
 * @Cpp11 use std::function
 */
struct FreeUntilMaxCompare
		: public std::binary_function<const FreeUntilMap::value_type&, const FreeUntilMap::value_type&,bool> {
	bool operator()(const FreeUntilMap::value_type &lhs, const FreeUntilMap::value_type &rhs) const {
		return lhs.second < rhs.second;
	}
};

/**
 * @Cpp11 use std::function
 */
struct SetUseDefOperand: public std::unary_function<const UseDef&,void> {
	MachineOperand *op;
	/// Constructor
	SetUseDefOperand(MachineOperand* op) : op(op) {}
	void operator()(const UseDef &usedef) {
		LOG2("  set def operand " << usedef << " to " << op << nl);
		usedef.get_operand()->op = op;
	}
};


MIIterator insert_move_before(MachineInstruction* move, UseDef usedef) {
	MIIterator free_until_pos_reg = usedef.get_iterator();
	assert(!(*free_until_pos_reg)->is_label());
	return insert_before(free_until_pos_reg, move);

}

} // end anonymous namespace

inline bool LinearScanAllocatorPass::try_allocate_free(LivetimeInterval &current, UseDef pos) {
	LOG2(Magenta << "try_allocate_free: " << current << reset_color << nl);
	// set free until pos of all physical operands to maximum
	FreeUntilMap free_until_pos;
	OperandFile op_file;
	backend->get_OperandFile(op_file,current.get_operand());
	// set to end
	std::for_each(op_file.begin(),op_file.end(),
		InitFreeUntilMap(free_until_pos, UseDef(UseDef::PseudoDef,MIS->mi_end())));

	// for each interval in active set free until pos to zero
	std::for_each(active.begin(), active.end(),
		SetActive(free_until_pos, UseDef(UseDef::PseudoUse,MIS->mi_begin())));

	// for each interval in inactive set free until pos to next intersection
	std::for_each(inactive.begin(), inactive.end(),
		SetIntersection(free_until_pos, current,pos,UseDef(UseDef::PseudoDef,MIS->mi_end())));

	LOG2("free_until_pos:" << nl);
	if (DEBUG_COND_N(2)) {
		for (FreeUntilMap::iterator i = free_until_pos.begin(), e = free_until_pos.end(); i != e; ++i) {
			LOG2("  " << (i->first) << ": " << i->second << nl);
		}
	}

	MachineOperand *reg = NULL;
	UseDef free_until_pos_reg(UseDef::PseudoUse,MIS->mi_begin());
	// check for hints
	MachineOperand *hint = current.get_hint();
	if (option::hints && hint) {
		if (hint->is_virtual()) {
			// find the current store for hint
			LivetimeInterval hint_lti = LA->get(hint);
			hint = hint_lti.get_operand(pos.get_iterator());
		}
		LOG2("hint current pos " << hint << nl);
		FreeUntilMap::const_iterator i = free_until_pos.find(hint);
		if (i !=  free_until_pos.end()) {
			free_until_pos_reg = i->second;
			if ((free_until_pos_reg != UseDef(UseDef::PseudoUse,MIS->mi_begin())) &&
					(current.back().end < free_until_pos_reg)) {
				LOG2("hint available! " << hint << nl);
				reg = hint;
				STATISTICS(++num_hints_followed);
			}
			else {
				STATISTICS(++num_hints_not_followed);
			}
		}
		else {
			STATISTICS(++num_hints_not_followed);
		}
	}
	// if not follow hint!
	if (!reg) {
		// get operand with highest free until pos
		FreeUntilMap::value_type x = *std::max_element(free_until_pos.begin(), free_until_pos.end(), FreeUntilMaxCompare());
		reg = x.first;
		free_until_pos_reg = x.second;
	}

	LOG2("reg: " << reg << " pos: " << free_until_pos_reg << nl);

	if (free_until_pos_reg == UseDef(UseDef::PseudoUse,MIS->mi_begin())) {
		// no register available without spilling
		return false;
	}
	current.set_operand(reg);
	LOG2("assigned operand: " << *reg << nl);
	if (current.back().end < free_until_pos_reg) {
		// register available for the whole interval
		return true;
	}
	// register available for the first part of the interval
	MachineOperand *tmp = new VirtualRegister(reg->get_type());
	if(!(*free_until_pos_reg.get_iterator())->is_label()) {
		// insert move
		MachineInstruction *move = backend->create_Move(reg,tmp);
		STATISTICS(++num_split_moves);
		MIIterator split_pos = insert_move_before(move,free_until_pos_reg);
		assert_msg(pos.get_iterator() < split_pos, "pos: " << pos << " free_until_pos_reg "
			<< free_until_pos_reg << " split: " << split_pos);
		LOG2("splitting interval " << current << " at " << split_pos << " inserted move: " << move << nl);
		LivetimeInterval lti = current.split_active(split_pos,&pos);
		unhandled.push(lti);
	} else {
		// no move required
		LOG2("splitting interval " << current << " at " << free_until_pos_reg << " no move required, op" << tmp << nl);
		LivetimeInterval lti = current.split_phi_active(free_until_pos_reg.get_iterator(), tmp);
		unhandled.push(lti);
	}
	return true;
}

namespace {
/**
 * @Cpp11 use std::function
 */
struct SetNextUseActive: public std::unary_function<LivetimeInterval&,void> {
	FreeUntilMap &next_use_pos;
	UseDef pos;
	UseDef end;
	/// Constructor
	SetNextUseActive(FreeUntilMap &next_use_pos,
		UseDef pos, UseDef end) : next_use_pos(next_use_pos), pos(pos), end(end) {}

	void operator()(LivetimeInterval& lti) {
		MachineOperand *MO = lti.get_operand();
		//LOG2("SetActive: " << lti << " operand: " << *MO << nl);
		FreeUntilMap::iterator i = next_use_pos.find(MO);
		if (i != next_use_pos.end()) {
			//LOG2("set to zero" << nl);
			i->second = lti.next_usedef_after(pos,end);
			LOG2("SetNextUseActive: " << lti << " operand: " << *MO << " to " << i->second << nl);
			// work around self use issue
			MachineInstruction *MI = *pos.get_iterator();
			MachineOperand *origMO = lti.get_init_operand();
			LOG2("MI: " << MI << " orig: " << origMO << nl);

			if (MI->find(origMO) != MI->end()) {
				// the operand is used in the interval start instruction
				// therefore we can not use the operand -> set to current pos
				i->second = pos;
			}

		}
	}
};

/**
 * @Cpp11 use std::function
 */
struct SetNextUseInactive: public std::unary_function<LivetimeInterval&,void> {
	FreeUntilMap &free_until_pos;
	LivetimeInterval &current;
	UseDef pos;
	UseDef end;
	/// Constructor
	SetNextUseInactive(FreeUntilMap &free_until_pos,
		LivetimeInterval &current, UseDef pos, UseDef end)
		: free_until_pos(free_until_pos), current(current), pos(pos), end(end) {}

	void operator()(LivetimeInterval& lti) {
		MachineOperand *MO = lti.get_operand();
		FreeUntilMap::iterator i = free_until_pos.find(MO);
		if (i != free_until_pos.end() && next_intersection(lti,current,pos,end) != end) {
			i->second = lti.next_usedef_after(pos,end);
			LOG2("SetNextUseInactive: " << lti << " operand: " << *MO << " to " << i->second << nl);
		}
	}
};

inline MachineOperand* get_stackslot(Backend *backend, Type::TypeID type) {
	return backend->get_JITData()->get_StackSlotManager()->create_ManagedStackSlot(type);
}

void split_active_position(LivetimeInterval lti, UseDef current_pos, UseDef next_use_pos, Backend *backend,
		LinearScanAllocatorPass::UnhandledSetTy &unhandled) {
	MachineOperand *MO = lti.get_operand();

	MachineOperand *stackslot = NULL;
	if (current_pos.is_pseudo()) {
		// We do not create moves for pseudo positions. This is handled during
		// resolve.
		if(lti.front().start == current_pos) {
			// we are trying to spill a phi output operand interval
			// just set the operand
			stackslot = get_stackslot(backend,MO->get_type());
			lti.set_operand(stackslot);
		}
		else {
			// we are trying to spill a phi output operand interval
			// split without a move
			stackslot = get_stackslot(backend,MO->get_type());
			lti = lti.split_phi_active(current_pos.get_iterator(),stackslot);
			// XXX should we add the stack interval?
			unhandled.push(lti);
		}
	}
	else {
		// move to stack
		stackslot = get_stackslot(backend,MO->get_type());
		MachineInstruction *move_to_stack = backend->create_Move(MO,stackslot);
		MIIterator split_pos = insert_move_before(move_to_stack,current_pos);
		STATISTICS(++num_spill_stores);
		lti = lti.split_active(split_pos,&current_pos);
		// XXX should we add the stack interval?
		unhandled.push(lti);
	}
	// move from stack
	if (!next_use_pos.is_pseudo()) {
		// if the next use position is no real use/def we can ignore it
		// the moves, if required, will be inserted by the resolution phase
		MachineOperand *vreg = new VirtualRegister(MO->get_type());
		MachineInstruction *move_from_stack = backend->create_Move(stackslot, vreg);
		MIIterator split_pos = insert_move_before(move_from_stack,next_use_pos);
		STATISTICS(++num_spill_loads);
		lti = lti.split_active(split_pos);
		unhandled.push(lti);
	}
}

/**
 * @Cpp11 use std::function
 */
struct SplitActive: public std::unary_function<LivetimeInterval&,void> {
	MachineOperand *reg;
	UseDef pos;
	UseDef next_use_pos;
	Backend *backend;
	LinearScanAllocatorPass::UnhandledSetTy &unhandled;
	/// Constructor
	SplitActive(MachineOperand *reg , UseDef pos, UseDef next_use_pos, Backend *backend,
		LinearScanAllocatorPass::UnhandledSetTy &unhandled)
			: reg(reg), pos(pos), next_use_pos(next_use_pos), backend(backend), unhandled(unhandled) {}

	void operator()(LivetimeInterval& lti) {
		MachineOperand *MO = lti.get_operand();
		if (reg->aquivalent(*MO)) {
			split_active_position(lti,pos,next_use_pos,backend, unhandled);
		}
	}
};

/**
 * @Cpp11 use std::function
 */
struct SplitInactive: public std::unary_function<LivetimeInterval&,void> {
	MachineOperand *reg;
	UseDef pos;
	LinearScanAllocatorPass::UnhandledSetTy &unhandled;
	/// Constructor
	SplitInactive(MachineOperand *reg , UseDef pos,	LinearScanAllocatorPass::UnhandledSetTy &unhandled)
			: reg(reg), pos(pos), unhandled(unhandled) {}

	void operator()(LivetimeInterval& lti) {
		MachineOperand *MO = lti.get_operand();
		if (reg->aquivalent(*MO)) {
			//ABORT_MSG("Spill current not yet implemented","not yet implemented");
			LivetimeInterval new_lti = lti.split_inactive(pos, new VirtualRegister(reg->get_type()));
			STATISTICS(++num_split_moves);
			assert(lti.front().start < new_lti.front().start);
			unhandled.push(new_lti);
		}
	}
};

inline void split_current(LivetimeInterval &current, Type::TypeID type, UseDef pos, UseDef end,
		LinearScanAllocatorPass::UnhandledSetTy &unhandled, Backend *backend) {
	LOG2("Spill current (" << current << ") at " << pos << nl);
	// create move
	MachineOperand *stackslot = get_stackslot(backend,type);
	// only create new register if there is a user
	if ( pos != end) {
		MachineOperand *vreg = new VirtualRegister(type);
		MachineInstruction *move_from_stack = backend->create_Move(stackslot,vreg);
		// split
		MIIterator split_pos = insert_move_before(move_from_stack,pos);
		STATISTICS(++num_spill_loads);
		LivetimeInterval new_lti = current.split_active(split_pos);
		unhandled.push(new_lti);
	}
	// set operand
	current.set_operand(stackslot);
}

#if defined(ENABLE_LOGGING)
OStream& print_code(OStream &OS, MIIterator pos, MIIterator min, MIIterator max,
		MIIterator::difference_type before, MIIterator::difference_type after) {
	MIIterator i = pos;
	MIIterator e = pos;
	before = std::min(before,std::distance(min,pos));
	after = std::min(after,std::distance(pos,max));
	std::advance(i,-before);
	std::advance(e,after+1);

	OS << "..." << nl;
	for ( ; i != e ; ++i) {
		if (i == pos) {
			OS << BoldYellow;
		}
		OS << i << reset_color << nl;
	}
	return OS << "..." << nl;
}
#endif
} // end anonymous namespace

inline bool LinearScanAllocatorPass::allocate_blocked(LivetimeInterval &current) {
	LOG2(Blue << "allocate_blocked: " << current << reset_color << nl);
	// set free until pos of all physical operands to maximum
	FreeUntilMap next_use_pos;
	OperandFile op_file;
	backend->get_OperandFile(op_file,current.get_operand());
	// set to end
	std::for_each(op_file.begin(),op_file.end(),
		InitFreeUntilMap(next_use_pos, UseDef(UseDef::PseudoDef,MIS->mi_end())));

	// for each interval it in active set next use to next use after start of current
	std::for_each(active.begin(), active.end(),
		SetNextUseActive(next_use_pos, current.front().start, UseDef(UseDef::PseudoDef,MIS->mi_end())));

	// for each interval in inactive intersecting with current  set next pos to next use
	std::for_each(inactive.begin(), inactive.end(),
		SetNextUseInactive(next_use_pos, current, current.front().start, UseDef(UseDef::PseudoDef,MIS->mi_end())));

	LOG2("next_use_pos:" << nl);
	for (FreeUntilMap::iterator i = next_use_pos.begin(), e = next_use_pos.end(); i != e; ++i) {
		LOG2("  " << (i->first) << ": " << i->second << nl);
	}

	// get register with highest nextUsePos
	FreeUntilMap::value_type x = *std::max_element(next_use_pos.begin(), next_use_pos.end(), FreeUntilMaxCompare());
	MachineOperand *reg = x.first;
	UseDef next_use_pos_reg = x.second;
	LOG2("reg: " << *reg << " pos: " << next_use_pos_reg << nl);

	UseDef next_use_pos_current = current.next_usedef_after(current.front().start,
		UseDef(UseDef::PseudoDef,MIS->mi_end()));
	if (next_use_pos_reg < next_use_pos_current && !current.front().start.is_def()) {
		// all other intervals are used before current
		// so spill current
		// if the start of the current interval is a definition, we need to assign a register
		split_current(current,current.get_operand()->get_type(), next_use_pos_current,
			UseDef(UseDef::PseudoDef,MIS->mi_end()), unhandled, backend);
		//ERROR_MSG("Spill current not yet implemented","not yet implemented");
		return true;
	}
	else {
		LOG2("Spill intervals blocking: " << *reg << nl);
		// spill intervals that currently block reg

		// split for each interval it in active that blocks reg (there can be only one)
		std::for_each(active.begin(), active.end(),
			SplitActive(reg, current.front().start, next_use_pos_reg, backend, unhandled));

		// spill each interval it in inactive for reg at the end of the livetime hole
		std::for_each(inactive.begin(), inactive.end(),
			SplitInactive(reg, current.front().start, unhandled));
		current.set_operand(reg);
		LOG2("assigned operand: " << *reg << nl);
		return true;
	}
	return false;
}

void LinearScanAllocatorPass::initialize() {
	while (!unhandled.empty()) {
		unhandled.pop();
	}
	handled.clear();
	inactive.clear();
	active.clear();
}

bool LinearScanAllocatorPass::allocate_unhandled() {
	while(!unhandled.empty()) {
		LivetimeInterval current = unhandled.top();
		unhandled.pop();
		LOG(BoldCyan << "current: " << current << reset_color<< nl);
		UseDef pos = current.front().start;
		DEBUG3(print_code(dbg(), pos.get_iterator(),MIS->mi_begin(), MIS->mi_end(), 2,2));

		// check for intervals in active that are handled or inactive
		LOG2("check for intervals in active that are handled or inactive" << nl);
		for (ActiveSetTy::iterator i = active.begin(), e = active.end();
				i != e ; /* ++i */) {
			LivetimeInterval act = *i;
			switch (act.get_State(pos)) {
			case LivetimeInterval::Handled:
				LOG2("  LTI " << act << " moved from active to handled" << nl);
				handled.push_back(act);
				i = active.erase(i);
				break;
			case LivetimeInterval::Inactive:
				LOG2("  LTI " << act << " moved from active to inactive" << nl);
				inactive.push_back(act);
				i = active.erase(i);
				break;
			default:
				++i;
			}
		}
		// check for intervals in inactive that are handled or active
		LOG2("check for intervals in inactive that are handled or active" << nl);
		for (InactiveSetTy::iterator i = inactive.begin(), e = inactive.end();
				i != e ; /* ++i */) {
			LivetimeInterval inact = *i;
			switch (inact.get_State(pos)) {
			case LivetimeInterval::Handled:
				LOG2("  LTI " << inact << " moved from inactive to handled" << nl);
				handled.push_back(inact);
				i = inactive.erase(i);
				break;
			case LivetimeInterval::Active:
				LOG2("  LTI " << inact << " moved from inactive to active" << nl);
				active.push_back(inact);
				i = inactive.erase(i);
				break;
			default:
				++i;
			}
		}

		LOG2("active: ");
		DEBUG2(print_container(dbg(),active.begin(),active.end())<<nl);
		LOG2("inactive: ");
		DEBUG2(print_container(dbg(),inactive.begin(),inactive.end())<<nl);

		if (current.get_operand()->is_virtual()) {
			// Try to find a register
			if (!try_allocate_free(current,pos)) {
				if (!allocate_blocked(current)) {
					ERROR_MSG("Register allocation failed",
						"could not allocate register for " << current);
					return false;
				}
				else {
					STATISTICS(++num_allocate_blocked);
				}
			}
			else {
				STATISTICS(++num_allocate_free);
			}
		}
		else {
			STATISTICS(++num_fixed_intervals);
			// fixed interval
			for (ActiveSetTy::iterator i = active.begin(), e = active.end(); i != e ; ++i ) {
				LivetimeInterval act = *i;
				if (current.get_operand()->aquivalent(*act.get_operand())) {
					MachineInstruction *MI = *pos.get_iterator();
					if (!(MI->is_move() && MI->get_result().op->aquivalent(*MI->get(0).op))) {
						LOG2("Spill intervals blocking fixed: " << current.get_operand() << nl);
						// spill intervals that currently block reg
						UseDef next_use_pos = act.next_usedef_after(
							current.front().start,UseDef(UseDef::PseudoDef,MIS->mi_end()));
						SplitActive split (current.get_operand(), current.front().start, 
									next_use_pos, backend, unhandled);
						SplitActive::argument_type split_act = act;
						split(split_act);
						// spill each interval in inactive for reg at the end of the livetime hole
						std::for_each(inactive.begin(), inactive.end(),
							SplitInactive(current.get_operand(), current.front().start, 
								unhandled));
					}
				}
			}
		}
		// add current to active
		active.push_back(current);
		LOG2("LTI " << current << " moved from unhandled to handled" << nl);
	}

	// move everything to handled
	for (ActiveSetTy::iterator i = active.begin(), e = active.end(); i != e ; /* ++i */) {
		handled.push_back(*i);
		i = active.erase(i);
	}
	for (InactiveSetTy::iterator i = inactive.begin(), e = inactive.end(); i != e ; /* ++i */) {
		handled.push_back(*i);
		i = inactive.erase(i);
	}
	return true;
}

bool LinearScanAllocatorPass::run(JITData &JD) {
	LA = get_Pass<LivetimeAnalysisPass>();
	MIS = get_Pass<MachineInstructionSchedulingPass>();
	jd = &JD;
	backend = jd->get_Backend();

	for (LivetimeAnalysisPass::iterator i = LA->begin(), e = LA->end();
			i != e ; ++i) {
		LivetimeInterval &inter = i->second;
		unhandled.push(inter);
	}
	// allocate
	if (!allocate_unhandled()) return false;
	// resolve
	if (!resolve())	return false;

	// set operands
	for (HandledSetTy::const_iterator i = handled.begin(), e = handled.end(); i != e ; ++i) {
		LivetimeInterval lti = *i;
		LOG("Assigned Interval " << lti << " (init: " << *lti.get_init_operand() << ")" <<nl );
		std::for_each(lti.use_begin(), lti.use_end(), SetUseDefOperand(lti.get_operand()));
		std::for_each(lti.def_begin(), lti.def_end(), SetUseDefOperand(lti.get_operand()));
	}
	return true;
}

typedef alloc::map<Type::TypeID, alloc::set<MachineOperand*,MachineOperandComp>::type >::type FreeRegsMap;

namespace {
/**
 * @Cpp11 use std::function
 */
template <class BinaryOperation>
class CfgEdgeFunctor : public std::unary_function<MachineBasicBlock*,void> {
private:
	BinaryOperation op;
public:
	/// Constructor
	CfgEdgeFunctor(BinaryOperation op) : op(op) {}
	void operator()(MachineBasicBlock *predecessor) {
		std::for_each(predecessor->back()->successor_begin(),
			predecessor->back()->successor_end(), std::bind1st(op,predecessor));
	}
};
/// Wrap class template argument.
template <class BinaryOperation>
CfgEdgeFunctor<BinaryOperation>	CfgEdge(BinaryOperation op) {
	return CfgEdgeFunctor<BinaryOperation>(op);
}

class CalcBBtoLTIMap : public std::unary_function<MachineBasicBlock*,void> {
private:
	LivetimeAnalysisPass* LA;
	BBtoLTI_Map &bb2lti_map;

	struct inner : public std::unary_function<LivetimeAnalysisPass::LivetimeIntervalMapTy::value_type,void> {
		MachineBasicBlock *MBB;
		BBtoLTI_Map &bb2lti_map;
		/// constructor
		inner(MachineBasicBlock *MBB, BBtoLTI_Map &bb2lti_map) : MBB(MBB), bb2lti_map(bb2lti_map) {}
		/// function call operator
		void operator()(argument_type lti_pair) {
			LivetimeInterval &lti = lti_pair.second;
			if (lti.get_State(UseDef(UseDef::PseudoUse,MBB->mi_first())) == LivetimeInterval::Active) {
				bb2lti_map[MBB].push_back(lti);
			}
			else {
				// we need to add the _initial_ lti to find the correct location later on.
				LivetimeInterval lti_next = lti;
				while (lti_next.has_next()) {
					lti_next = lti_next.get_next();
					if (lti_next.get_State(UseDef(UseDef::PseudoUse,MBB->mi_first())) == LivetimeInterval::Active) {
						bb2lti_map[MBB].push_back(lti);
					}
				}
			}
		}
	};
public:
	/// constructor
	CalcBBtoLTIMap(LivetimeAnalysisPass* LA, BBtoLTI_Map &bb2lti_map)
		: LA(LA), bb2lti_map(bb2lti_map) {}
	/// function call operator
	void operator()(MachineBasicBlock *MBB) {
		std::for_each(LA->begin(),LA->end(),inner(MBB,bb2lti_map));
	}
};

#if 0
bool operator<(const Move &lhs, const Move &rhs) {
	if (lhs.from < rhs.from)
		return true;
	if (rhs.from < lhs.from)
		return false;
	return lhs.to < rhs.to;
}
#endif

class ForEachLiveiterval : public std::unary_function<LivetimeInterval&,void> {
private:
	MachineBasicBlock *predecessor;
	MachineBasicBlock *successor;
	LivetimeAnalysisPass *LA;
	MoveMapTy &move_map;
public:
	/// constructor
	ForEachLiveiterval(MachineBasicBlock *predecessor, MachineBasicBlock *successor,
		LivetimeAnalysisPass *LA, MoveMapTy &move_map) :
		predecessor(predecessor), successor(successor), LA(LA), move_map(move_map) {}
	/// function call operator
	void operator()(LivetimeInterval& lti) const {
		LOG2("live: " << lti << " op: " << *lti.get_init_operand() << nl);

		MachineOperand *move_from;
		// if is phi
		if (lti.front().start.get_iterator() == successor->mi_first()) {
			MachinePhiInst *phi = get_phi_from_operand(successor, lti.get_init_operand());
			assert(phi);
			LOG2("starts at successor begin: " << *phi << nl);
			std::size_t index = successor->get_predecessor_index(predecessor);
			MachineOperand *op = phi->get(index).op;
			if (op->is_Immediate()) {
				ABORT_MSG("PHI with immediate operand", "Can not yet handle phis with immediate operands");
				move_from = NULL;
			} else {
				move_from = LA->get(op).get_operand(predecessor->mi_last());
			}

		}
		else {
			move_from = lti.get_operand(predecessor->mi_last());
		}
		MachineOperand *move_to = lti.get_operand(successor->mi_first());
		LOG3(" from: " << move_from << " to " << move_to << nl);
		if (!move_from->aquivalent(*move_to)) {
			move_map.push_back(Move(move_from,move_to));
		}
	}
};

#if defined(ENABLE_LOGGING)
OStream& operator<<(OStream &OS, const FreeRegsMap &free_regs) {
	for (FreeRegsMap::const_iterator i = free_regs.begin(), e = free_regs.end() ; i!=e ; ++i) {
		Type::TypeID type = i->first;
		const FreeRegsMap::mapped_type &regs = i->second;
		OS << "type: " << type << " ";
		print_ptr_container(OS,regs.begin(), regs.end()) << nl;
	}
	return OS;
}
#endif

Type::TypeID types[] = {
	Type::ByteTypeID,
	Type::IntTypeID,
	Type::LongTypeID,
	Type::ReferenceTypeID,
	Type::DoubleTypeID,
	Type::VoidTypeID
};

inline void free_regs_remove_op(FreeRegsMap &free_regs, MachineOperand *op) {
	for (Type::TypeID *p = types; *p != Type::VoidTypeID; ++p) {
		Type::TypeID type = *p;
		free_regs[type].erase(op);
	}
}

inline MachineOperand* get_and_remove_free_regs(FreeRegsMap &free_regs, Type::TypeID type) {
	FreeRegsMap::mapped_type &map = free_regs[type];
	if(map.empty()) {
		// no more registers
		return NULL;
	}
	STATISTICS(++num_resolution_regs);
	// arbitrary select the first free register
	FreeRegsMap::mapped_type::iterator i = map.begin();
	MachineOperand *op = *i;
	map.erase(i);
	free_regs_remove_op(free_regs,op);
	return op;
}


/**
 * @todo merge with movemap creation
 */
inline void calc_free_regs(FreeRegsMap &free_regs, BBtoLTI_Map &bb2lti_map, MachineBasicBlock* predecessor,
		MachineBasicBlock *successor, Backend *backend, LivetimeAnalysisPass *LA) {
	// get all potential registers
	for (Type::TypeID *p = types; *p != Type::VoidTypeID; ++p) {
		Type::TypeID type = *p;
		OperandFile OF;
		VirtualRegister vreg(type);
		backend->get_OperandFile(OF,&vreg);
		free_regs[type].insert(OF.begin(),OF.end());
	}
	// remove used operands
	BBtoLTI_Map::mapped_type &ltis = bb2lti_map[successor];
	LOG2(free_regs << nl);
	for (BBtoLTI_Map::mapped_type::iterator i = ltis.begin(), e = ltis.end(); i != e; ++i) {
		LivetimeInterval lti = *i;
		MachineOperand *op_pred;
		// if is phi
		if (lti.front().start.get_iterator() == successor->mi_first()) {
			MachinePhiInst *phi = get_phi_from_operand(successor, lti.get_init_operand());
			assert(phi);
			std::size_t index = successor->get_predecessor_index(predecessor);
			MachineOperand *op = phi->get(index).op;
			op_pred = LA->get(op).get_operand(predecessor->mi_last());
		} else {
			op_pred = lti.get_operand(predecessor->mi_last());
		}
		MachineOperand *op_succ = lti.get_operand(successor->mi_first());
		assert(op_succ);
		assert(op_pred);
		// delete from _all_ types
		LOG2("calc_free_regs: " << op_pred << " -> " << op_succ << nl);
		free_regs_remove_op(free_regs,op_succ);
		free_regs_remove_op(free_regs,op_pred);
	}
}

#if defined(ENABLE_LOGGING)
inline OStream& operator<<(OStream &OS, const Move &move) {
	return OS << "move from " << *move.from << " to " << *move.to;
}
#endif

inline bool is_stack2stack_move(Move* move) {
	return (move->from->is_ManagedStackSlot() || move->from->is_StackSlot() ) &&
		(move->to->is_ManagedStackSlot() || move->to->is_StackSlot());
}

bool compare_moves(Move *lhs, Move *rhs) {
	if (!lhs->dep)
		return true;
	if (!rhs->dep)
		return false;
	return lhs < rhs;
}

class MoveScheduler {
private:
	alloc::list<MachineInstruction*>::type &scheduled;
	MoveMapTy &move_map;
	alloc::list<Move*>::type &queue;
	Backend *backend;
	alloc::list<Move*>::type &stack;
	FreeRegsMap &free_regs;
public:
	/// constructor
	MoveScheduler(alloc::list<MachineInstruction*>::type &scheduled, MoveMapTy &move_map,
		alloc::list<Move*>::type &queue, Backend *backend, alloc::list<Move*>::type &stack,
		FreeRegsMap &free_regs)
		: scheduled(scheduled), move_map(move_map), queue(queue), backend(backend), stack(stack),
			free_regs(free_regs) {}
	/// call operator
	void operator()() {
		Move* node = stack.back();
		assert(!node->from->aquivalent(*node->to));
		LOG2("schedule pre: " << *node << nl);
		// already scheduled?
		if (node->is_scheduled()) {
			stack.pop_back();
			return;
		}
		if (is_stack2stack_move(node)){
			LOG2("Stack to stack move detected!" << nl);
			MachineOperand *tmp = get_and_remove_free_regs(free_regs,node->to->get_type());
			if (!tmp) {
				// No more free register!
				Type::TypeID type = node->from->get_type();
				// get stack slot
				MachineOperand *tmp_stack = backend->get_JITData()->
					get_StackSlotManager()->create_ManagedStackSlot(type);
				// get reg
				OperandFile OF;
				backend->get_OperandFile(OF,tmp_stack);
				MachineOperand *tmp_reg = OF.front();
				// backup register
				scheduled.push_back(backend->create_Move(tmp_reg,tmp_stack));
				// stack to stack move
				scheduled.push_back(backend->create_Move(node->from,tmp_reg));
				scheduled.push_back(backend->create_Move(tmp_reg,node->to));
				// restore register
				scheduled.push_back(backend->create_Move(tmp_stack,tmp_reg));
				//
				LOG2("schedule post: " << *node << nl);
				stack.pop_back();
				node->scheduled = true;
				return;
			}

			move_map.push_back(Move(tmp, node->to));
			queue.push_back(&move_map.back());
			node->to = tmp;
			// no really needed, is it?
			node->dep = NULL;
		}
		else if (node->dep && !node->dep->is_scheduled()) {
			alloc::list<Move*>::type::iterator i = std::find(stack.begin(),stack.end(),node->dep);
			if (i != stack.end()) {
				LOG2("cycle detected!" << nl);
				// try to get a register
				MachineOperand *tmp = get_and_remove_free_regs(free_regs,node->to->get_type());
				if (!tmp) {
					// No more free register -> use stack slot
					assert(!node->from->is_StackSlot());
					assert(!node->to->is_StackSlot());
					tmp = get_stackslot(backend,node->to->get_type());
					STATISTICS(++num_resolution_stacks);
				}
				move_map.push_back(Move(tmp, node->to));
				Move *tmp_move = &move_map.back();
				tmp_move->dep = *i;
				queue.push_back(tmp_move);
				node->to = tmp;
				node->dep = NULL;
			} else {
				stack.push_back(node->dep);
				operator()();
			}
		}
		LOG2("schedule post: " << *node << nl);
		stack.pop_back();
		scheduled.push_back(backend->create_Move(node->from,node->to));
		node->scheduled = true;
	}
};

template <class OutputIterator>
struct RefToPointerInserterImpl: public std::unary_function<Move&,void> {
	OutputIterator i;
	/// constructor
	RefToPointerInserterImpl(OutputIterator i) : i(i) {}
	/// call operator
	void operator()(Move& move) {
		*i = &move;
	}
};
/// wrapper
template <class OutputIterator>
inline RefToPointerInserterImpl<OutputIterator> RefToPointerInserter(OutputIterator i) {
	return RefToPointerInserterImpl<OutputIterator>(i);
}

inline LivetimeInterval& find_or_insert(
		LivetimeAnalysisPass::LivetimeIntervalMapTy &lti_map, MachineOperand* op) {
	LivetimeAnalysisPass::LivetimeIntervalMapTy::iterator i = lti_map.find(op);
	if (i == lti_map.end()) {
		LivetimeInterval lti(op);
		i = lti_map.insert(std::make_pair(op,lti)).first;
	}
	return i->second;
}
/**
 * @Cpp11 use std::function
 */
class ProcessOutOperands : public std::unary_function<MachineOperandDesc,void> {
private:
	LivetimeAnalysisPass::LivetimeIntervalMapTy &lti_map;
	MIIterator i;
	MIIterator e;
public:
	/// Constructor
	ProcessOutOperands(LivetimeAnalysisPass::LivetimeIntervalMapTy &lti_map,
			MIIterator i, MIIterator e)
				: lti_map(lti_map), i(i), e(e) {}
	void operator()(MachineOperandDesc &op) {
		if (op.op->is_virtual()) {
			find_or_insert(lti_map,op.op).set_from(UseDef(UseDef::Def,i,&op), UseDef(UseDef::PseudoDef,e));
		}
	}
};
/**
 * @Cpp11 use std::function
 */
class ProcessInOperands : public std::unary_function<MachineOperandDesc,void> {
private:
	LivetimeAnalysisPass::LivetimeIntervalMapTy &lti_map;
	MIIterator first;
	MIIterator current;
public:
	/// Constructor
	ProcessInOperands(LivetimeAnalysisPass::LivetimeIntervalMapTy &lti_map,
		MIIterator first, MIIterator current) : lti_map(lti_map), first(first),
		current(current) {}
	void operator()(MachineOperandDesc &op) {
		if (op.op->is_virtual()) {
			find_or_insert(lti_map,op.op).add_range(UseDef(UseDef::PseudoUse,first),
				UseDef(UseDef::Use,current,&op));
		}
	}
};

class ForEachCfgEdge : public std::binary_function<MachineBasicBlock*,MachineBasicBlock*,void> {
private:
	BBtoLTI_Map &bb2lti_map;
	EdgeMoveMapTy &edge_move_map;
	LivetimeAnalysisPass *LA;

public:
	/// constructor
	ForEachCfgEdge(BBtoLTI_Map &bb2lti_map, EdgeMoveMapTy &edge_move_map, LivetimeAnalysisPass *LA)
		: bb2lti_map(bb2lti_map), edge_move_map(edge_move_map), LA(LA) {}
	/// function call operator
	void operator()(MachineBasicBlock *predecessor, MachineBasicBlock *successor) const {
		MoveMapTy &move_map = edge_move_map[Edge(predecessor,successor)];
		LOG2(Cyan << "edge " << *predecessor << " -> " << *successor << nl);
		BBtoLTI_Map::mapped_type &lti_live_set = bb2lti_map[successor];
		// for each live interval live at successor
		std::for_each(lti_live_set.begin(), lti_live_set.end(),
			ForEachLiveiterval(predecessor,successor, LA, move_map));
	}
};

} // end anonymous namespace


//bool LinearScanAllocatorPass::order_and_insert_move(MachineBasicBlock *predecessor, MachineBasicBlock *successor,
//		MoveMapTy &move_map) {
bool LinearScanAllocatorPass::order_and_insert_move(EdgeMoveMapTy::value_type &entry, BBtoLTI_Map &bb2lti_map) {
	MachineBasicBlock *predecessor = entry.first.predecessor;
	MachineBasicBlock *successor = entry.first.successor;
	MoveMapTy &move_map = entry.second;

	LOG2("order_and_insert_move: " << *predecessor << " -> " << *successor << nl);
	// get free registers
	FreeRegsMap free_regs;
	calc_free_regs(free_regs,bb2lti_map,predecessor,successor,backend, LA);
	LOG2(free_regs << nl);

	if (move_map.empty()) return true;
	// calculate data dependency graph (DDG)
	alloc::map<MachineOperand*,Move*,MachineOperandComp>::type read_from;

	// create graph nodes
	for (MoveMapTy::iterator i = move_map.begin(), e = move_map.end(); i != e; ++i) {
		LOG2("need move from " << i->from << " to " << i->to << nl);
		read_from[i->from] = &*i;
	}
	// create graph edges
	for (MoveMapTy::iterator i = move_map.begin(), e = move_map.end(); i != e; ++i) {
		i->dep = read_from[i->to];
	}
	// nodes already scheduled
	alloc::list<MachineInstruction*>::type scheduled;
	alloc::list<Move*>::type stack;
	alloc::list<Move*>::type queue;
	std::for_each(move_map.begin(), move_map.end(), RefToPointerInserter(std::back_inserter(queue)));
	assert(move_map.size() == queue.size());

	MoveScheduler scheduler(scheduled, move_map, queue, backend, stack, free_regs);

	while (!queue.empty()) {
		// sort and pop element
		queue.sort(compare_moves);
		stack.push_back(queue.front());
		queue.pop_front();
		// schedule
		scheduler();
		assert(stack.empty());
	}

	MIIterator last = get_edge_iterator(predecessor, successor,backend);
	MIIterator first = last;
	// insert first element
	insert_before(last,scheduled.front());
	--first;
	for (alloc::list<MachineInstruction*>::type::const_iterator i = ++scheduled.begin(),
			e  = scheduled.end(); i != e ; ++i) {
		MachineInstruction *MI = *i;
		insert_before(last, MI);
		#if defined(ENABLE_STATISTICS)
		if (MI->front().op->is_ManagedStackSlot() && MI->back().op->is_Register()) {
			STATISTICS(++num_spill_loads);
		}
		else if (MI->front().op->is_Register() && MI->back().op->is_ManagedStackSlot()) {
			STATISTICS(++num_spill_stores);
		}
		else {
			STATISTICS(++num_resolution_moves);
		}
		#endif
	}
	return true;
}


bool LinearScanAllocatorPass::reg_alloc_resolve_block(MIIterator first, MIIterator last) {
	assert(active.empty());
	assert(inactive.empty());
	assert(unhandled.empty());
	std::size_t handled_size = handled.size();
	/// calculate state sets
	for (HandledSetTy::iterator i = handled.begin(), e = handled.end();
			i != e ; /* ++i */ ) {
		LivetimeInterval lti = *i;
		switch (lti.get_State(first)) {
		case LivetimeInterval::Active:
			active.push_back(lti);
			i = handled.erase(i);
			break;
		case LivetimeInterval::Inactive:
			inactive.push_back(lti);
			i = handled.erase(i);
			break;
		default:
			++i;
			break;
		}
	}
	/// calculate intervals
	LivetimeAnalysisPass::LivetimeIntervalMapTy lti_map;
	MIIterator current = last;
	while (first != current) {
		--current;
		// for each output operand of MI
		ProcessOutOperands(lti_map, current, last)((*current)->get_result());
		// for each input operand of MI
		std::for_each((*current)->begin(),(*current)->end(),
			ProcessInOperands(lti_map, current, last));
	}
	for(LivetimeAnalysisPass::LivetimeIntervalMapTy::iterator i = lti_map.begin(),
			e = lti_map.end(); i != e; ++i) {
		unhandled.push(i->second);
	}
	handled_size += unhandled.size();
	allocate_unhandled();
	assert_msg(handled.size() == handled_size, "handled.size(): " << handled.size() << " != handled_size: "
		<< handled_size);
	return true;
}

bool LinearScanAllocatorPass::resolve() {
	LOG(BoldGreen << "resolve: " << nl);
	// calculate basicblock to live interval map
	BBtoLTI_Map bb2lti_map;
	std::for_each(MIS->begin(), MIS->end(), CalcBBtoLTIMap(LA,bb2lti_map));

	if (DEBUG_COND_N(2))
	for (BBtoLTI_Map::const_iterator i = bb2lti_map.begin(), e = bb2lti_map.end(); i != e ; ++i) {
		LOG2("live at: " << *i->first << nl);
		for (BBtoLTI_Map::mapped_type::const_iterator ii = i->second.begin(), ee = i->second.end(); ii != ee ; ++ii) {
			LOG2("  " << *ii << nl);
		}
	}

	EdgeMoveMapTy edge_move_map;
	// for each cfg edge from predecessor to successor (implemented in ForEachCfgEdge)
	std::for_each(MIS->begin(), MIS->end(), CfgEdge(ForEachCfgEdge(bb2lti_map,edge_move_map,LA)));
	// order and insert move
	for (EdgeMoveMapTy::iterator i = edge_move_map.begin(), e = edge_move_map.end(); i != e ; ++i) {
		if (!order_and_insert_move(*i,bb2lti_map)) {
			return false;
		}
	}
	// remove all phis
	std::for_each(MIS->begin(), MIS->end(), std::mem_fun(&MachineBasicBlock::phi_clear));

	return true;
}

#define LSRA_VERIFY
namespace {

#if defined(LSRA_VERIFY)
inline bool is_virtual(const MachineOperandDesc &op) {
	return op.op->is_virtual();
}
#endif

} // end anonymous namespace
bool LinearScanAllocatorPass::verify() const {
#if defined(LSRA_VERIFY)
	for(MIIterator i = MIS->mi_begin(), e = MIS->mi_end(); i != e ; ++i) {
		MachineInstruction *MI = *i;
		// result virtual
		if (is_virtual(MI->get_result()) || cacao::any_of(MI->begin(), MI->end(), is_virtual)) {
			ERROR_MSG("Unallocated Operand!","Instruction " << *MI << " contains unallocated operands.");
			return false;
		}

	}
#endif
	return true;
}


// pass usage
PassUsage& LinearScanAllocatorPass::get_PassUsage(PassUsage &PU) const {
	// requires
	PU.add_requires<LivetimeAnalysisPass>();
	PU.add_requires<MachineInstructionSchedulingPass>();
	// modified
	PU.add_modifies<MachineInstructionSchedulingPass>();
	// destroys
	// not yet updated correctly (might be only changed)
	PU.add_destroys<LivetimeAnalysisPass>();
	return PU;
}

// the address of this variable is used to identify the pass
char LinearScanAllocatorPass::ID = 0;

// register pass
static PassRegistry<LinearScanAllocatorPass> X("LinearScanAllocatorPass");

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