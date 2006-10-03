/* src/vm/jit/jit.c - calls the code generation functions

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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

   Contact: cacao@cacaojvm.org

   Authors: Andreas Krall
            Reinhard Grafl

   Changes: Edwin Steiner
            Christian Thalinger
            Christian Ullrich

   $Id: jit.c 5655 2006-10-03 20:44:46Z edwin $

*/


#include "config.h"
#include "vm/types.h"

#include <assert.h>

#include "mm/memory.h"
#include "native/native.h"
#include "toolbox/logging.h"

#if defined(ENABLE_THREADS)
# include "threads/native/lock.h"
#else
# include "threads/none/lock.h"
#endif

#include "vm/class.h"
#include "vm/global.h"
#include "vm/initialize.h"
#include "vm/loader.h"
#include "vm/method.h"
#include "vm/options.h"
#include "vm/statistics.h"
#include "vm/jit/asmpart.h"

# include "vm/jit/cfg.h"

#include "vm/jit/codegen-common.h"
#include "vm/jit/disass.h"
#include "vm/jit/dseg.h"
#include "vm/jit/jit.h"
#include "vm/jit/parse.h"
#include "vm/jit/reg.h"

# include "vm/jit/reorder.h"

#include "vm/jit/show.h"
#include "vm/jit/stack.h"

#include "vm/jit/allocator/simplereg.h"
#if defined(ENABLE_LSRA) && !defined(ENABLE_SSA)
# include "vm/jit/allocator/lsra.h"
#endif
#if defined(ENABLE_SSA)
# include "vm/jit/optimizing/lsra.h"
# include "vm/jit/optimizing/ssa.h"
#endif


#if defined(ENABLE_IFCONV)
# include "vm/jit/ifconv/ifconv.h"
#endif

#include "vm/jit/loop/analyze.h"
#include "vm/jit/loop/graph.h"
#include "vm/jit/loop/loop.h"
#include "vm/jit/verify/typecheck.h"
#include "vm/rt-timing.h"

#if defined(ENABLE_THREADS)
# include "threads/native/threads.h"
#endif


/* debug macros ***************************************************************/

#if !defined(NDEBUG)
#define DEBUG_JIT_COMPILEVERBOSE(x) \
    do { \
        if (compileverbose) { \
            log_message_method(x, m); \
        } \
    } while (0)
#else
#define DEBUG_JIT_COMPILEVERBOSE(x)    /* nothing */
#endif

 
/* global switches ************************************************************/

int stackreq[256] = {
	0,    /* JAVA_NOP                         0 */
	1,    /* JAVA_ACONST                      1 */
	1,    /* JAVA_ICONST_M1                   2 */
	1,    /* JAVA_ICONST_0                    3 */
	1,    /* JAVA_ICONST_1                    4 */
	1,    /* JAVA_ICONST_2                    5 */
	1,    /* JAVA_ICONST_3                    6 */
	1,    /* JAVA_ICONST_4                    7 */
	1,    /* JAVA_ICONST_5                    8 */
	1,    /* JAVA_LCONST_0                    9 */
	1,    /* JAVA_LCONST_1                   10 */
	1,    /* JAVA_FCONST_0                   11 */
	1,    /* JAVA_FCONST_1                   12 */
	1,    /* JAVA_FCONST_2                   13 */
	1,    /* JAVA_DCONST_0                   14 */
	1,    /* JAVA_DCONST_1                   15 */
	1,    /* JAVA_BIPUSH                     16 */
	1,    /* JAVA_SIPUSH                     17 */
	1,    /* JAVA_LDC                        18 */
	1,    /* JAVA_LDC_W                      19 */
	1,    /* JAVA_LDC2_W                     20 */
	1,    /* JAVA_ILOAD                      21 */
	1,    /* JAVA_LLOAD                      22 */
	1,    /* JAVA_FLOAD                      23 */
	1,    /* JAVA_DLOAD                      24 */
	1,    /* JAVA_ALOAD                      25 */
	1,    /* JAVA_ILOAD_0                    26 */
	1,    /* JAVA_ILOAD_1                    27 */
	1,    /* JAVA_ILOAD_2                    28 */
	1,    /* JAVA_ILOAD_3                    29 */
	1,    /* JAVA_LLOAD_0                    30 */
	1,    /* JAVA_LLOAD_1                    31 */
	1,    /* JAVA_LLOAD_2                    32 */
	1,    /* JAVA_LLOAD_3                    33 */
	1,    /* JAVA_FLOAD_0                    34 */
	1,    /* JAVA_FLOAD_1                    35 */
	1,    /* JAVA_FLOAD_2                    36 */
	1,    /* JAVA_FLOAD_3                    37 */
	1,    /* JAVA_DLOAD_0                    38 */
	1,    /* JAVA_DLOAD_1                    39 */
	1,    /* JAVA_DLOAD_2                    40 */
	1,    /* JAVA_DLOAD_3                    41 */
	1,    /* JAVA_ALOAD_0                    42 */
	1,    /* JAVA_ALOAD_1                    43 */
	1,    /* JAVA_ALOAD_2                    44 */
	1,    /* JAVA_ALOAD_3                    45 */
	1,    /* JAVA_IALOAD                     46 */
	1,    /* JAVA_LALOAD                     47 */
	1,    /* JAVA_FALOAD                     48 */
	1,    /* JAVA_DALOAD                     49 */
	1,    /* JAVA_AALOAD                     50 */
	1,    /* JAVA_BALOAD                     51 */
	1,    /* JAVA_CALOAD                     52 */
	1,    /* JAVA_SALOAD                     53 */
	0,    /* JAVA_ISTORE                     54 */
	0,    /* JAVA_LSTORE                     55 */
	0,    /* JAVA_FSTORE                     56 */
	0,    /* JAVA_DSTORE                     57 */
	0,    /* JAVA_ASTORE                     58 */
	0,    /* JAVA_ISTORE_0                   59 */
	0,    /* JAVA_ISTORE_1                   60 */
	0,    /* JAVA_ISTORE_2                   61 */
	0,    /* JAVA_ISTORE_3                   62 */
	0,    /* JAVA_LSTORE_0                   63 */
	0,    /* JAVA_LSTORE_1                   64 */
	0,    /* JAVA_LSTORE_2                   65 */
	0,    /* JAVA_LSTORE_3                   66 */
	0,    /* JAVA_FSTORE_0                   67 */
	0,    /* JAVA_FSTORE_1                   68 */
	0,    /* JAVA_FSTORE_2                   69 */
	0,    /* JAVA_FSTORE_3                   70 */
	0,    /* JAVA_DSTORE_0                   71 */
	0,    /* JAVA_DSTORE_1                   72 */
	0,    /* JAVA_DSTORE_2                   73 */
	0,    /* JAVA_DSTORE_3                   74 */
	0,    /* JAVA_ASTORE_0                   75 */
	0,    /* JAVA_ASTORE_1                   76 */
	0,    /* JAVA_ASTORE_2                   77 */
	0,    /* JAVA_ASTORE_3                   78 */
	0,    /* JAVA_IASTORE                    79 */
	0,    /* JAVA_LASTORE                    80 */
	0,    /* JAVA_FASTORE                    81 */
	0,    /* JAVA_DASTORE                    82 */
	0,    /* JAVA_AASTORE                    83 */
	0,    /* JAVA_BASTORE                    84 */
	0,    /* JAVA_CASTORE                    85 */
	0,    /* JAVA_SASTORE                    86 */
	0,    /* JAVA_POP                        87 */
	0,    /* JAVA_POP2                       88 */
	1,    /* JAVA_DUP                        89 */
	3,    /* JAVA_DUP_X1                     90 */
	4,    /* JAVA_DUP_X2                     91 */
	2,    /* JAVA_DUP2                       92 */
	3,    /* JAVA_DUP2_X1                    93 */
	4,    /* JAVA_DUP2_X2                    94 */
	2,    /* JAVA_SWAP                       95 */
	1,    /* JAVA_IADD                       96 */
	1,    /* JAVA_LADD                       97 */
	1,    /* JAVA_FADD                       98 */
	1,    /* JAVA_DADD                       99 */
	1,    /* JAVA_ISUB                      100 */
	1,    /* JAVA_LSUB                      101 */
	1,    /* JAVA_FSUB                      102 */
	1,    /* JAVA_DSUB                      103 */
	1,    /* JAVA_IMUL                      104 */
	1,    /* JAVA_LMUL                      105 */
	1,    /* JAVA_FMUL                      106 */
	1,    /* JAVA_DMUL                      107 */
	1,    /* JAVA_IDIV                      108 */
	1,    /* JAVA_LDIV                      109 */
	1,    /* JAVA_FDIV                      110 */
	1,    /* JAVA_DDIV                      111 */
	1,    /* JAVA_IREM                      112 */
	1,    /* JAVA_LREM                      113 */
	1,    /* JAVA_FREM                      114 */
	1,    /* JAVA_DREM                      115 */
	1,    /* JAVA_INEG                      116 */
	1,    /* JAVA_LNEG                      117 */
	1,    /* JAVA_FNEG                      118 */
	1,    /* JAVA_DNEG                      119 */
	1,    /* JAVA_ISHL                      120 */
	1,    /* JAVA_LSHL                      121 */
	1,    /* JAVA_ISHR                      122 */
	1,    /* JAVA_LSHR                      123 */
	1,    /* JAVA_IUSHR                     124 */
	1,    /* JAVA_LUSHR                     125 */
	1,    /* JAVA_IAND                      126 */
	1,    /* JAVA_LAND                      127 */
	1,    /* JAVA_IOR                       128 */
	1,    /* JAVA_LOR                       129 */
	1,    /* JAVA_IXOR                      130 */
	1,    /* JAVA_LXOR                      131 */
	0,    /* JAVA_IINC                      132 */
	1,    /* JAVA_I2L                       133 */
	1,    /* JAVA_I2F                       134 */
	1,    /* JAVA_I2D                       135 */
	1,    /* JAVA_L2I                       136 */
	1,    /* JAVA_L2F                       137 */
	1,    /* JAVA_L2D                       138 */
	1,    /* JAVA_F2I                       139 */
	1,    /* JAVA_F2L                       140 */
	1,    /* JAVA_F2D                       141 */
	1,    /* JAVA_D2I                       142 */
	1,    /* JAVA_D2L                       143 */
	1,    /* JAVA_D2F                       144 */
	1,    /* JAVA_INT2BYTE                  145 */
	1,    /* JAVA_INT2CHAR                  146 */
	1,    /* JAVA_INT2SHORT                 147 */
	1,    /* JAVA_LCMP                      148 */
	1,    /* JAVA_FCMPL                     149 */
	1,    /* JAVA_FCMPG                     150 */
	1,    /* JAVA_DCMPL                     151 */
	1,    /* JAVA_DCMPG                     152 */
	0,    /* JAVA_IFEQ                      153 */
	0,    /* JAVA_IFNE                      154 */
	0,    /* JAVA_IFLT                      155 */
	0,    /* JAVA_IFGE                      156 */
	0,    /* JAVA_IFGT                      157 */
	0,    /* JAVA_IFLE                      158 */
	0,    /* JAVA_IF_ICMPEQ                 159 */
	0,    /* JAVA_IF_ICMPNE                 160 */
	0,    /* JAVA_IF_ICMPLT                 161 */
	0,    /* JAVA_IF_ICMPGE                 162 */
	0,    /* JAVA_IF_ICMPGT                 163 */
	0,    /* JAVA_IF_ICMPLE                 164 */
	0,    /* JAVA_IF_ACMPEQ                 165 */
	0,    /* JAVA_IF_ACMPNE                 166 */
	0,    /* JAVA_GOTO                      167 */
	1,    /* JAVA_JSR                       168 */
	0,    /* JAVA_RET                       169 */
	0,    /* JAVA_TABLESWITCH               170 */
	0,    /* JAVA_LOOKUPSWITCH              171 */
	0,    /* JAVA_IRETURN                   172 */
	0,    /* JAVA_LRETURN                   173 */
	0,    /* JAVA_FRETURN                   174 */
	0,    /* JAVA_DRETURN                   175 */
	0,    /* JAVA_ARETURN                   176 */
	0,    /* JAVA_RETURN                    177 */
	1,    /* JAVA_GETSTATIC                 178 */
	0,    /* JAVA_PUTSTATIC                 179 */
	1,    /* JAVA_GETFIELD                  180 */
	0,    /* JAVA_PUTFIELD                  181 */
	1,    /* JAVA_INVOKEVIRTUAL             182 */
	1,    /* JAVA_INVOKESPECIAL             183 */
	1,    /* JAVA_INVOKESTATIC              184 */
	1,    /* JAVA_INVOKEINTERFACE           185 */
	1,    /* JAVA_UNDEF186                  186 */
	1,    /* JAVA_NEW                       187 */
	1,    /* JAVA_NEWARRAY                  188 */
	1,    /* JAVA_ANEWARRAY                 189 */
	1,    /* JAVA_ARRAYLENGTH               190 */
	1,    /* JAVA_ATHROW                    191 */
	1,    /* JAVA_CHECKCAST                 192 */
	1,    /* JAVA_INSTANCEOF                193 */
	0,    /* JAVA_MONITORENTER              194 */
	0,    /* JAVA_MONITOREXIT               195 */
	0,    /* JAVA_WIDE                      196 */
	1,    /* JAVA_MULTIANEWARRAY            197 */
	0,    /* JAVA_IFNULL                    198 */
	0,    /* JAVA_IFNONNULL                 199 */
	0,    /* JAVA_GOTO_W                    200 */
	1,    /* JAVA_JSR_W                     201 */
	0,    /* JAVA_BREAKPOINT                202 */
	1,    /* JAVA_UNDEF203                  203 */
	1,    /* JAVA_UNDEF204                  204 */
	1,    /* JAVA_UNDEF205                  205 */
	1,    /* JAVA_UNDEF206                  206 */
	1,    /* JAVA_UNDEF207                  207 */
	1,    /* JAVA_UNDEF208                  208 */
	1,    /* JAVA_UNDEF209                  209 */
	1,    /* JAVA_UNDEF210                  210 */
	1,    /* JAVA_UNDEF211                  211 */
	1,    /* JAVA_UNDEF212                  212 */
	1,    /* JAVA_UNDEF213                  213 */
	1,    /* JAVA_UNDEF214                  214 */
	1,    /* JAVA_UNDEF215                  215 */
	1,    /* JAVA_UNDEF216                  216 */
	1,    /* JAVA_UNDEF217                  217 */
	1,    /* JAVA_UNDEF218                  218 */
	1,    /* JAVA_UNDEF219                  219 */
	1,    /* JAVA_UNDEF220                  220 */
	1,    /* JAVA_UNDEF221                  221 */
	1,    /* JAVA_UNDEF222                  222 */
	1,    /* JAVA_UNDEF223                  223 */
	1,    /* JAVA_UNDEF224                  224 */
	1,    /* JAVA_UNDEF225                  225 */
	1,    /* JAVA_UNDEF226                  226 */
	1,    /* JAVA_UNDEF227                  227 */
	1,    /* JAVA_UNDEF228                  228 */
	1,    /* JAVA_UNDEF229                  229 */
	1,    /* JAVA_UNDEF230                  230 */
	1,    /* JAVA_UNDEF231                  231 */
	1,    /* JAVA_UNDEF232                  232 */
	1,    /* JAVA_UNDEF233                  233 */
	1,    /* JAVA_UNDEF234                  234 */
	1,    /* JAVA_UNDEF235                  235 */
	1,    /* JAVA_UNDEF236                  236 */
	1,    /* JAVA_UNDEF237                  237 */
	1,    /* JAVA_UNDEF238                  238 */
	1,    /* JAVA_UNDEF239                  239 */
	1,    /* JAVA_UNDEF240                  240 */
	1,    /* JAVA_UNDEF241                  241 */
	1,    /* JAVA_UNDEF242                  242 */
	1,    /* JAVA_UNDEF243                  243 */
	1,    /* JAVA_UNDEF244                  244 */
	1,    /* JAVA_UNDEF245                  245 */
	1,    /* JAVA_UNDEF246                  246 */
	1,    /* JAVA_UNDEF247                  247 */
	1,    /* JAVA_UNDEF248                  248 */
	1,    /* JAVA_UNDEF249                  249 */
	1,    /* JAVA_UNDEF250                  250 */
	1,    /* JAVA_UNDEF251                  251 */
	1,    /* JAVA_UNDEF252                  252 */
	1,    /* JAVA_UNDEF253                  253 */
	1,    /* JAVA_UNDEF254                  254 */
	1,    /* JAVA_UNDEF255                  255 */
};

                                
int jcommandsize[256] = {

	1,    /* JAVA_NOP                         0 */
	1,    /* JAVA_ACONST_NULL                 1 */
	1,    /* JAVA_ICONST_M1                   2 */
	1,    /* JAVA_ICONST_0                    3 */
	1,    /* JAVA_ICONST_1                    4 */
	1,    /* JAVA_ICONST_2                    5 */
	1,    /* JAVA_ICONST_3                    6 */
	1,    /* JAVA_ICONST_4                    7 */
	1,    /* JAVA_ICONST_5                    8 */
	1,    /* JAVA_LCONST_0                    9 */
	1,    /* JAVA_LCONST_1                   10 */
	1,    /* JAVA_FCONST_0                   11 */
	1,    /* JAVA_FCONST_1                   12 */
	1,    /* JAVA_FCONST_2                   13 */
	1,    /* JAVA_DCONST_0                   14 */
	1,    /* JAVA_DCONST_1                   15 */
	2,    /* JAVA_BIPUSH                     16 */
	3,    /* JAVA_SIPUSH                     17 */
	2,    /* JAVA_LDC1                       18 */
	3,    /* JAVA_LDC2                       19 */
	3,    /* JAVA_LDC2W                      20 */
	2,    /* JAVA_ILOAD                      21 */
	2,    /* JAVA_LLOAD                      22 */
	2,    /* JAVA_FLOAD                      23 */
	2,    /* JAVA_DLOAD                      24 */
	2,    /* JAVA_ALOAD                      25 */
	1,    /* JAVA_ILOAD_0                    26 */
	1,    /* JAVA_ILOAD_1                    27 */
	1,    /* JAVA_ILOAD_2                    28 */
	1,    /* JAVA_ILOAD_3                    29 */
	1,    /* JAVA_LLOAD_0                    30 */
	1,    /* JAVA_LLOAD_1                    31 */
	1,    /* JAVA_LLOAD_2                    32 */
	1,    /* JAVA_LLOAD_3                    33 */
	1,    /* JAVA_FLOAD_0                    34 */
	1,    /* JAVA_FLOAD_1                    35 */
	1,    /* JAVA_FLOAD_2                    36 */
	1,    /* JAVA_FLOAD_3                    37 */
	1,    /* JAVA_DLOAD_0                    38 */
	1,    /* JAVA_DLOAD_1                    39 */
	1,    /* JAVA_DLOAD_2                    40 */
	1,    /* JAVA_DLOAD_3                    41 */
	1,    /* JAVA_ALOAD_0                    42 */
	1,    /* JAVA_ALOAD_1                    43 */
	1,    /* JAVA_ALOAD_2                    44 */
	1,    /* JAVA_ALOAD_3                    45 */
	1,    /* JAVA_IALOAD                     46 */
	1,    /* JAVA_LALOAD                     47 */
	1,    /* JAVA_FALOAD                     48 */
	1,    /* JAVA_DALOAD                     49 */
	1,    /* JAVA_AALOAD                     50 */
	1,    /* JAVA_BALOAD                     51 */
	1,    /* JAVA_CALOAD                     52 */
	1,    /* JAVA_SALOAD                     53 */
	2,    /* JAVA_ISTORE                     54 */
	2,    /* JAVA_LSTORE                     55 */
	2,    /* JAVA_FSTORE                     56 */
	2,    /* JAVA_DSTORE                     57 */
	2,    /* JAVA_ASTORE                     58 */
	1,    /* JAVA_ISTORE_0                   59 */
	1,    /* JAVA_ISTORE_1                   60 */
	1,    /* JAVA_ISTORE_2                   61 */
	1,    /* JAVA_ISTORE_3                   62 */
	1,    /* JAVA_LSTORE_0                   63 */
	1,    /* JAVA_LSTORE_1                   64 */
	1,    /* JAVA_LSTORE_2                   65 */
	1,    /* JAVA_LSTORE_3                   66 */
	1,    /* JAVA_FSTORE_0                   67 */
	1,    /* JAVA_FSTORE_1                   68 */
	1,    /* JAVA_FSTORE_2                   69 */
	1,    /* JAVA_FSTORE_3                   70 */
	1,    /* JAVA_DSTORE_0                   71 */
	1,    /* JAVA_DSTORE_1                   72 */
	1,    /* JAVA_DSTORE_2                   73 */
	1,    /* JAVA_DSTORE_3                   74 */
	1,    /* JAVA_ASTORE_0                   75 */
	1,    /* JAVA_ASTORE_1                   76 */
	1,    /* JAVA_ASTORE_2                   77 */
	1,    /* JAVA_ASTORE_3                   78 */
	1,    /* JAVA_IASTORE                    79 */
	1,    /* JAVA_LASTORE                    80 */
	1,    /* JAVA_FASTORE                    81 */
	1,    /* JAVA_DASTORE                    82 */
	1,    /* JAVA_AASTORE                    83 */
	1,    /* JAVA_BASTORE                    84 */
	1,    /* JAVA_CASTORE                    85 */
	1,    /* JAVA_SASTORE                    86 */
	1,    /* JAVA_POP                        87 */
	1,    /* JAVA_POP2                       88 */
	1,    /* JAVA_DUP                        89 */
	1,    /* JAVA_DUP_X1                     90 */
	1,    /* JAVA_DUP_X2                     91 */
	1,    /* JAVA_DUP2                       92 */
	1,    /* JAVA_DUP2_X1                    93 */
	1,    /* JAVA_DUP2_X2                    94 */
	1,    /* JAVA_SWAP                       95 */
	1,    /* JAVA_IADD                       96 */
	1,    /* JAVA_LADD                       97 */
	1,    /* JAVA_FADD                       98 */
	1,    /* JAVA_DADD                       99 */
	1,    /* JAVA_ISUB                      100 */
	1,    /* JAVA_LSUB                      101 */
	1,    /* JAVA_FSUB                      102 */
	1,    /* JAVA_DSUB                      103 */
	1,    /* JAVA_IMUL                      104 */
	1,    /* JAVA_LMUL                      105 */
	1,    /* JAVA_FMUL                      106 */
	1,    /* JAVA_DMUL                      107 */
	1,    /* JAVA_IDIV                      108 */
	1,    /* JAVA_LDIV                      109 */
	1,    /* JAVA_FDIV                      110 */
	1,    /* JAVA_DDIV                      111 */
	1,    /* JAVA_IREM                      112 */
	1,    /* JAVA_LREM                      113 */
	1,    /* JAVA_FREM                      114 */
	1,    /* JAVA_DREM                      115 */
	1,    /* JAVA_INEG                      116 */
	1,    /* JAVA_LNEG                      117 */
	1,    /* JAVA_FNEG                      118 */
	1,    /* JAVA_DNEG                      119 */
	1,    /* JAVA_ISHL                      120 */
	1,    /* JAVA_LSHL                      121 */
	1,    /* JAVA_ISHR                      122 */
	1,    /* JAVA_LSHR                      123 */
	1,    /* JAVA_IUSHR                     124 */
	1,    /* JAVA_LUSHR                     125 */
	1,    /* JAVA_IAND                      126 */
	1,    /* JAVA_LAND                      127 */
	1,    /* JAVA_IOR                       128 */
	1,    /* JAVA_LOR                       129 */
	1,    /* JAVA_IXOR                      130 */
	1,    /* JAVA_LXOR                      131 */
	3,    /* JAVA_IINC                      132 */
	1,    /* JAVA_I2L                       133 */
	1,    /* JAVA_I2F                       134 */
	1,    /* JAVA_I2D                       135 */
	1,    /* JAVA_L2I                       136 */
	1,    /* JAVA_L2F                       137 */
	1,    /* JAVA_L2D                       138 */
	1,    /* JAVA_F2I                       139 */
	1,    /* JAVA_F2L                       140 */
	1,    /* JAVA_F2D                       141 */
	1,    /* JAVA_D2I                       142 */
	1,    /* JAVA_D2L                       143 */
	1,    /* JAVA_D2F                       144 */
	1,    /* JAVA_INT2BYTE                  145 */
	1,    /* JAVA_INT2CHAR                  146 */
	1,    /* JAVA_INT2SHORT                 147 */
	1,    /* JAVA_LCMP                      148 */
	1,    /* JAVA_FCMPL                     149 */
	1,    /* JAVA_FCMPG                     150 */
	1,    /* JAVA_DCMPL                     151 */
	1,    /* JAVA_DCMPG                     152 */
	3,    /* JAVA_IFEQ                      153 */
	3,    /* JAVA_IFNE                      154 */
	3,    /* JAVA_IFLT                      155 */
	3,    /* JAVA_IFGE                      156 */
	3,    /* JAVA_IFGT                      157 */
	3,    /* JAVA_IFLE                      158 */
	3,    /* JAVA_IF_ICMPEQ                 159 */
	3,    /* JAVA_IF_ICMPNE                 160 */
	3,    /* JAVA_IF_ICMPLT                 161 */
	3,    /* JAVA_IF_ICMPGE                 162 */
	3,    /* JAVA_IF_ICMPGT                 163 */
	3,    /* JAVA_IF_ICMPLE                 164 */
	3,    /* JAVA_IF_ACMPEQ                 165 */
	3,    /* JAVA_IF_ACMPNE                 166 */
	3,    /* JAVA_GOTO                      167 */
	3,    /* JAVA_JSR                       168 */
	2,    /* JAVA_RET                       169 */
	0,    /* JAVA_TABLESWITCH               170 */ /* variable length */
	0,    /* JAVA_LOOKUPSWITCH              171 */ /* variable length */
	1,    /* JAVA_IRETURN                   172 */
	1,    /* JAVA_LRETURN                   173 */
	1,    /* JAVA_FRETURN                   174 */
	1,    /* JAVA_DRETURN                   175 */
	1,    /* JAVA_ARETURN                   176 */
	1,    /* JAVA_RETURN                    177 */
	3,    /* JAVA_GETSTATIC                 178 */
	3,    /* JAVA_PUTSTATIC                 179 */
	3,    /* JAVA_GETFIELD                  180 */
	3,    /* JAVA_PUTFIELD                  181 */
	3,    /* JAVA_INVOKEVIRTUAL             182 */
	3,    /* JAVA_INVOKESPECIAL             183 */
	3,    /* JAVA_INVOKESTATIC              184 */
	5,    /* JAVA_INVOKEINTERFACE           185 */
	1,    /* UNDEF186 */
	3,    /* JAVA_NEW                       187 */
	2,    /* JAVA_NEWARRAY                  188 */
	3,    /* JAVA_ANEWARRAY                 189 */
	1,    /* JAVA_ARRAYLENGTH               190 */
	1,    /* JAVA_ATHROW                    191 */
	3,    /* JAVA_CHECKCAST                 192 */
	3,    /* JAVA_INSTANCEOF                193 */
	1,    /* JAVA_MONITORENTER              194 */
	1,    /* JAVA_MONITOREXIT               195 */
	0,    /* JAVA_WIDE                      196 */ /* variable length */
	4,    /* JAVA_MULTIANEWARRAY            197 */
	3,    /* JAVA_IFNULL                    198 */
	3,    /* JAVA_IFNONNULL                 199 */
	5,    /* JAVA_GOTO_W                    200 */
	5,    /* JAVA_JSR_W                     201 */
	1,    /* JAVA_BREAKPOINT                202 */

	1,    /* UNDEF203 */
	1,
	1,
	1,
	1,
	1,
	1,
	1,    /* UNDEF210 */
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,    /* UNDEF220 */
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,    /* UNDEF230 */
	1,
	1,
	1,
	1,

	/* unused */
	        1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1
};


char *icmd_names[256] = {
	"NOP            ", /*               0 */
	"ACONST         ", /*               1 */
	"CHECKNULL      ", /* ICONST_M1     2 */
	"ICONST         ", /*               3 */
	"CHECKNULL_POP  ", /* ICONST_1      4 */
	"IDIVPOW2       ", /* ICONST_2      5 */
	"LDIVPOW2       ", /* ICONST_3      6 */
	"UNDEF7         ", /* ICONST_4      7 */
	"UNDEF8         ", /* ICONST_5      8 */
	"LCONST         ", /*               9 */
	"LCMPCONST      ", /* LCONST_1     10 */
	"FCONST         ", /*              11 */
	"UNDEF12        ", /* FCONST_1     12 */
	"UNDEF13        ", /* FCONST_2     13 */
	"DCONST         ", /*              14 */
	"COPY           ", /* DCONST_1     15 */
	"MOVE           ", /* BIPUSH       16 */
	"UNDEF17        ", /* SIPUSH       17 */
	"UNDEF18        ", /* LDC1         18 */
	"UNDEF19        ", /* LDC2         19 */
	"UNDEF20        ", /* LDC2W        20 */
	"ILOAD          ", /*              21 */
	"LLOAD          ", /*              22 */
	"FLOAD          ", /*              23 */
	"DLOAD          ", /*              24 */
	"ALOAD          ", /*              25 */
	"IADDCONST      ", /* ILOAD_0      26 */
	"ISUBCONST      ", /* ILOAD_1      27 */
	"IMULCONST      ", /* ILOAD_2      28 */
	"IANDCONST      ", /* ILOAD_3      29 */
	"IORCONST       ", /* LLOAD_0      30 */
	"IXORCONST      ", /* LLOAD_1      31 */
	"ISHLCONST      ", /* LLOAD_2      32 */
	"ISHRCONST      ", /* LLOAD_3      33 */
	"IUSHRCONST     ", /* FLOAD_0      34 */
	"IREMPOW2       ", /* FLOAD_1      35 */
	"LADDCONST      ", /* FLOAD_2      36 */
	"LSUBCONST      ", /* FLOAD_3      37 */
	"LMULCONST      ", /* DLOAD_0      38 */
	"LANDCONST      ", /* DLOAD_1      39 */
	"LORCONST       ", /* DLOAD_2      40 */
	"LXORCONST      ", /* DLOAD_3      41 */
	"LSHLCONST      ", /* ALOAD_0      42 */
	"LSHRCONST      ", /* ALOAD_1      43 */
	"LUSHRCONST     ", /* ALOAD_2      44 */
	"LREMPOW2       ", /* ALOAD_3      45 */
	"IALOAD         ", /*              46 */
	"LALOAD         ", /*              47 */
	"FALOAD         ", /*              48 */
	"DALOAD         ", /*              49 */
	"AALOAD         ", /*              50 */
	"BALOAD         ", /*              51 */
	"CALOAD         ", /*              52 */
	"SALOAD         ", /*              53 */
	"ISTORE         ", /*              54 */
	"LSTORE         ", /*              55 */
	"FSTORE         ", /*              56 */
	"DSTORE         ", /*              57 */
	"ASTORE         ", /*              58 */
	"IF_LEQ         ", /* ISTORE_0     59 */
	"IF_LNE         ", /* ISTORE_1     60 */
	"IF_LLT         ", /* ISTORE_2     61 */
	"IF_LGE         ", /* ISTORE_3     62 */
	"IF_LGT         ", /* LSTORE_0     63 */
	"IF_LLE         ", /* LSTORE_1     64 */
	"IF_LCMPEQ      ", /* LSTORE_2     65 */
	"IF_LCMPNE      ", /* LSTORE_3     66 */
	"IF_LCMPLT      ", /* FSTORE_0     67 */
	"IF_LCMPGE      ", /* FSTORE_1     68 */
	"IF_LCMPGT      ", /* FSTORE_2     69 */
	"IF_LCMPLE      ", /* FSTORE_3     70 */
	"UNDEF71        ", /* DSTORE_0     71 */
	"UNDEF72        ", /* DSTORE_1     72 */
	"UNDEF73        ", /* DSTORE_2     73 */
	"UNDEF74        ", /* DSTORE_3     74 */
	"UNDEF75        ", /* ASTORE_0     75 */
	"UNDEF76        ", /* ASTORE_1     76 */
	"UNDEF77        ", /* ASTORE_2     77 */
	"UNDEF78        ", /* ASTORE_3     78 */
	"IASTORE        ", /*              79 */
	"LASTORE        ", /*              80 */
	"FASTORE        ", /*              81 */
	"DASTORE        ", /*              82 */
	"AASTORE        ", /*              83 */
	"BASTORE        ", /*              84 */
	"CASTORE        ", /*              85 */
	"SASTORE        ", /*              86 */
	"POP            ", /*              87 */
	"POP2           ", /*              88 */
	"DUP            ", /*              89 */
	"DUP_X1         ", /*              90 */
	"DUP_X2         ", /*              91 */
	"DUP2           ", /*              92 */
	"DUP2_X1        ", /*              93 */
	"DUP2_X2        ", /*              94 */
	"SWAP           ", /*              95 */
	"IADD           ", /*              96 */
	"LADD           ", /*              97 */
	"FADD           ", /*              98 */
	"DADD           ", /*              99 */
	"ISUB           ", /*             100 */
	"LSUB           ", /*             101 */
	"FSUB           ", /*             102 */
	"DSUB           ", /*             103 */
	"IMUL           ", /*             104 */
	"LMUL           ", /*             105 */
	"FMUL           ", /*             106 */
	"DMUL           ", /*             107 */
	"IDIV           ", /*             108 */
	"LDIV           ", /*             109 */
	"FDIV           ", /*             110 */
	"DDIV           ", /*             111 */
	"IREM           ", /*             112 */
	"LREM           ", /*             113 */
	"FREM           ", /*             114 */
	"DREM           ", /*             115 */
	"INEG           ", /*             116 */
	"LNEG           ", /*             117 */
	"FNEG           ", /*             118 */
	"DNEG           ", /*             119 */
	"ISHL           ", /*             120 */
	"LSHL           ", /*             121 */
	"ISHR           ", /*             122 */
	"LSHR           ", /*             123 */
	"IUSHR          ", /*             124 */
	"LUSHR          ", /*             125 */
	"IAND           ", /*             126 */
	"LAND           ", /*             127 */
	"IOR            ", /*             128 */
	"LOR            ", /*             129 */
	"IXOR           ", /*             130 */
	"LXOR           ", /*             131 */
	"IINC           ", /*             132 */
	"I2L            ", /*             133 */
	"I2F            ", /*             134 */
	"I2D            ", /*             135 */
	"L2I            ", /*             136 */
	"L2F            ", /*             137 */
	"L2D            ", /*             138 */
	"F2I            ", /*             139 */
	"F2L            ", /*             140 */
	"F2D            ", /*             141 */
	"D2I            ", /*             142 */
	"D2L            ", /*             143 */
	"D2F            ", /*             144 */
	"INT2BYTE       ", /*             145 */
	"INT2CHAR       ", /*             146 */
	"INT2SHORT      ", /*             147 */
	"LCMP           ", /*             148 */
	"FCMPL          ", /*             149 */
	"FCMPG          ", /*             150 */
	"DCMPL          ", /*             151 */
	"DCMPG          ", /*             152 */
	"IFEQ           ", /*             153 */
	"IFNE           ", /*             154 */
	"IFLT           ", /*             155 */
	"IFGE           ", /*             156 */
	"IFGT           ", /*             157 */
	"IFLE           ", /*             158 */
	"IF_ICMPEQ      ", /*             159 */
	"IF_ICMPNE      ", /*             160 */
	"IF_ICMPLT      ", /*             161 */
	"IF_ICMPGE      ", /*             162 */
	"IF_ICMPGT      ", /*             163 */
	"IF_ICMPLE      ", /*             164 */
	"IF_ACMPEQ      ", /*             165 */
	"IF_ACMPNE      ", /*             166 */
	"GOTO           ", /*             167 */
	"JSR            ", /*             168 */
	"RET            ", /*             169 */
	"TABLESWITCH    ", /*             170 */
	"LOOKUPSWITCH   ", /*             171 */
	"IRETURN        ", /*             172 */
	"LRETURN        ", /*             173 */
	"FRETURN        ", /*             174 */
	"DRETURN        ", /*             175 */
	"ARETURN        ", /*             176 */
	"RETURN         ", /*             177 */
	"GETSTATIC      ", /*             178 */
	"PUTSTATIC      ", /*             179 */
	"GETFIELD       ", /*             180 */
	"PUTFIELD       ", /*             181 */
	"INVOKEVIRTUAL  ", /*             182 */
	"INVOKESPECIAL  ", /*             183 */
	"INVOKESTATIC   ", /*             184 */
	"INVOKEINTERFACE", /*             185 */
	"UNDEF186       ", /* UNDEF186    186 */
	"NEW            ", /*             187 */
	"NEWARRAY       ", /*             188 */
	"ANEWARRAY      ", /*             189 */
	"ARRAYLENGTH    ", /*             190 */
	"ATHROW         ", /*             191 */
	"CHECKCAST      ", /*             192 */
	"INSTANCEOF     ", /*             193 */
	"MONITORENTER   ", /*             194 */
	"MONITOREXIT    ", /*             195 */
	"UNDEF196       ", /* WIDE        196 */
	"MULTIANEWARRAY ", /*             197 */
	"IFNULL         ", /*             198 */
	"IFNONNULL      ", /*             199 */
	"UNDEF200       ", /* GOTO_W      200 */
	"UNDEF201       ", /* JSR_W       201 */
	"UNDEF202       ", /* BREAKPOINT  202 */
	"UNDEF203       ", /* UNDEF203    203 */
	"IASTORECONST   ", /*             204 */
	"LASTORECONST   ", /*             205 */
	"FASTORECONST   ", /*             206 */
	"DASTORECONST   ", /*             207 */
	"AASTORECONST   ", /*             208 */
	"BASTORECONST   ", /*             209 */
	"CASTORECONST   ", /*             210 */
	"SASTORECONST   ", /*             211 */
	"PUTSTATICCONST ", /*             212 */
	"PUTFIELDCONST  ", /*             213 */
	"IMULPOW2       ", /*             214 */
	"LMULPOW2       ", /*             215 */

	"IF_FCMPEQ      ", /*             216 */
	"IF_FCMPNE      ", /*             217 */

	"IF_FCMPL_LT    ", /*             218 */
	"IF_FCMPL_GE    ", /*             219 */
	"IF_FCMPL_GT    ", /*             220 */
	"IF_FCMPL_LE    ", /*             221 */

	"IF_FCMPG_LT    ", /*             222 */
	"IF_FCMPG_GE    ", /*             223 */
	"IF_FCMPG_GT    ", /*             224 */
	"IF_FCMPG_LE    ", /*             225 */

	"IF_DCMPEQ      ", /*             226 */
	"IF_DCMPNE      ", /*             227 */

	"IF_DCMPL_LT    ", /*             228 */
	"IF_DCMPL_GE    ", /*             229 */
	"IF_DCMPL_GT    ", /*             230 */
	"IF_DCMPL_LE    ", /*             231 */
	
	"IF_DCMPG_LT    ", /*             232 */
	"IF_DCMPG_GE    ", /*             233 */
	"IF_DCMPG_GT    ", /*             234 */
	"IF_DCMPG_LE    ", /*             235 */
	
	"UNDEF236", "UNDEF237", "UNDEF238", "UNDEF239", "UNDEF240",
	"UNDEF241", "UNDEF242", "UNDEF243", "UNDEF244", "UNDEF245",
	"UNDEF246", "UNDEF247", "UNDEF248", "UNDEF249", "UNDEF250",

	"INLINE_START   ", /*             251 */
	"INLINE_END     ", /*             252 */
	"INLINE_GOTO    ", /*             253 */

	"UNDEF254",

	"BUILTIN        "  /*             255 */
};


char *opcode_names[256] = {
	"NOP            ", /*               0 */
	"ACONST         ", /*               1 */
	"ICONST_M1      ", /* ICONST_M1     2 */
	"ICONST_0       ", /* ICONST_0      3 */
	"ICONST_1       ", /* ICONST_1      4 */
	"ICONST_2       ", /* ICONST_2      5 */
	"ICONST_3       ", /* ICONST_3      6 */
	"ICONST_4       ", /* ICONST_4      7 */
	"ICONST_5       ", /* ICONST_5      8 */
	"LCONST_0       ", /* LCONST_0      9 */
	"LCONST_1       ", /* LCONST_1     10 */
	"FCONST_0       ", /* FCONST_0     11 */
	"FCONST_1       ", /* FCONST_1     12 */
	"FCONST_2       ", /* FCONST_2     13 */
	"DCONST_0       ", /* DCONST_0     14 */
	"DCONST_1       ", /* DCONST_1     15 */
	"BIPUSH         ", /* BIPUSH       16 */
	"SIPUSH         ", /* SIPUSH       17 */
	"LDC            ", /* LDC          18 */
	"LDC_W          ", /* LDC_W        19 */
	"LDC2_W         ", /* LDC2_W       20 */
	"ILOAD          ", /*              21 */
	"LLOAD          ", /*              22 */
	"FLOAD          ", /*              23 */
	"DLOAD          ", /*              24 */
	"ALOAD          ", /*              25 */
	"ILOAD_0        ", /* ILOAD_0      26 */
	"ILOAD_1        ", /* ILOAD_1      27 */
	"ILOAD_2        ", /* ILOAD_2      28 */
	"ILOAD_3        ", /* ILOAD_3      29 */
	"LLOAD_0        ", /* LLOAD_0      30 */
	"LLOAD_1        ", /* LLOAD_1      31 */
	"LLOAD_2        ", /* LLOAD_2      32 */
	"LLOAD_3        ", /* LLOAD_3      33 */
	"FLOAD_0        ", /* FLOAD_0      34 */
	"FLOAD_1        ", /* FLOAD_1      35 */
	"FLOAD_2        ", /* FLOAD_2      36 */
	"FLOAD_3        ", /* FLOAD_3      37 */
	"DLOAD_0        ", /* DLOAD_0      38 */
	"DLOAD_1        ", /* DLOAD_1      39 */
	"DLOAD_2        ", /* DLOAD_2      40 */ 
	"DLOAD_3        ", /* DLOAD_3      41 */
	"ALOAD_0        ", /* ALOAD_0      42 */
	"ALOAD_1        ", /* ALOAD_1      43 */
	"ALOAD_2        ", /* ALOAD_2      44 */
	"ALOAD_3        ", /* ALOAD_3      45 */
	"IALOAD         ", /*              46 */
	"LALOAD         ", /*              47 */
	"FALOAD         ", /*              48 */
	"DALOAD         ", /*              49 */
	"AALOAD         ", /*              50 */
	"BALOAD         ", /*              51 */
	"CALOAD         ", /*              52 */
	"SALOAD         ", /*              53 */
	"ISTORE         ", /*              54 */
	"LSTORE         ", /*              55 */
	"FSTORE         ", /*              56 */
	"DSTORE         ", /*              57 */
	"ASTORE         ", /*              58 */
	"ISTORE_0       ", /* ISTORE_0     59 */
	"ISTORE_1       ", /* ISTORE_1     60 */
	"ISTORE_2       ", /* ISTORE_2     61 */
	"ISTORE_3       ", /* ISTORE_3     62 */
	"LSTORE_0       ", /* LSTORE_0     63 */
	"LSTORE_1       ", /* LSTORE_1     64 */
	"LSTORE_2       ", /* LSTORE_2     65 */
	"LSTORE_3       ", /* LSTORE_3     66 */
	"FSTORE_0       ", /* FSTORE_0     67 */
	"FSTORE_1       ", /* FSTORE_1     68 */
	"FSTORE_2       ", /* FSTORE_2     69 */
	"FSTORE_3       ", /* FSTORE_3     70 */
	"DSTORE_0       ", /* DSTORE_0     71 */
	"DSTORE_1       ", /* DSTORE_1     72 */
	"DSTORE_2       ", /* DSTORE_2     73 */
	"DSTORE_3       ", /* DSTORE_3     74 */
	"ASTORE_0       ", /* ASTORE_0     75 */
	"ASTORE_1       ", /* ASTORE_1     76 */
	"ASTORE_2       ", /* ASTORE_2     77 */
	"ASTORE_3       ", /* ASTORE_3     78 */
	"IASTORE        ", /*              79 */
	"LASTORE        ", /*              80 */
	"FASTORE        ", /*              81 */
	"DASTORE        ", /*              82 */
	"AASTORE        ", /*              83 */
	"BASTORE        ", /*              84 */
	"CASTORE        ", /*              85 */
	"SASTORE        ", /*              86 */
	"POP            ", /*              87 */
	"POP2           ", /*              88 */
	"DUP            ", /*              89 */
	"DUP_X1         ", /*              90 */
	"DUP_X2         ", /*              91 */
	"DUP2           ", /*              92 */
	"DUP2_X1        ", /*              93 */
	"DUP2_X2        ", /*              94 */
	"SWAP           ", /*              95 */
	"IADD           ", /*              96 */
	"LADD           ", /*              97 */
	"FADD           ", /*              98 */
	"DADD           ", /*              99 */
	"ISUB           ", /*             100 */
	"LSUB           ", /*             101 */
	"FSUB           ", /*             102 */
	"DSUB           ", /*             103 */
	"IMUL           ", /*             104 */
	"LMUL           ", /*             105 */
	"FMUL           ", /*             106 */
	"DMUL           ", /*             107 */
	"IDIV           ", /*             108 */
	"LDIV           ", /*             109 */
	"FDIV           ", /*             110 */
	"DDIV           ", /*             111 */
	"IREM           ", /*             112 */
	"LREM           ", /*             113 */
	"FREM           ", /*             114 */
	"DREM           ", /*             115 */
	"INEG           ", /*             116 */
	"LNEG           ", /*             117 */
	"FNEG           ", /*             118 */
	"DNEG           ", /*             119 */
	"ISHL           ", /*             120 */
	"LSHL           ", /*             121 */
	"ISHR           ", /*             122 */
	"LSHR           ", /*             123 */
	"IUSHR          ", /*             124 */
	"LUSHR          ", /*             125 */
	"IAND           ", /*             126 */
	"LAND           ", /*             127 */
	"IOR            ", /*             128 */
	"LOR            ", /*             129 */
	"IXOR           ", /*             130 */
	"LXOR           ", /*             131 */
	"IINC           ", /*             132 */
	"I2L            ", /*             133 */
	"I2F            ", /*             134 */
	"I2D            ", /*             135 */
	"L2I            ", /*             136 */
	"L2F            ", /*             137 */
	"L2D            ", /*             138 */
	"F2I            ", /*             139 */
	"F2L            ", /*             140 */
	"F2D            ", /*             141 */
	"D2I            ", /*             142 */
	"D2L            ", /*             143 */
	"D2F            ", /*             144 */
	"INT2BYTE       ", /*             145 */
	"INT2CHAR       ", /*             146 */
	"INT2SHORT      ", /*             147 */
	"LCMP           ", /*             148 */
	"FCMPL          ", /*             149 */
	"FCMPG          ", /*             150 */
	"DCMPL          ", /*             151 */
	"DCMPG          ", /*             152 */
	"IFEQ           ", /*             153 */
	"IFNE           ", /*             154 */
	"IFLT           ", /*             155 */
	"IFGE           ", /*             156 */
	"IFGT           ", /*             157 */
	"IFLE           ", /*             158 */
	"IF_ICMPEQ      ", /*             159 */
	"IF_ICMPNE      ", /*             160 */
	"IF_ICMPLT      ", /*             161 */
	"IF_ICMPGE      ", /*             162 */
	"IF_ICMPGT      ", /*             163 */
	"IF_ICMPLE      ", /*             164 */
	"IF_ACMPEQ      ", /*             165 */
	"IF_ACMPNE      ", /*             166 */
	"GOTO           ", /*             167 */
	"JSR            ", /*             168 */
	"RET            ", /*             169 */
	"TABLESWITCH    ", /*             170 */
	"LOOKUPSWITCH   ", /*             171 */
	"IRETURN        ", /*             172 */
	"LRETURN        ", /*             173 */
	"FRETURN        ", /*             174 */
	"DRETURN        ", /*             175 */
	"ARETURN        ", /*             176 */
	"RETURN         ", /*             177 */
	"GETSTATIC      ", /*             178 */
	"PUTSTATIC      ", /*             179 */
	"GETFIELD       ", /*             180 */
	"PUTFIELD       ", /*             181 */
	"INVOKEVIRTUAL  ", /*             182 */
	"INVOKESPECIAL  ", /*             183 */
	"INVOKESTATIC   ", /*             184 */
	"INVOKEINTERFACE", /*             185 */
	"UNDEF186       ", /*             186 */
	"NEW            ", /*             187 */
	"NEWARRAY       ", /*             188 */
	"ANEWARRAY      ", /*             189 */
	"ARRAYLENGTH    ", /*             190 */
	"ATHROW         ", /*             191 */
	"CHECKCAST      ", /*             192 */
	"INSTANCEOF     ", /*             193 */
	"MONITORENTER   ", /*             194 */
	"MONITOREXIT    ", /*             195 */
	"WIDE           ", /* WIDE        196 */
	"MULTIANEWARRAY ", /*             197 */
	"IFNULL         ", /*             198 */
	"IFNONNULL      ", /*             199 */
	"GOTO_W         ", /* GOTO_W      200 */
	"JSR_W          ", /* JSR_W       201 */
	"BREAKPOINT     ", /* BREAKPOINT  202 */

	                        "UNDEF203", "UNDEF204", "UNDEF205",
	"UNDEF206", "UNDEF207", "UNDEF208", "UNDEF209", "UNDEF210",
	"UNDEF211", "UNDEF212", "UNDEF213", "UNDEF214", "UNDEF215",
	"UNDEF216", "UNDEF217", "UNDEF218", "UNDEF219", "UNDEF220",
	"UNDEF221", "UNDEF222", "UNDEF223", "UNDEF224", "UNDEF225",
	"UNDEF226", "UNDEF227", "UNDEF228", "UNDEF229", "UNDEF230",
	"UNDEF231", "UNDEF232", "UNDEF233", "UNDEF234", "UNDEF235",
	"UNDEF236", "UNDEF237", "UNDEF238", "UNDEF239", "UNDEF240",
	"UNDEF241", "UNDEF242", "UNDEF243", "UNDEF244", "UNDEF245",
	"UNDEF246", "UNDEF247", "UNDEF248", "UNDEF249", "UNDEF250",
	"UNDEF251", "UNDEF252", "UNDEF253", "UNDEF254", "UNDEF255"
};

int op_data[256][OP_DATA_SIZE];
/* int op_needs_saved[256]; */
/* int op_is_pei[256]; */

/* jit_init ********************************************************************

   Initializes the JIT subsystem.

*******************************************************************************/

void jit_init(void)
{
	s4 i;

	for( i = 0; i < 256; i++) {
		op_data[i][NEEDS_SAVED] = 0;
		op_data[i][PEI] = 0;
	}

	op_data[ICMD_AASTORE  ][NEEDS_SAVED] = 1;
#if !SUPPORT_DIVISION
	op_data[ICMD_IDIV     ][NEEDS_SAVED] = 1;
	op_data[ICMD_IREM     ][NEEDS_SAVED] = 1;
#endif
#if !(SUPPORT_DIVISION && SUPPORT_LONG && SUPPORT_LONG_DIV)
	op_data[ICMD_LDIV     ][NEEDS_SAVED] = 1;
	op_data[ICMD_LREM     ][NEEDS_SAVED] = 1;
#endif
	op_data[ICMD_CHECKCAST][NEEDS_SAVED] = 1;

	op_data[ICMD_BUILTIN        ][NEEDS_SAVED] = 1;
	op_data[ICMD_INVOKESTATIC   ][NEEDS_SAVED] = 1;
	op_data[ICMD_INVOKESPECIAL  ][NEEDS_SAVED] = 1;
	op_data[ICMD_INVOKEVIRTUAL  ][NEEDS_SAVED] = 1;
	op_data[ICMD_INVOKEINTERFACE][NEEDS_SAVED] = 1;
	op_data[ICMD_MULTIANEWARRAY ][NEEDS_SAVED] = 1;

	op_data[ICMD_ACONST][PEI] = 1; /* OutOfMemoryError */
	op_data[ICMD_NEWARRAY][PEI] = 1; /* NegativeArraySizeException,OutOfMemoryError */
	op_data[ICMD_ANEWARRAY][PEI] = 1; /* NegativeArraySizeException,OutOfMemoryError */
	op_data[ICMD_MULTIANEWARRAY][PEI] = 1; /* NegativeArraySizeException,OutOfMemoryError */
	op_data[ICMD_ARRAYLENGTH][PEI] = 1; /* NullPointerException */
	op_data[ICMD_IALOAD][PEI] = 1; /* NullPointerException, ArrayIndexOutOfBoundsException */
	op_data[ICMD_LALOAD][PEI] = 1; /* NullPointerException, ArrayIndexOutOfBoundsException */
	op_data[ICMD_FALOAD][PEI] = 1; /* NullPointerException, ArrayIndexOutOfBoundsException */
	op_data[ICMD_DALOAD][PEI] = 1; /* NullPointerException, ArrayIndexOutOfBoundsException */
	op_data[ICMD_AALOAD][PEI] = 1; /* NullPointerException, ArrayIndexOutOfBoundsException */
	op_data[ICMD_BALOAD][PEI] = 1; /* NullPointerException, ArrayIndexOutOfBoundsException */
	op_data[ICMD_CALOAD][PEI] = 1; /* NullPointerException, ArrayIndexOutOfBoundsException */
	op_data[ICMD_SALOAD][PEI] = 1; /* NullPointerException, ArrayIndexOutOfBoundsException */
	op_data[ICMD_IASTORE][PEI] = 1; /* NullPointerException, ArrayIndexOutOfBoundsException */
	op_data[ICMD_LASTORE][PEI] = 1; /* NullPointerException, ArrayIndexOutOfBoundsException */
	op_data[ICMD_FASTORE][PEI] = 1; /* NullPointerException, ArrayIndexOutOfBoundsException */
	op_data[ICMD_DASTORE][PEI] = 1; /* NullPointerException, ArrayIndexOutOfBoundsException */
	op_data[ICMD_AASTORE][PEI] = 1; /* NullPointerException, ArrayIndexOutOfBoundsException */
	/* ArrayStoreException */
	op_data[ICMD_BASTORE][PEI] = 1; /* NullPointerException, ArrayIndexOutOfBoundsException */
	op_data[ICMD_CASTORE][PEI] = 1; /* NullPointerException, ArrayIndexOutOfBoundsException */
	op_data[ICMD_SASTORE][PEI] = 1; /* NullPointerException, ArrayIndexOutOfBoundsException */

	op_data[ICMD_IDIV][PEI] = 1; /* "/ by Zero" ArithmeticException */
	op_data[ICMD_LDIV][PEI] = 1; /* "/ by Zero" ArithmeticException */
	op_data[ICMD_IREM][PEI] = 1; /* "/ by Zero" ArithmeticException */
	op_data[ICMD_LREM][PEI] = 1; /* "/ by Zero" ArithmeticException */

	op_data[ICMD_PUTFIELD][PEI] = 1; /* NullPointerException, IncompatibleClassChangeError */
	op_data[ICMD_GETFIELD][PEI] = 1; /* NullPointerException, IncompatibleClassChangeError */
#if 0
	op_data[ICMD_PUTFIELDCONST][PEI] = 1; /*NullPointerException, IncompatibleClassChangeError */
	op_data[ICMD_GETFIELDCONST][PEI] = 1; /*NullPointerException, IncompatibleClassChangeError */
#endif
	op_data[ICMD_PUTSTATIC][PEI] = 1; /* IncompatibleClassChangeError */
	op_data[ICMD_GETSTATIC][PEI] = 1; /* IncompatibleClassChangeError */

	op_data[ICMD_BUILTIN][PEI] = 1; /* ?NullPointerException, StackOverflowError,? * */
	op_data[ICMD_INVOKEVIRTUAL][PEI] = 1; /* NullPointerException, StackOverflowError, * */
	op_data[ICMD_INVOKESPECIAL][PEI] = 1; /* NullPointerException, StackOverflowError, * */
	op_data[ICMD_INVOKESTATIC][PEI] = 1; /* StackOverflowError, * */
	op_data[ICMD_INVOKEINTERFACE][PEI] = 1; /* NullPointerException, StackOverflowError, * */

	op_data[ICMD_ATHROW][PEI] = 1; /* NullPointerException, * */

	op_data[ICMD_CHECKCAST][PEI] = 1; /* ClassCastException */

	op_data[ICMD_MONITORENTER][PEI] = 1; /* NullPointerException */
	op_data[ICMD_MONITOREXIT][PEI] = 1; /* NullPointerException */

	op_data[ICMD_CHECKNULL][PEI] = 1; /* NullPointerException */

	/* initialize stack analysis subsystem */

	(void) stack_init();

	/* initialize show subsystem */

#if !defined(NDEBUG)
	(void) show_init();
#endif

	/* initialize codegen subsystem */

	codegen_init();
}


/* jit_close *******************************************************************

   Close the JIT subsystem.

*******************************************************************************/

void jit_close(void)
{
	/* do nothing */
}


/* dummy function, used when there is no JavaVM code available                */

static u1 *do_nothing_function(void)
{
	return NULL;
}


/* jit_jitdata_new *************************************************************

   Allocates and initalizes a new jitdata structure.

*******************************************************************************/

static jitdata *jit_jitdata_new(methodinfo *m)
{
	jitdata *jd;

	/* allocate jitdata structure and fill it */

	jd = DNEW(jitdata);

	jd->m     = m;
	jd->cd    = DNEW(codegendata);
	jd->rd    = DNEW(registerdata);
#if defined(ENABLE_LOOP)
	jd->ld    = DNEW(loopdata);
#endif

	/* Allocate codeinfo memory from the heap as we need to keep them. */

	jd->code  = code_codeinfo_new(m);

	/* initialize variables */

	jd->flags        = 0;
	jd->isleafmethod = true;

	return jd;
}


/* jit_compile *****************************************************************

   Translates one method to machine code.

*******************************************************************************/

static u1 *jit_compile_intern(jitdata *jd);

u1 *jit_compile(methodinfo *m)
{
	u1      *r;
	jitdata *jd;
	s4       dumpsize;

	STATISTICS(count_jit_calls++);

	/* Initialize the static function's class. */

	/* ATTENTION: This MUST be done before the method lock is aquired,
	   otherwise we could run into a deadlock with <clinit>'s that
	   call static methods of it's own class. */

  	if ((m->flags & ACC_STATIC) && !(m->class->state & CLASS_INITIALIZED)) {
#if !defined(NDEBUG)
		if (initverbose)
			log_message_class("Initialize class ", m->class);
#endif

		if (!initialize_class(m->class))
			return NULL;

		/* check if the method has been compiled during initialization */

		if ((m->code != NULL) && (m->code->entrypoint != NULL))
			return m->code->entrypoint;
	}

	/* enter a monitor on the method */

	LOCK_MONITOR_ENTER(m);

	/* if method has been already compiled return immediately */

	if (m->code != NULL) {
		LOCK_MONITOR_EXIT(m);

		assert(m->code->entrypoint);
		return m->code->entrypoint;
	}

	STATISTICS(count_methods++);

#if defined(ENABLE_STATISTICS)
	/* measure time */

	if (opt_getcompilingtime)
		compilingtime_start();
#endif

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* create jitdata structure */

	jd = jit_jitdata_new(m);

	/* set the flags for the current JIT run */

	jd->flags = JITDATA_FLAG_PARSE;

#if defined(ENABLE_VERIFIER)
	if (opt_verify)
		jd->flags |= JITDATA_FLAG_VERIFY;
#endif

	if (opt_prof)
		jd->flags |= JITDATA_FLAG_INSTRUMENT;

#if defined(ENABLE_IFCONV)
	if (opt_ifconv)
		jd->flags |= JITDATA_FLAG_IFCONV;
#endif

	if (opt_showintermediate)
		jd->flags |= JITDATA_FLAG_SHOWINTERMEDIATE;

	if (opt_showdisassemble)
		jd->flags |= JITDATA_FLAG_SHOWDISASSEMBLE;

	if (opt_verbosecall)
		jd->flags |= JITDATA_FLAG_VERBOSECALL;

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (!opt_intrp)
# endif
		/* initialize the register allocator */
	{
		reg_setup(jd);
	}
#endif

	/* setup the codegendata memory */

	codegen_setup(jd);

	/* now call internal compile function */

	r = jit_compile_intern(jd);

	if (r == NULL) {
		/* We had an exception! Finish stuff here if necessary. */

		/* release codeinfo */

		code_codeinfo_free(jd->code);

		/* Release memory for basic block profiling information. */

		if (JITDATA_HAS_FLAG_INSTRUMENT(jd))
			if (jd->code->bbfrequency != NULL)
				MFREE(jd->code->bbfrequency, u4, jd->code->basicblockcount);
	}
	else {
		DEBUG_JIT_COMPILEVERBOSE("Running: ");
	}

	/* release dump area */

	dump_release(dumpsize);

#if defined(ENABLE_STATISTICS)
	/* measure time */

	if (opt_getcompilingtime)
		compilingtime_stop();
#endif

	/* leave the monitor */

	LOCK_MONITOR_EXIT(m);

	/* return pointer to the methods entry point */

	return r;
}


/* jit_recompile ***************************************************************

   Recompiles a Java method.

*******************************************************************************/

u1 *jit_recompile(methodinfo *m)
{
	u1      *r;
	jitdata *jd;
	u1       optlevel;
	s4       dumpsize;

	/* check for max. optimization level */

	optlevel = m->code->optlevel;

	if (optlevel == 1) {
/* 		log_message_method("not recompiling: ", m); */
		return NULL;
	}

	log_message_method("Recompiling start: ", m);

	STATISTICS(count_jit_calls++);

#if defined(ENABLE_STATISTICS)
	/* measure time */

	if (opt_getcompilingtime)
		compilingtime_start();
#endif

	/* mark start of dump memory area */

	dumpsize = dump_size();

	/* create jitdata structure */

	jd = jit_jitdata_new(m);

	/* set the current optimization level to the previous one plus 1 */

	jd->code->optlevel = optlevel + 1;

	/* get the optimization flags for the current JIT run */

	jd->flags |= JITDATA_FLAG_REORDER;
	jd->flags |= JITDATA_FLAG_SHOWINTERMEDIATE;
	jd->flags |= JITDATA_FLAG_SHOWDISASSEMBLE;
/* 	jd->flags |= JITDATA_FLAG_VERBOSECALL; */

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (!opt_intrp)
# endif
		/* initialize the register allocator */

		reg_setup(jd);
#endif

	/* setup the codegendata memory */

	codegen_setup(jd);

	/* now call internal compile function */

	r = jit_compile_intern(jd);

	if (r == NULL) {
		/* We had an exception! Finish stuff here if necessary. */

		/* release codeinfo */

		code_codeinfo_free(jd->code);
	}

	/* release dump area */

	dump_release(dumpsize);

#if defined(ENABLE_STATISTICS)
	/* measure time */

	if (opt_getcompilingtime)
		compilingtime_stop();
#endif

	log_message_method("Recompiling done: ", m);

	/* return pointer to the methods entry point */

	return r;
}


/* jit_compile_intern **********************************************************

   Static internal function which does the actual compilation.

*******************************************************************************/

static u1 *jit_compile_intern(jitdata *jd)
{
	methodinfo  *m;
	codegendata *cd;
	codeinfo    *code;

#if defined(ENABLE_RT_TIMING)
	struct timespec time_start,time_checks,time_parse,time_stack,
					time_typecheck,time_loop,time_ifconv,time_alloc,
					time_rplpoints,time_codegen;
#endif
	
	RT_TIMING_GET_TIME(time_start);

	/* get required compiler data */

#if defined(ENABLE_LSRA) || defined(ENABLE_SSA)
	jd->ls = NULL;
#endif
	m    = jd->m;
	code = jd->code;
	cd   = jd->cd;
	
	/* print log message for compiled method */

	DEBUG_JIT_COMPILEVERBOSE("Compiling: ");

	/* handle native methods and create a native stub */

	if (m->flags & ACC_NATIVE) {
		functionptr f;

#if defined(WITH_STATIC_CLASSPATH)
		f = native_findfunction(m->class->name,	m->name, m->descriptor,
								(m->flags & ACC_STATIC));
		if (f == NULL)
			return NULL;
#else
		f = NULL;
#endif

		code = codegen_createnativestub(f, m);

		assert(!m->code); /* native methods are never recompiled */
		m->code = code;
		
		return code->entrypoint;
	}

	/* if there is no javacode, print error message and return empty method   */

	if (m->jcode == NULL) {
		DEBUG_JIT_COMPILEVERBOSE("No code given for: ");

		code->entrypoint = (u1 *) (ptrint) do_nothing_function;
		m->code = code;

		return code->entrypoint;        /* return empty method                */
	}

#if defined(ENABLE_STATISTICS)
	if (opt_stat) {
		count_tryblocks    += m->exceptiontablelength;
		count_javacodesize += m->jcodelength + 18;
		count_javaexcsize  += m->exceptiontablelength * SIZEOF_VOID_P;
	}
#endif

	RT_TIMING_GET_TIME(time_checks);

	/* call the compiler passes ***********************************************/

	DEBUG_JIT_COMPILEVERBOSE("Parsing: ");

	/* call parse pass */

	if (!parse(jd)) {
		DEBUG_JIT_COMPILEVERBOSE("Exception while parsing: ");

		return NULL;
	}
	RT_TIMING_GET_TIME(time_parse);

	DEBUG_JIT_COMPILEVERBOSE("Parsing done: ");
	
	DEBUG_JIT_COMPILEVERBOSE("Analysing: ");

	/* call stack analysis pass */

	if (!stack_analyse(jd)) {
		DEBUG_JIT_COMPILEVERBOSE("Exception while analysing: ");

		return NULL;
	}
	RT_TIMING_GET_TIME(time_stack);

	DEBUG_JIT_COMPILEVERBOSE("Analysing done: ");

	/* Build the CFG.  This has to be done after stack_analyse, as
	   there happens the JSR elimination. */

	if (!cfg_build(jd))
		return NULL;

#ifdef ENABLE_VERIFIER
	if (jd->flags & JITDATA_FLAG_VERIFY) {
		DEBUG_JIT_COMPILEVERBOSE("Typechecking: ");

		/* call typecheck pass */
		if (!typecheck(jd)) {
			DEBUG_JIT_COMPILEVERBOSE("Exception while typechecking: ");

			return NULL;
		}

		DEBUG_JIT_COMPILEVERBOSE("Typechecking done: ");
	}
#endif
	RT_TIMING_GET_TIME(time_typecheck);

#if defined(ENABLE_LOOP)
	if (opt_loops) {
		depthFirst(jd);
		analyseGraph(jd);
		optimize_loops(jd);
	}
#endif
	RT_TIMING_GET_TIME(time_loop);

#if defined(ENABLE_IFCONV)
	if (JITDATA_HAS_FLAG_IFCONV(jd))
		if (!ifconv_static(jd))
			return NULL;
#endif
	RT_TIMING_GET_TIME(time_ifconv);

	/* Basic block reordering.  I think this should be done after
	   if-conversion, as we could lose the ability to do the
	   if-conversion. */

	if (JITDATA_HAS_FLAG_REORDER(jd))
		if (!reorder(jd))
			return NULL;

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (!opt_intrp) {
# endif
		DEBUG_JIT_COMPILEVERBOSE("Allocating registers: ");

#if defined(ENABLE_LSRA) && !defined(ENABLE_SSA)
		/* allocate registers */
		if (opt_lsra) {
			if (!lsra(jd))
				return NULL;

			STATISTICS(count_methods_allocated_by_lsra++);

		} else
# endif /* defined(ENABLE_LSRA) && !defined(ENABLE_SSA) */
#if defined(ENABLE_SSA)
		/* allocate registers */
		if ((opt_lsra) && (cd->exceptiontablelength == 0)) {
			jd->ls = DNEW(lsradata);
			lsra(jd);

			STATISTICS(count_methods_allocated_by_lsra++);

		} else
# endif /* defined(ENABLE_SSA) */
		{
			STATISTICS(count_locals_conflicts += (cd->maxlocals - 1) * (cd->maxlocals));

			regalloc(jd);
		}

		STATISTICS(reg_make_statistics(jd));

		DEBUG_JIT_COMPILEVERBOSE("Allocating registers done: ");
# if defined(ENABLE_INTRP)
	}
# endif
#endif /* defined(ENABLE_JIT) */
	RT_TIMING_GET_TIME(time_alloc);

	/* Allocate memory for basic block profiling information. This
	   _must_ be done after loop optimization and register allocation,
	   since they can change the basic block count. */

	if (JITDATA_HAS_FLAG_INSTRUMENT(jd))
		code->bbfrequency = MNEW(u4, jd->new_basicblockcount);

	DEBUG_JIT_COMPILEVERBOSE("Generating code: ");

	/* create the replacement points */
#if 0
	if (!replace_create_replacement_points(jd))
		return NULL;
#endif
	RT_TIMING_GET_TIME(time_rplpoints);

	/* now generate the machine code */

#if defined(ENABLE_JIT)
# if defined(ENABLE_INTRP)
	if (opt_intrp) {
		if (!intrp_codegen(jd)) {
			DEBUG_JIT_COMPILEVERBOSE("Exception while generating code: ");

			return NULL;
		}
	} else
# endif
		{
			if (!codegen(jd)) {
				DEBUG_JIT_COMPILEVERBOSE("Exception while generating code: ");

				return NULL;
			}
		}
#else
	if (!intrp_codegen(jd)) {
		DEBUG_JIT_COMPILEVERBOSE("Exception while generating code: ");

		return NULL;
	}
#endif
	RT_TIMING_GET_TIME(time_codegen);

	DEBUG_JIT_COMPILEVERBOSE("Generating code done: ");

#if !defined(NDEBUG)
	/* intermediate and assembly code listings */
		
	if (JITDATA_HAS_FLAG_SHOWINTERMEDIATE(jd)) {
		show_method(jd, SHOW_CODE);
	}
	else if (JITDATA_HAS_FLAG_SHOWDISASSEMBLE(jd)) {
# if defined(ENABLE_DISASSEMBLER)
		DISASSEMBLE(code->entrypoint,
					code->entrypoint + (code->mcodelength - cd->dseglen));
# endif
	}

	if (opt_showddatasegment)
		dseg_display(jd);
#endif

	DEBUG_JIT_COMPILEVERBOSE("Compiling done: ");

	/* switch to the newly generated code */

	assert(code);
	assert(code->entrypoint);

	/* add the current compile version to the methodinfo */

	code->prev = m->code;
	m->code = code;

	RT_TIMING_TIME_DIFF(time_start,time_checks,RT_TIMING_JIT_CHECKS);
	RT_TIMING_TIME_DIFF(time_checks,time_parse,RT_TIMING_JIT_PARSE);
	RT_TIMING_TIME_DIFF(time_parse,time_stack,RT_TIMING_JIT_STACK);
	RT_TIMING_TIME_DIFF(time_stack,time_typecheck,RT_TIMING_JIT_TYPECHECK);
	RT_TIMING_TIME_DIFF(time_typecheck,time_loop,RT_TIMING_JIT_LOOP);
	RT_TIMING_TIME_DIFF(time_loop,time_alloc,RT_TIMING_JIT_ALLOC);
	RT_TIMING_TIME_DIFF(time_alloc,time_rplpoints,RT_TIMING_JIT_RPLPOINTS);
	RT_TIMING_TIME_DIFF(time_rplpoints,time_codegen,RT_TIMING_JIT_CODEGEN);
	RT_TIMING_TIME_DIFF(time_start,time_codegen,RT_TIMING_JIT_TOTAL);

	/* return pointer to the methods entry point */

	return code->entrypoint;
} 


/* jit_asm_compile *************************************************************

   This method is called from asm_vm_call_method and does:

     - create stackframe info for exceptions
     - compile the method
     - patch the entrypoint of the method into the calculated address in
       the JIT code
     - flushes the instruction cache.

*******************************************************************************/

u1 *jit_asm_compile(methodinfo *m, u1 *mptr, u1 *sp, u1 *ra)
{
	stackframeinfo  sfi;
	u1             *entrypoint;
	u1             *pa;
	ptrint         *p;

	/* create the stackframeinfo (XPC is equal to RA) */

	stacktrace_create_extern_stackframeinfo(&sfi, NULL, sp, ra, ra);

	/* actually compile the method */

	entrypoint = jit_compile(m);

	/* remove the stackframeinfo */

	stacktrace_remove_stackframeinfo(&sfi);

	/* there was a problem during compilation */

	if (entrypoint == NULL)
		return NULL;

	/* get the method patch address */

	pa = md_get_method_patch_address(ra, &sfi, mptr);

	/* patch the method entry point */

	p = (ptrint *) pa;

	*p = (ptrint) entrypoint;

	/* flush the instruction cache */

	md_icacheflush(pa, SIZEOF_VOID_P);

	return entrypoint;
}


/* jit_complement_condition ****************************************************

   Returns the complement of the passed conditional instruction.

   We use the order of the different conditions, e.g.:

   ICMD_IFEQ         153
   ICMD_IFNE         154

   If the passed opcode is odd, we simply add 1 to get the complement.
   If the opcode is even, we subtract 1.

   Exception:

   ICMD_IFNULL       198
   ICMD_IFNONNULL    199

*******************************************************************************/

s4 jit_complement_condition(s4 opcode)
{
	switch (opcode) {
	case ICMD_IFNULL:
		return ICMD_IFNONNULL;

	case ICMD_IFNONNULL:
		return ICMD_IFNULL;

	default:
		/* check if opcode is odd */

		if (opcode & 0x1)
			return opcode + 1;
		else
			return opcode - 1;
	}
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
