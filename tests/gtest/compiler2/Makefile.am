## tests/gtest/compiler2/Makefile.am
##
## Copyright (C) 1996-2013
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

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR) -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR)/$(OS_DIR) -I$(top_builddir)

AM_DEFAULT_SOURCE_EXT = .cpp

# ignore gtest warnings
GTEST_CXXFLAGS += -Wno-variadic-macros -Wno-sign-compare

AM_CPPFLAGS += $(GTEST_CPPFLAGS)
AM_CXXFLAGS += $(GTEST_CXXFLAGS)
AM_LDFLAGS   = $(GTEST_LDFLAGS)
LIBS        += $(GTEST_LIBS)

LDADD=$(top_builddir)/tests/gtest/libgtest_main.a
LDADD+=$(top_builddir)/src/cacao/libjvm.la

TESTS = MachineBasicBlock_test MachineInstructionSchedule_test
check_PROGRAMS = $(TESTS)

## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
