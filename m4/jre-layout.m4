dnl m4/jre-layout.m4
dnl
dnl Copyright (C) 2007 R. Grafl, A. Krall, C. Kruegel,
dnl C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
dnl E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
dnl J. Wenninger, Institut f. Computersprachen - TU Wien
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
dnl 
dnl $Id: jre-layout.m4 8373 2007-08-20 22:43:09Z twisti $


dnl if we compile for a JRE-style directory layout

AC_DEFUN([AC_CHECK_WITH_JRE_LAYOUT],[
AC_MSG_CHECKING(if we compile for a JRE-style directory layout)
AC_ARG_WITH([jre-layout],
            [AS_HELP_STRING(--with-jre-layout,compile for JRE-style directory layout [[default=no]])],
            [case "${enableval}" in
                yes)
                    WITH_JRE_LAYOUT=yes
                    AC_DEFINE([WITH_JRE_LAYOUT], 1, [with JRE layout])
                    ;;
                *)
                    WITH_JRE_LAYOUT=no
                    ;;
             esac],
            [WITH_JRE_LAYOUT=no])
AC_MSG_RESULT(${WITH_JRE_LAYOUT})
])
