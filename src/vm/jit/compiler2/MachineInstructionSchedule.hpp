/* src/vm/jit/compiler2/MachineInstructionSchedule.hpp - MachineInstructionSchedule

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

#ifndef _JIT_COMPILER2_MACHINEINSTRUCTIONSCHEDULE
#define _JIT_COMPILER2_MACHINEINSTRUCTIONSCHEDULE

#include "toolbox/ordered_list.hpp"
#include "toolbox/future.hpp"

#include <map>
#include <list>
#include <vector>
#include <cassert>

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declarations
class MachineBasicBlock;
class MachineInstructionSchedule;

class MBBIterator {
	typedef ordered_list<MachineBasicBlock*>::iterator iterator;
	MachineInstructionSchedule *parent;
	iterator it;
	/// empty constructor
	MBBIterator() {}
public:
	typedef iterator::reference reference;
	typedef iterator::pointer pointer;

	MBBIterator(MachineInstructionSchedule *parent, const iterator &it)
		: parent(parent), it(it) {}
	MBBIterator(const MBBIterator& other) : parent(other.parent), it(other.it) {}
	MBBIterator& operator++() {
		++it;
		return *this;
	}
	MachineInstructionSchedule* get_parent() const { return parent; }
	MBBIterator operator++(int) {
		MBBIterator tmp(*this);
		operator++();
		return tmp;
	}
	MBBIterator& operator--() {
		--it;
		return *this;
	}
	MBBIterator operator--(int) {
		MBBIterator tmp(*this);
		operator--();
		return tmp;
	}
	bool operator==(const MBBIterator& rhs) const {
		assert(parent == rhs.parent);
		return it == rhs.it;
	}
	bool operator<( const MBBIterator& rhs) const {
		assert(parent == rhs.parent);
		return it < rhs.it;
	}
	bool operator!=(const MBBIterator& rhs) const { return !(*this == rhs); }
	bool operator>( const MBBIterator& rhs) const { return rhs < *this; }
	reference       operator*()        { return *it; }
	const reference operator*()  const { return *it; }
	pointer         operator->()       { return &*it; }
	const pointer   operator->() const { return &*it; }

	friend class MachineInstructionSchedule;
	friend class MachineBasicBlock;
};

class MBBBuilder {
private:
	MachineBasicBlock *MBB;
public:
	MBBBuilder();
	friend class MachineInstructionSchedule;
};

class MachineInstructionSchedule {
public:
	typedef MBBIterator iterator;
	/// construct an empty MachineInstructionSchedule
	MachineInstructionSchedule() {};
	/// returns the number of elements
	std::size_t size() const;
	/// Appends the given element value to the end of the container.
	MachineInstructionSchedule::iterator push_back(const MBBBuilder& value);
	/// inserts value to the beginning
	MachineInstructionSchedule::iterator push_front(const MBBBuilder& value);
	/// inserts value before the element pointed to by pos
	MachineInstructionSchedule::iterator insert_before(iterator pos, const MBBBuilder& value);
	/// inserts value after the element pointed to by pos
	MachineInstructionSchedule::iterator insert_after(iterator pos, const MBBBuilder& value);
	/// returns an iterator to the beginning
	iterator begin();
	/// returns an iterator to the end
	iterator end();
private:
	ordered_list<MachineBasicBlock*> list;
};

inline std::size_t MachineInstructionSchedule::size() const {
	return list.size();
}
inline MachineInstructionSchedule::iterator MachineInstructionSchedule::insert_after(iterator pos, const MBBBuilder& value) {
	return insert_before(++pos,value);
}
inline MachineInstructionSchedule::iterator MachineInstructionSchedule::begin() {
	return iterator(this,list.begin());
}
inline MachineInstructionSchedule::iterator MachineInstructionSchedule::end() {
	return iterator(this,list.end());
}

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_MACHINEINSTRUCTIONSCHEDULE */


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
