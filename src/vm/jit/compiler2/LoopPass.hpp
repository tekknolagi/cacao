/* src/vm/jit/compiler2/LoopPass.hpp - LoopPass

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

#ifndef _JIT_COMPILER2_LOOPPASS
#define _JIT_COMPILER2_LOOPPASS

#include "vm/jit/compiler2/Pass.hpp"
#include "vm/jit/compiler2/Loop.hpp"
#include "vm/jit/compiler2/JITData.hpp"

#include <set>
#include <map>
#include <vector>

namespace cacao {
namespace jit {
namespace compiler2 {

/**
 * Calculate the Loop Tree.
 *
 * The algorithm used here is based on the method proposed in
 * "Testing Flow Graph Reducibility" by Tarjan @cite Tarjan1974
 * with the modifications in "SSA-Based Reduction of Operator Strengh" by
 * Vick @cite VickMScThesis. See also Click's Phd Thesis, Chapter 6
 * @cite ClickPHD.
 */
template <class _T>
class LoopPassBase : public Pass , public LoopTreeBase<_T> {
private:
	typedef _T NodeType;
	typedef std::set<const NodeType *> NodeListTy;
	typedef std::map<const NodeType *,NodeListTy> NodeListMapTy;
	typedef std::vector<const NodeType *> NodeMapTy;
	typedef std::map<const NodeType *,int> IndexMapTy;
	typedef std::map<const NodeType *,const NodeType *> EdgeMapTy;

	#if 0
	EdgeMapTy parent;
	NodeListMapTy pred;
	#endif
	NodeListMapTy bucket;
	IndexMapTy semi;
	NodeMapTy vertex;
	int n;

	EdgeMapTy ancestor;
	EdgeMapTy label;

	NodeListTy& succ(const NodeType *v, NodeListTy &list);
	void DFS(const NodeType * v);

	void Link(const NodeType *v, const NodeType *w);
	const NodeType* Eval(const NodeType *v);
	void Compress(const NodeType *v);
public:
	static char ID;
	LoopPassBase() : Pass() {}
	bool run(JITData &JD);
};

typedef LoopPassBase<BeginInst> LoopPass;

} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_LOOPPASS */


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
