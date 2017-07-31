/** classes/cacao/org/cacaojvm/compiler2/test/Compiler2Test - cacao compiler2 test driver
 *
 * Copyright (C) 1996-2014
 * CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
 *
 * This file is part of CACAO.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

package org.cacaojvm.compiler2.test;

class Compiler2Test {
	private static native void compileMethod(boolean baseline,
			Class<?> compileClass, String methodName, String methodDesc);
	
	protected static native Object executeMethod(Class<?> compileClass, 
			String methodName, String methodDesc, Object[] args) throws Throwable;

	protected static void compileBaseline(Class<?> compileClass,
			String methodName, String methodDesc) {
		compileMethod(true, compileClass, methodName, methodDesc);
	}

	protected static void compileCompiler2(Class<?> compileClass,
			String methodName, String methodDesc) {
		compileMethod(false, compileClass, methodName, methodDesc);
	}

}
