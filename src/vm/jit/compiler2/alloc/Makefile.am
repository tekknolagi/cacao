## src/vm/jit/compiler2/alloc/Makefile.am
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

noinst_LTLIBRARIES = \
	liballoc.la

liballoc_la_SOURCES = \
	Allocator.cpp \
	Allocator.hpp \
	deque.hpp \
	list.hpp \
	map.hpp \
	ordered_list.hpp \
	queue.hpp \
	set.hpp \
	stack.hpp \
	unordered_map.hpp \
	unordered_set.hpp \
	vector.hpp

## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
