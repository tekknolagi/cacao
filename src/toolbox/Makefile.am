## src/toolbox/Makefile.am
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


AM_CPPFLAGS += -I$(top_srcdir)/src -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR) -I$(top_srcdir)/src/vm/jit/$(ARCH_DIR)/$(OS_DIR)

LIBS =

noinst_LTLIBRARIES = \
	libtoolbox.la

libtoolbox_la_SOURCES = \
	assert.hpp \
	avl.cpp \
	avl.hpp \
	bitvector.cpp \
	bitvector.hpp \
	buffer.hpp \
	endianess.hpp \
	GraphPrinter.hpp \
	hashtable.cpp \
	hashtable.hpp \
	intern_table.hpp \
	list.hpp \
	logging.cpp \
	logging.hpp \
	Option.hpp \
	Option.cpp \
	ordered_list.hpp \
	set.hpp \
	set.cpp \
	UnionFind.hpp \
	utf_utils.hpp \
	utf8_transform.inc \
	utf16_transform.inc \
	util.cpp \
	util.hpp \
	worklist.cpp \
	worklist.hpp \
	Debug.hpp \
	Debug.cpp \
	OStream.cpp \
	OStream.hpp


## Local variables:
## mode: Makefile
## indent-tabs-mode: t
## c-basic-offset: 4
## tab-width: 8
## compile-command: "automake --add-missing"
## End:
