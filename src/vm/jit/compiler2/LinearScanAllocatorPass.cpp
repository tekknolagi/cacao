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
#include "vm/jit/compiler2/ListSchedulingPass.hpp"
#include "vm/jit/compiler2/BasicBlockSchedulingPass.hpp"
#include "vm/jit/compiler2/ScheduleClickPass.hpp"
#include "vm/statistics.hpp"

#include <climits>

#include "toolbox/logging.hpp"

#define DEBUG_NAME "compiler2/LinearScanAllocator"

STAT_REGISTER_GROUP(lsra_stat,"LSRA","Linear Scan Register Allocator")
STAT_REGISTER_GROUP_VAR(unsigned,num_hints_followed,0,"hints followed",
	"Number of Hints followed (i.e. no move)",lsra_stat)

namespace cacao {
namespace jit {
namespace compiler2 {

bool LinearScanAllocatorPass::StartComparator::operator()(const LivetimeInterval &lhs,
		const LivetimeInterval &rhs) {
	return rhs.front().start < lhs.front().start;
}


#if 0
template <typename Key, typename T>
bool max_value_comparator(const std::pair<Key,T> &lhs, const std::pair<Key,T> &rhs) {
	return lhs.second < rhs.second;
}

void LinearScanAllocatorPass::split(LivetimeInterval *lti, unsigned pos) {
#if 0
	// split interval
	MachineRegister *reg = lti->get_Register()->to_MachineRegister();

	lti->split(pos, jd->get_StackSlotManager());
	LivetimeInterval *tmp_lti = lti->get_next();
	if (tmp_lti) {
		LOG2("altered lti: " << lti << nl);
		if (!tmp_lti->is_in_Register()) {
			// StackSlot
			LivetimeInterval *stack_lti = tmp_lti;
			unhandled.push(stack_lti);
			// get slot
			ManagedStackSlot *slot = stack_lti->get_ManagedStackSlot();
			assert(slot);
			// spill
			MachineInstruction *move_to_stack = backend->create_Move(reg,slot);
			MIS->add_before(unsigned(pos/2),move_to_stack);
			LOG2("spill instruction: " << move_to_stack << " pos " << unsigned(pos/2) << nl);

			LOG2("stack interval: " << stack_lti << nl);
			LivetimeInterval *new_lti = stack_lti->get_next();
			if (new_lti) {
				// we a new lti
				LOG2("new lti: " << new_lti << nl);
				// load
				MachineInstruction *move_from_stack =
					backend->create_Move(slot,new_lti->get_Register());
				LOG2("load instruction: " << move_from_stack << " pos " << unsigned(new_lti->get_start() / 2) << nl);
				MIS->add_before(unsigned(new_lti->get_start()/2),move_from_stack);
				new_lti->add_def(new_lti->get_start(),move_from_stack->get_result());
				unhandled.push(new_lti);
			}
		} else {
			LOG2("new lti (no stack): " << tmp_lti << nl);
			unhandled.push(tmp_lti);
		}
		// TODO is this really enough? dont we need more loads?
		// register new def (load)

	}
#endif
}

#endif

namespace {
/**
 * @Cpp11 use std::function
 */
struct FreeUntilCompare: public std::binary_function<MachineOperand*,MachineOperand*,bool> {
	bool operator()(MachineOperand *lhs, MachineOperand *rhs) {
		bool r = lhs->aquivalence_less(*rhs);
		//LOG2("FreeUntilCompare: " << *lhs << " aquivalence_less " << *rhs << "=" << r << nl);
		return r;
	}
};

} // end anonymous namespace

typedef std::map<MachineOperand*,MIIterator,FreeUntilCompare> FreeUntilMap;

namespace {

#if 0
MachineResource get_MachineResource_from_MachineRegister(const MachineRegister* reg) {
	return reg->get_MachineResource();
}
MachineRegister* get_MachineRegister_from_MachineResource(const MachineResource& res, Type::TypeID type) {
	return res.create_MachineRegister(type);
}
#endif

/**
 * @Cpp11 use std::function
 */
struct InitFreeUntilMap: public std::unary_function<MachineOperand*,void> {
	FreeUntilMap &free_until_pos;
	MIIterator end;
	/// Constructor
	InitFreeUntilMap(FreeUntilMap &free_until_pos,
		MIIterator end) : free_until_pos(free_until_pos), end(end) {}

	void operator()(MachineOperand *MO) {
		free_until_pos.insert(std::make_pair(MO,end));
	}
};


/**
 * @Cpp11 use std::function
 */
struct SetActive: public std::unary_function<LivetimeInterval&,void> {
	FreeUntilMap &free_until_pos;
	MIIterator start;
	/// Constructor
	SetActive(FreeUntilMap &free_until_pos,
		MIIterator start) : free_until_pos(free_until_pos), start(start) {}

	void operator()(LivetimeInterval& lti) {
		MachineOperand *MO = lti.get_operand();
		LOG2("SetActive: " << lti << " operand: " << *MO << nl);
		FreeUntilMap::iterator i = free_until_pos.find(MO);
		if (i != free_until_pos.end()) {
			LOG("set to zero" << nl);
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
	MIIterator pos;
	MIIterator end;
	/// Constructor
	SetIntersection(FreeUntilMap &free_until_pos,
		LivetimeInterval &current, MIIterator pos, MIIterator end)
		: free_until_pos(free_until_pos), current(current), pos(pos), end(end) {}

	void operator()(LivetimeInterval& lti) {
		MachineOperand *MO = lti.get_operand();
		FreeUntilMap::iterator i = free_until_pos.find(MO);
		if (i != free_until_pos.end()) {
			i->second = next_intersection(lti,current,pos,end);
		}
	}
};


/**
 * @Cpp11 use std::function
 */
struct FreeUntilMaxCompare
		: public std::binary_function<const FreeUntilMap::value_type&, const FreeUntilMap::value_type&,bool> {
	bool operator()(const FreeUntilMap::value_type &lhs, const FreeUntilMap::value_type &rhs) {
		return lhs.second < rhs.second;
	}
};

} // end anonymous namespace

inline bool LinearScanAllocatorPass::try_allocate_free(LivetimeInterval &current, MIIterator pos) {
	// set free until pos of all physical operands to maximum
	FreeUntilMap free_until_pos;
	OperandFile op_file;
	backend->get_OperandFile(op_file,current.get_operand());
	// set to end
	std::for_each(op_file.begin(),op_file.end(),InitFreeUntilMap(free_until_pos, MIS->mi_end()));

	// for each interval in active set free until pos to zero
	std::for_each(active.begin(), active.end(), SetActive(free_until_pos, MIS->mi_begin()));

	// for each interval in inactive set free until pos to next intersection
	std::for_each(active.begin(), active.end(), SetIntersection(free_until_pos, current,pos,MIS->mi_end()));

	for (FreeUntilMap::iterator i = free_until_pos.begin(), e = free_until_pos.end(); i != e; ++i) {
		LOG((i->first) << ": " << i->second << nl);
	}

	// XXX hints!
	// get operand with highest free until pos
	FreeUntilMap::value_type x = *std::max_element(free_until_pos.begin(), free_until_pos.end(), FreeUntilMaxCompare());
	LOG2("reg: " << x.first << " pos: " << x.second << nl);

	if (x.second == MIS->mi_begin()) {
		// no register available without spilling
		return false;
	}
	if (current.back().end.get_iterator() < x.second) {
		// register available for the whole interval
		current.set_operand(x.first);
		return true;
	}
	// register available for the first part of the interval
	ABORT_MSG("Not yet implemented","splitting intervals not yet implemented");
	return true;
}
#if 0
	RegisterFile* reg_file = backend->get_RegisterFile(current->get_type());
	assert(reg_file);
	Type::TypeID type = current->get_type();

	std::map<MachineResource,unsigned> free_until_pos;
	LOG2(BoldMagenta << "try_allocate_free_reg (current=" << current << ")" << reset_color << nl);

	// set all registers to free
	for(RegisterFile::const_iterator i = reg_file->begin(), e = reg_file->end();
			i != e ; ++i) {
		MachineResource res = *i;
		free_until_pos[res] = UINT_MAX;
		//LOG3("res free_until_pos[" << res << "] = UINT_MAX " << UINT_MAX << nl);
	}
	// set active registes to occupied
	for (ActiveSetTy::const_iterator i = active.begin(), e = active.end();
			i != e ; ++i) {
		if (!(*i)->is_in_Register()) continue;
		MachineRegister *reg = (*i)->get_Register()->to_MachineRegister();
		// only consider valid regs
		if (!reg_file->contains(reg)) continue;
		// all active intervals must be assigned to machine registers
		if (DEBUG_COND && !reg) {
			ABORT_MSG("Interval " << current << " not in a machine reg " << (*i)->get_Register(), "Not yet supported!");
		}
		assert(reg);
		// reg not free!
		free_until_pos[get_MachineResource_from_MachineRegister(reg)] = 0;
		LOG3("active " << *i << " free_until_pos[" << reg << "] = 0" << nl);
	}
	// all intervals in inactive intersection with current
	for (InactiveSetTy::const_iterator i = inactive.begin(), e = inactive.end();
			i != e ; ++i) {
		LivetimeInterval *inact = *i;
		assert(inact);
		if (!inact->is_in_Register()) continue;
		signed intersection = current->intersects(*inact);
		if (intersection != -1) {
			LOG2("interval " << current << " intersects with " << inact << " (inactive) at " << intersection << nl);
			MachineRegister *reg = inact->get_Register()->to_MachineRegister();
			assert(reg);
			// only consider valid regs
			if (!reg_file->contains(reg)) continue;
			// reg not free!
			free_until_pos[get_MachineResource_from_MachineRegister(reg)] = intersection;
		}

	}
	// selected register
	MachineRegister *reg = NULL;
	unsigned free_pos;
	// new scope
	{
		// check register hint
		Register *hint = current->get_hint();
		LivetimeInterval *hint_lti;
		if (hint && (hint_lti = LA->get(hint)) ) {
			LOG2("Hint: " << hint_lti << nl);
			reg = hint_lti->get_Register()->to_MachineRegister();
			assert(reg);
			// only consider valid regs
			if (reg_file->contains(reg)) {
				free_pos = free_until_pos[get_MachineResource_from_MachineRegister(reg)];
				// free_pos = 0 indicates that either reg is occupied or is
				// generally not available for assignment (not in regfile)
				if (current->get_start() <= hint_lti->get_end() && free_pos != 0) {
					LOG2(" Followed!" << nl);
					STATISTICS(num_hints_followed++);
				} else {
					LOG2("Not followed!" << nl);
					reg = NULL;
				}
			} else {
				LOG2("Not followed! (invalid hint!)" << nl);
				reg = NULL;
			}
		}
	}
	if (!reg) {
		// get the register with the maximum free until number
		std::map<MachineResource,unsigned>::const_iterator i = std::max_element(free_until_pos.begin(),
			free_until_pos.end(), max_value_comparator<MachineResource, unsigned>);
		assert(i != free_until_pos.end());
		reg = get_MachineRegister_from_MachineResource(i->first,type);
		free_pos = i->second;
		assert(reg);
	}

	LOG("Selected Register: " << reg << " (free until: " << free_pos << ")" << nl);
	if ( free_pos == 0) {
		// no register available without spilling
		return false;
	}
	if ( current->get_end() <= free_pos ) {
		// register available for the whole interval. jackpot!
		current->set_Register(reg);
		return true;
	}
	// register available for the first part of the interval
	current->set_Register(reg);
	// split current before free_pos
	LOG2("split: " << current << " at " << free_pos << nl);
	split(current, free_pos);

	return true;
}
#endif

inline bool LinearScanAllocatorPass::allocate_blocked(LivetimeInterval &current) {
	return false;
}
#if 0
	RegisterFile* reg_file = backend->get_RegisterFile(current->get_type());
	assert(reg_file);

	std::map<MachineResource,unsigned> next_use_pos;
	LOG2(BoldCyan << "allocate_blocked_reg (current=" << current << ")" << reset_color << nl);

	// Remember best interval
	LivetimeInterval *best_lti = NULL;
	MachineRegister *best_reg = NULL;
	signed best_next_use_pos = -1;

	// set all registers to free
	for(RegisterFile::const_iterator i = reg_file->begin(), e = reg_file->end();
			i != e ; ++i) {
		MachineResource res = *i;
		next_use_pos[res] = UINT_MAX;
	}
	// for each interval in active
	for (ActiveSetTy::const_iterator i = active.begin(), e = active.end();
			i != e ; ++i) {
		LivetimeInterval *lti = *i;
		if (!lti->is_in_Register()) continue;
		MachineRegister *reg = lti->get_Register()->to_MachineRegister();
		assert(reg);
		// only consider valid regs
		if (!reg_file->contains(reg)) continue;
		signed pos = lti->next_usedef_after(current->get_start());
		next_use_pos[get_MachineResource_from_MachineRegister(reg)] = pos;
		LOG3("active: " << lti << " pos: " << pos << nl);
		if (pos > best_next_use_pos && !lti->is_fixed_interval()) {
			best_next_use_pos = pos;
			best_reg = reg;
			best_lti = lti;
		}
	}
	// for each interval in inactive intersecting with current
	for (InactiveSetTy::const_iterator i = inactive.begin(), e = inactive.end();
			i != e ; ++i) {
		LivetimeInterval *inact = *i;
		if (!inact->is_in_Register()) continue;
		assert(inact);
		signed intersection = current->intersects(*inact);
		if (intersection != -1) {
			LivetimeInterval *lti = *i;
			MachineRegister *reg = lti->get_Register()->to_MachineRegister();
			assert(reg);
			// only consider valid regs
			if (!reg_file->contains(reg)) continue;
			signed pos = lti->next_usedef_after(current->get_start());
			next_use_pos[get_MachineResource_from_MachineRegister(reg)] = pos;
			LOG3("inactive: " << lti << " pos: " << pos << nl);
			if (pos > best_next_use_pos && !lti->is_fixed_interval()) {
				best_next_use_pos = pos;
				best_reg = reg;
				best_lti = lti;
			}
		}

	}

	// get the register with the highest next use position
	#if 0
	std::map<MachineResource,unsigned>::const_iterator i = std::max_element(next_use_pos.begin(),
		next_use_pos.end(), max_value_comparator<MachineRegister*, unsigned>);
	assert(i != next_use_pos.end());
	MachineRegister *reg = i->first;
	unsigned use_pos = i->second;
	#endif
	MachineRegister *reg = best_reg;
	signed use_pos = best_next_use_pos;
	LivetimeInterval *lti = best_lti;

	assert(lti);

	LOG("Selected Register: " << reg << " (free until: " << use_pos << ")" << nl);
	signed current_usepos = current->next_usedef_after(current->get_start());
	assert(current_usepos != -1);
	LOG("current usepos: " << current_usepos << nl);
	if (current_usepos > use_pos) {
		// all other intervals are used before current
		// so it is best to spill current itself
		LOG2("all other intervals are used before current so it is best to spill current itself" << nl);
		// TODO assign spillslot to current
		// TODO spill current before its first use position that requires a register (??)
		ABORT_MSG("not implemented","");
	} else {
		// spill intervals that currently block reg
		LOG2("spill intervals that currently block reg" << nl);
		current->set_Register(reg);
		active.push_back(current);
		// TODO split active interval for reg at position (position is start of current)
		// stack slot
		LOG2("split " << lti << " at " << current->get_start() << nl);

		split(lti,current->get_start());

		// TODO split an inactive interval for reg at the end of its lifetime hole
		for (InactiveSetTy::const_iterator i = inactive.begin(), e = inactive.end();
				i != e ; ++i) {
			LivetimeInterval *inact = *i;
			assert(inact);
			if (inact->get_Register() == reg) {
				assert(lti != inact);
				LOG2("split " << inact << " at end of livetime hole" << nl);
				split(inact,current->get_start());
				//ABORT_MSG("split at end of livetime hole","not implemented");
			}
		}
	}

	// make sure that current does not intersect with the fixed interval for reg
	LOG2("make sure that current does not intersect with the fixed interval for reg" << nl);
	// TODO if current intersects with the fixed interval for reg then split current before this intersection
	return true;
}

namespace {

template <class _T, typename _V>
struct push_lambda {
	_T& obj;
	push_lambda(_T &obj) : obj(obj) {}
	void operator()(_V x) {
		obj.push(x);
	}
};


#if 0
/// @Cpp11 use std::function instead
struct _lti_same_operand
		: public std::binary_function<LivetimeInterval*,LivetimeInterval*,bool> {
	bool operator()(LivetimeInterval *lhs, LivetimeInterval *rhs) const {
		return lhs->get_Register() == rhs->get_Register() ||
			lhs->get_ManagedStackSlot() == rhs->get_ManagedStackSlot();
	}
} lti_same_operand;
#endif

} //end anonymous namespace

void LinearScanAllocatorPass::split_blocking_ltis(LivetimeInterval* current) {
	MachineRegister *reg = current->get_Register()->to_MachineRegister();
	assert(reg);
	// spill intervals that currently block reg
	LOG2("spill intervals that currently block reg" << nl);
	LOG2("current " << current << " " << (void*) current->get_Register() <<  nl);
	// split active
	for (ActiveSetTy::const_iterator i = active.begin(), e = active.end();
			i != e ; ++i) {
		LivetimeInterval *act = *i;
		assert(act);
		if (!act->is_in_Register()) continue;
		LOG2("active " << act << " " << (void *) act->get_Register() << nl);
		if (act->get_Register() && *act->get_Register()->to_MachineRegister() == reg) {
			assert(current != act);
			LOG2("split " << act << " at " << current->get_start() << nl);
			split(act,current->get_start());
		}
	}

	// split inactive
	for (InactiveSetTy::const_iterator i = inactive.begin(), e = inactive.end();
			i != e ; ++i) {
		LivetimeInterval *inact = *i;
		assert(inact);
		if (!inact->is_in_Register()) continue;
		LOG2("inactive " << inact << nl);
		if (inact->get_Register() && *inact->get_Register()->to_MachineRegister() == reg) {
			assert(current != inact);
			LOG2("split " << inact << " at end of livetime hole" << nl);
			split(inact,current->get_start());
		}
	}
}

#endif

void LinearScanAllocatorPass::initialize() {
	while (!unhandled.empty()) {
		unhandled.pop();
	}
	handled.clear();
	inactive.clear();
	active.clear();
}

namespace {
#if 0
/**
 * @Cpp11 use std::function
 */
class CheckActive : public std::unary_function<LivetimeInterval&,void> {
private:
	MIIterator position;
public:
	/// Constructor
	CheckActive() {}
	void operator()(LivetimeInterval& lti) {
	}
};
#endif
} // end anonymous namespace

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
	while(!unhandled.empty()) {
		LivetimeInterval current = unhandled.top();
		unhandled.pop();
		LOG("current: " << current << nl);
		MIIterator pos = current.front().start.get_iterator();

		// check for intervals in active that are handled or inactive
		LOG2("check for intervals in active that are handled or inactive" << nl);
		for (ActiveSetTy::iterator i = active.begin(), e = active.end();
				i != e ; /* ++i */) {
			LivetimeInterval act = *i;
			switch (act.get_State(pos)) {
			case LivetimeInterval::Handled:
				LOG2("LTI " << act << " moved from active to handled" << nl);
				handled.push_back(act);
				i = active.erase(i);
				break;
			case LivetimeInterval::Inactive:
				LOG2("LTI " << act << " moved from active to inactive" << nl);
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
				LOG2("LTI " << inact << " moved from inactive to handled" << nl);
				handled.push_back(inact);
				i = inactive.erase(i);
				break;
			case LivetimeInterval::Active:
				LOG2("LTI " << inact << " moved from inactive to active" << nl);
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
			}
		}
		// add current to active
		active.push_back(current);
	}
#if 0

		if (!current->is_in_Register()) {
			// stackslot
			active.push_back(current);
			LOG2("Stackslot: " << current << nl);
			continue;
		}
		if (current->get_Register()->to_MachineRegister()) {
			// preallocated
			LOG2("Preallocated: " << current << nl);
			split_blocking_ltis(current);
			active.push_back(current);
			continue;
		}
	}

	// move all active/inactive to handled
	handled.insert(handled.end(),inactive.begin(),inactive.end());
	handled.insert(handled.end(),active.begin(),active.end());

	// set src/dst operands
	for (HandledSetTy::const_iterator i = handled.begin(), e = handled.end();
			i != e ; ++i) {
		LivetimeInterval *lti = *i;
		if (!lti->is_in_Register()) continue;
		Register *reg = lti->get_Register();
		assert(reg->to_MachineRegister());
		for (LivetimeInterval::const_def_iterator i = lti->def_begin(),
				e = lti->def_end(); i != e ; ++i) {
			MachineOperandDesc *MOD = i->second;
			MOD->op = lti->get_Register();
		}
		for (LivetimeInterval::const_use_iterator i = lti->use_begin(),
				e = lti->use_end(); i != e ; ++i) {
			MachineOperandDesc *MOD = i->second;
			MOD->op = lti->get_Register();
		}
	}

	// debug
	for (LivetimeAnalysisPass::const_iterator i = LA->begin(), e = LA->end();
			i != e ; ++i) {
		const LivetimeInterval *lti = &i->second;
		LOG("Assigned Interval " << lti << nl );
		assert(!lti->is_in_Register() || lti->get_Register()->to_MachineRegister());
		while ( (lti = lti->get_next()) ) {
			LOG("Split Interval " << lti << nl );
			assert(!lti->is_in_Register() || lti->get_Register()->to_MachineRegister());
		}

	}
	DEBUG(LA->print(dbg()));


	#endif
	resolve();

	for (LivetimeAnalysisPass::const_iterator i = LA->begin(), e = LA->end();
			i != e ; ++i) {
		const LivetimeInterval *lti = &i->second;
		LOG("Assigned Interval " << lti << " (init: " << *lti->get_init_operand() << ")" <<nl );
	}
	return true;
}

typedef std::map<MachineBasicBlock*, std::list<LivetimeInterval> > BBtoLTI_Map;

namespace {
#if 0
struct ClassRegPtrComp {
	bool operator() (Register* lhs, Register* rhs) const {
		if (lhs->operator==(rhs)) {
			return false;
		}
		return lhs < rhs;
	}
};
#endif
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
			if (lti.get_State(MBB->mi_first()) == LivetimeInterval::Active) {
				bb2lti_map[MBB].push_back(lti);
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
struct Move {
	MachineOperand *from;
	MachineOperand *to;
	Move* dep;
	bool scheduled;
	Move(MachineOperand *from, MachineOperand *to) : from(from), to(to), dep(NULL), scheduled(false) {}
	bool is_scheduled() const { return scheduled; }
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

typedef std::list<Move> MoveMapTy;

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
		MachineOperand *move_to = lti.get_operand();
		if (move_from != move_to) {
			move_map.push_back(Move(move_from,move_to));
		}
	}
};

struct MachineOperandCmp : public std::binary_function<MachineOperand*,MachineOperand*,bool> {
	bool operator()(MachineOperand *lhs, MachineOperand *rhs) const {
		return lhs->aquivalence_less(*rhs);
	}
};


inline OStream& operator<<(OStream &OS, const Move &move) {
	return OS << "move from " << *move.from << " to " << *move.to;
}

void schedule(std::list<Move*> &scheduled, MoveMapTy &move_map, std::list<Move*> &stack) {
	Move* node = stack.back();
	// already scheduled?
	if (node->is_scheduled()) {
		stack.pop_back();
		return;
	}
	if (node->dep && !node->dep->is_scheduled()) {
		if (std::find(stack.begin(),stack.end(),node->dep) == stack.end()) {
			LOG2("cycle detected!" << nl);
			MachineOperand *tmp = new VirtualRegister(node->to->get_type());
			move_map.push_back(Move(tmp, node->to));
			node->to = tmp;
			node->dep = NULL;
		} else {
			stack.push_back(node->dep);
			schedule(scheduled, move_map, stack);
		}
	}
	LOG2("schedule: " << *node << nl);
	stack.pop_back();
	scheduled.push_back(node);
	node->scheduled = true;
}

void order_and_insert_move(MachineBasicBlock *predecessor, MachineBasicBlock *successor,
		MoveMapTy &move_map) {

	if (move_map.empty()) return;
	// calculate data dependency graph (DDG)
	std::map<MachineOperand*,Move*,MachineOperandCmp> read_from;

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
	std::list<Move*> scheduled;
	std::list<Move*> stack;

	for (MoveMapTy::iterator i = move_map.begin(), e = move_map.end(); i != e; ++i) {
		stack.push_back(&*i);
		schedule(scheduled, move_map, stack);
		assert(stack.empty());
	}

}

class ForEachCfgEdge : public std::binary_function<MachineBasicBlock*,MachineBasicBlock*,void> {
private:
	BBtoLTI_Map &bb2lti_map;
	LivetimeAnalysisPass *LA;

public:
	/// constructor
	ForEachCfgEdge(BBtoLTI_Map &bb2lti_map, LivetimeAnalysisPass *LA)
		: bb2lti_map(bb2lti_map), LA(LA) {}
	/// function call operator
	void operator()(MachineBasicBlock *predecessor, MachineBasicBlock *successor) const {
		MoveMapTy move_map;
		LOG2("edge " << *predecessor << " -> " << *successor << nl);
		BBtoLTI_Map::mapped_type &lti_live_set = bb2lti_map[successor];
		// for each live interval live at successor
		std::for_each(lti_live_set.begin(), lti_live_set.end(),
			ForEachLiveiterval(predecessor,successor, LA, move_map));
		// order and insert move
		order_and_insert_move(predecessor, successor, move_map);
	}
};

} // end anonymous namespace

void LinearScanAllocatorPass::resolve() {

	// calculate basicblock to live interval map
	BBtoLTI_Map bb2lti_map;
	std::for_each(MIS->begin(), MIS->end(), CalcBBtoLTIMap(LA,bb2lti_map));
	// for each cfg edge from predecessor to successor (implemented in ForEachCfgEdge)
	std::for_each(MIS->begin(), MIS->end(), CfgEdge(ForEachCfgEdge(bb2lti_map,LA)));
#if 0
	LoweringPass *LP = get_Pass<LoweringPass>();
	ListSchedulingPass *IS = get_Pass<ListSchedulingPass>();
	GlobalSchedule *GS = get_Pass<ScheduleClickPass>();

	// resolution
	Method *M = jd->get_Method();

	// fill live in information
	// TODO this can be done more efficient I guess
	push_lambda<UnhandledSetTy,LivetimeInterval*> push_me(unhandled);
	// FIXME this can be handled with <functional> or C++11
	LOG("all handled" << nl);
	DEBUG(print_container(dbg(),handled.begin(),handled.end()) << nl);
	std::for_each(handled.begin(),handled.end(),push_me);
	active.clear();
	inactive.clear();
	LOG2("size of unhandled: " << unhandled.size() << nl);

	std::map<BeginInst*,std::set<LivetimeInterval*> > active_map_begin;
	std::map<BeginInst*,std::set<LivetimeInterval*> > active_map_end;

	for (Method::const_bb_iterator i = M->bb_begin(), e = M->bb_end();
			i != e ; ++i) {
		BeginInst *BI = *i;
		unsigned bb_begin = MIS->get_range(BI).first * 2;
		unsigned bb_end = MIS->get_range(BI).second * 2 - 1;
		LOG2(BoldGreen << "BI " << BI << "(" << bb_begin <<"-" << bb_end<< "): " << reset_color << nl);
		unsigned pos = bb_begin;
		for(int i = 0; i < 2; ++i) {
			// check for intervals in active that are handled or inactive
			for (ActiveSetTy::iterator ii = active.begin(), e = active.end();
					ii != e ; /* ++ii */) {
				LivetimeInterval *act = *ii;
				LOG3("active LTI " << act << nl);
				if (act->is_handled(pos)) {
					LOG3("act->hddld " << act << nl);
					// add to handled
					//handled.push_back(act);
					// remove from active
					ii = active.erase(ii);
				} else if(act->is_inactive(pos)) {
					LOG3("act->inact " << act << nl);
					// add to inactive
					inactive.push_back(act);
					// remove from active
					ii = active.erase(ii);
				} else {
					// NOTE: erase returns the next element so we can not increment
					// in the loop header!
					++ii;
				}
			}
			// check for intervals in inactive that are handled or active
			for (InactiveSetTy::iterator ii = inactive.begin(), e = inactive.end();
					ii != e ; /* ++ii */) {
				LivetimeInterval *inact = *ii;
				LOG3("inactive LTI " << inact << nl);
				if (inact->is_handled(pos)) {
					LOG3("inact->hndld " << inact << nl);
					// add to handled
					//handled.push_back(inact);
					// remove from inactive
					ii = inactive.erase(ii);
				} else if(inact->is_active(pos)) {
					LOG3("inact->act " << inact << nl);
					// add to active
					active.push_back(inact);
					// remove from inactive
					ii = inactive.erase(ii);
				} else {
					// NOTE: erase returns the next element so we can not increment
					// in the loop header!
					++i;
				}
			}
			// check unhandled
			while(!unhandled.empty()) {
				LivetimeInterval *current = unhandled.top();
				if (current->is_unhandled(pos)) {
					break;
				}
				unhandled.pop();
				if (current->is_handled(pos)) {
					// handled
					LOG3("unhndld->hndld " << current << nl);
					continue;
				}
				if(current->is_inactive(pos)) {
					// inactive
					LOG3("unhndld->inactive " << current << nl);
					inactive.push_back(current);
				} else {
					// active
					LOG3("unhndld->active " << current << nl);
					active.push_back(current);
				}

			}
			// all in active are now live
			if ( i == 0) {
				LOG2("active at begin of " << BI << "(" << bb_begin <<"-" << bb_end << "): " << nl);
				LOG2("");
				DEBUG2(print_container(dbg(),active.begin(),active.end()) << nl);
				active_map_begin[BI].insert(active.begin(),active.end());
			} else {
				LOG2("active at end of " << BI << "(" << bb_begin <<"-" << bb_end << "): " << nl);
				LOG2("");
				DEBUG2(print_container(dbg(),active.begin(),active.end()) << nl);
				active_map_end[BI].insert(active.begin(),active.end());
			}
			pos = bb_end;
		}
	}

	MoveMapTy move_map;
	// for all basic blocks
	for (Method::const_bb_iterator i = M->bb_begin(), e = M->bb_end();
			i != e ; ++i) {

		BeginInst *pred = *i;
		EndInst *pred_end = pred->get_EndInst();
		for (EndInst::SuccessorListTy::const_iterator i = pred_end->succ_begin(),
				e = pred_end->succ_end(); i != e ; ++i) {
			// for each edge pred -> succ
			BeginInst *succ = i->get();
			unsigned bb_start;
			//unsigned bb_end;
			{
				MachineInstructionSchedule::MachineInstructionRangeTy succ_range = MIS->get_range(succ);
				bb_start = succ_range.first * 2;
				//bb_end = succ_range.second * 2;
			}
			LOG2(BoldGreen << "Edge: " << pred << " -> " << succ << reset_color << nl);
			std::set<LivetimeInterval*> &ltis_succ = active_map_begin[succ];
			std::set<LivetimeInterval*> &ltis_pred = active_map_end[pred];
			for (std::set<LivetimeInterval*>::const_iterator i = ltis_succ.begin(),
					e = ltis_succ.end(); i != e ; ++i) {
				// for each live interval lti active at the start of succ
				LivetimeInterval* lti_succ = *i;
				for (std::set<LivetimeInterval*>::const_iterator i = ltis_pred.begin(),
						e = ltis_pred.end(); i != e ; ++i) {
					// for each live interval lti active at the end of pred
					LivetimeInterval* lti_pred = *i;
					if (lti_succ->is_split_of(*lti_pred)){
						LOG2("interval " << lti_succ << " is a spilt of " << lti_pred << nl);
						MachineOperand* op_pred = lti_pred->is_in_Register()
							? (MachineOperand*) lti_pred->get_Register()
							: (MachineOperand*) lti_pred->get_ManagedStackSlot();
						MachineOperand* op_succ = lti_succ->is_in_Register()
							? (MachineOperand*) lti_succ->get_Register()
							: (MachineOperand*) lti_succ->get_ManagedStackSlot();
						if (op_pred != op_succ) {
							MachineInstruction* move = backend->create_Move(op_pred,op_succ);
							move_map[std::make_pair(pred,succ)].push_back(move);
						}

					}
				}
				// resolve PHIs!
				if (lti_succ->get_start() == bb_start) {
					LOG(BoldCyan << "WE HAVE A WINNER! " << reset_color << lti_succ << nl);
					// XXX should this even happen?
					if (lti_succ->is_in_StackSlot()) continue;
					assert(lti_succ->def_size() == 1);
					MachineInstruction *MI = lti_succ->def_front().second->get_MachineInstruction();
					LOG3("PHI: " << MI << nl);
					assert( MI->is_phi());
					LoweredInstDAG *use_dag = MI->get_parent();
					PHIInst *phi = use_dag->get_Instruction()->to_PHIInst();
					assert(phi);
					signed index = succ->get_predecessor_index(pred);
					assert(index != -1);
					Instruction *I = phi->get_operand(index)->to_Instruction();
					assert(I);
					LoweredInstDAG *def_dag = LP->get_LoweredInstDAG(I);
					assert(def_dag);
					MachineOperand *pred_op = def_dag->get_result()->get_result().op;
					//MachineOperand *succ_op = MI->get(index).op;
					assert(pred_op == MI->get(index).op);
					MachineOperand *result_op = MI->get_result().op;
					LOG("result is in: " << pred_op << " and should be in " << result_op << nl);
					assert(pred_op->is_Register());
					assert(result_op->is_Register());
					assert(pred_op->to_Register()->to_MachineRegister());
					assert(result_op->to_Register()->to_MachineRegister());
					//if (pred_op->to_Register()->to_MachineRegister() ==
					//		result_op->to_Register()->to_MachineRegister()) {
					if (result_op != pred_op) {
						LOG("adding move " << pred_op << " to " << result_op << nl);
						MachineInstruction* move = backend->create_Move(pred_op,result_op);
						move_map[std::make_pair(pred,succ)].push_back(move);
					}
				}
			}
		}
	}
	for (MoveMapTy::const_iterator i = move_map.begin(), e = move_map.end();
			i != e; ++i) {
		const MoveListTy &mlist = i->second;
		BeginInst *pred = i->first.first;
		BeginInst *succ = i->first.second;

		LOG("Edge " << pred << " -> " << succ << nl);
		// get block for the move
		BeginInst* BI = M->get_edge_block(pred,succ);
		EndInst* EI = BI->get_EndInst();
		ContainerInst *I = new ContainerInst(BI);
		M->add_Instruction(I);
		LoweredInstDAG *dag = new LoweredInstDAG(I);

		// remember "destroyed" registers
		std::set<Register*,ClassRegPtrComp> destroyed;
		MachineInstruction *move = NULL;
		LOG2("create moves" << nl);
		for (MoveListTy::const_iterator i = mlist.begin(), e = mlist.end();
				i != e ; ++i ) {
			LOG2(nl);
			move = *i;
			MachineOperand *src = move->get(0).op;
			MachineOperand *dst = move->get_result().op;
			Register *reg = dst->to_Register();
			if (reg) {
				LOG2("destroyed: " << reg << " " << (void*) reg << nl);

				destroyed.insert(reg);
			}
			LOG2("used:      " << src->to_Register() << " " << (void*) src->to_Register() << nl);
			if (destroyed.find(src->to_Register()) != destroyed.end()) {
				// the value has been overwritten -> stackslot
				if (dst->is_ManagedStackSlot()) {
					// dst is a stack slot
					LOG("MOVE (to stackslot)" << move << nl);
					dag->add_front(move);
				} else {
					assert(dst->is_Register());
					ManagedStackSlot *slot = jd->get_StackSlotManager()->create_ManagedStackSlot(src->get_type());
					MachineInstruction *spill = backend->create_Move(src,slot);
					dag->add_front(spill);
					move->set_operand(0,slot);
					LOG2("spill needed: " << spill << nl);
					LOG("MOVE2 " << move << nl);
					dag->add(move);
				}
			} else {
				LOG("MOVE " << move << nl);
				dag->add(move);
			}
		}
		// mark last move as result
		assert(move);
		dag->set_result(move);
		LP->set_dag(BI,backend->lower(BI));
		LP->set_dag(I,dag);
		LP->set_dag(EI,backend->lower(EI));
		// we have tho fix GlobalSchedule
		// TODO
		GS->add_Instruction(BI,BI);
		GS->add_Instruction(I,BI);
		GS->add_Instruction(EI,BI);
		// we have fix InstructionScheduling
		IS->schedule(BI);
		// we have to fix BasicBlockScheduling
		// -> rerun the pass
	}
	#if 0
	ABORT_MSG("Values not in the same store (" << op_pred
		<< " vs " << op_succ << ")" , "move not yet implemented");
	#endif


	// write back the spill/store instructions
	MIS->insert_added_instruction();
#endif
}

bool LinearScanAllocatorPass::verify() const {
#if 0
	for (MachineInstructionSchedule::const_iterator i = MIS->begin(),
			e = MIS->end(); i != e; ++i) {
		MachineInstruction *MI = *i;
		MachineOperand *op = MI->get_result().op;
		if (!op->is_StackSlot() &&
				op->is_Register() && !op->to_Register()->to_MachineRegister() ) {
			LOG("Not allocatd: " << MI << nl);
			return false;
		}

	}
#endif
	return true;
}


// pass usage
PassUsage& LinearScanAllocatorPass::get_PassUsage(PassUsage &PU) const {
	// requires
	PU.add_requires(LivetimeAnalysisPass::ID);
	PU.add_requires(MachineInstructionSchedulingPass::ID);
	PU.add_requires(ListSchedulingPass::ID);
	PU.add_requires(ScheduleClickPass::ID);
	// modified
	PU.add_modifies(ListSchedulingPass::ID);
	PU.add_modifies(ScheduleClickPass::ID);
	// destroys
	PU.add_destroys(BasicBlockSchedulingPass::ID);
	PU.add_destroys(MachineInstructionSchedulingPass::ID);
	// not yet updated correctly (might be only changed)
	PU.add_destroys(LivetimeAnalysisPass::ID);
	return PU;
}

// the address of this variable is used to identify the pass
char LinearScanAllocatorPass::ID = 0;

// register pass
static PassRegistery<LinearScanAllocatorPass> X("LinearScanAllocatorPass");


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
