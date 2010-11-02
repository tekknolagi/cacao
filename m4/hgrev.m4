dnl m4/hgrev.m4
dnl
dnl Copyright (C) 2010
dnl CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
dnl 
dnl This file is part of CACAO.
dnl 
dnl This program is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU General Public License as
dnl published by the Free Software Foundation; either version 2, or (at
dnl your option) any later version.
dnl 
dnl This program is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
dnl 02110-1301, USA.


dnl Mercurial revision

AC_DEFUN([AC_CHECK_WITH_HGREV],[
AC_MSG_CHECKING(for the Mercurial revision)
AC_ARG_WITH([hg-revision],
            [AS_HELP_STRING(--with-hg-revision=<hash>, the Mercurial revision used for this CACAO build)],
            [CACAO_HGREV=${withval}])
AC_MSG_RESULT(${CACAO_HGREV-not specified})
AC_DEFINE_UNQUOTED([CACAO_HGREV], "${CACAO_HGREV}", [CACAO's Mercurial revision])
AC_SUBST(CACAO_HGREV)
])
