## src/vm/jit/compiler2/Makefile.am
##
## Copyright (C) 2013
## CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
##
## This file is part of CACAO.
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License as
## published by the Free Software Foundation; either version 2, or (at
## your option) any later version.
##
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
## 02110-1301, USA.


AM_CPPFLAGS += -I$(top_srcdir)/src \
	-I$(top_srcdir)/src/vm/jit/$(ARCH_DIR) \
	-I$(top_srcdir)/src/vm/jit/$(ARCH_DIR)/$(OS_DIR) \
	-I$(top_srcdir)/src/vm/jit/compiler2/$(ARCH_DIR) \
	-I$(top_builddir)/src

LIBS =

SUBDIRS = \
	alloc \
	memory \
	$(ARCH_DIR)

noinst_LTLIBRARIES = \
	libcompiler2.la

libcompiler2_la_SOURCES = \
	Backend.cpp \
	BasicBlockSchedulingPass.cpp \
	CFGConstructionPass.cpp \
	CFGMetaPass.cpp \
	CodeGenPass.cpp \
	CodeMemory.cpp \
	CodeSegment.cpp \
	Compiler.cpp \
	Conditional.cpp \
	DisassemblerPass.cpp \
	DataSegment.cpp \
	DominatorPass.cpp \
	DomTreePrinterPass.cpp \
	ExamplePass.cpp \
	GraphHelper.cpp \
	ICMDPrinterPass.cpp \
	Instruction.cpp \
	InstructionMetaPass.cpp \
	InstructionVisitor.cpp \
	JITData.cpp \
	LinearScanAllocatorPass.cpp \
	LivetimeAnalysisPass.cpp \
	LivetimeInterval.cpp \
	Loop.cpp \
	LoopTreePrinterPass.cpp \
	LoopPass.cpp \
	LoopSimplificationPass.cpp \
	Matcher.cpp \
	MachineBasicBlock.cpp \
	MachineInstruction.cpp \
	MachineInstructions.cpp \
	MachineInstructionSchedule.cpp \
	MachineInstructionSchedulingPass.cpp \
	MachineInstructionPrinterPass.cpp \
	MachineLoop.cpp \
	MachineLoopPass.cpp \
	MachineOperand.cpp \
	MethodC2.cpp \
	MethodDescriptor.cpp \
	ObjectFileWriterPass.cpp \
	ParserPass.cpp \
	Pass.cpp \
	PassDependencyGraphPrinter.cpp \
	PassManager.cpp \
	RegisterAllocatorPass.cpp \
	ScheduleClickPass.cpp \
	ScheduleEarlyPass.cpp \
	ScheduleLatePass.cpp \
	Segment.cpp \
	SSAConstructionPass.cpp \
	SSAPrinterPass.cpp \
	StackAnalysisPass.cpp \
	StackSlotManager.cpp \
	Type.cpp \
	Value.cpp \
	VerifierPass.cpp \
	DeadCodeEliminationPass.cpp \
	ConstantPropagationPass.cpp \
	GlobalValueNumberingPass.cpp

libcompiler2_la_LIBADD = \
	alloc/liballoc.la \
	memory/libmemory.la \
	$(ARCH_DIR)/lib$(ARCH_DIR).la

## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End: