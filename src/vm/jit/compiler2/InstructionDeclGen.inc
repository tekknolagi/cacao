/* src/vm/jit/compiler2/InstructionDeclGen.inc - Instruction Declarations

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

/*
WARNING: THIS FILE IS AUTO-GENERATED! DO NOT ALTER!
Instead have a look at the generator (./instruction_gen.py)
and the input file (./instruction_table.csv).
*/

class UnaryInst;
class BinaryInst;
class FieldAccessInst;
class CHECKNULLInst;
class ARRAYLENGTHInst;
class ARRAYBOUNDSCHECKInst;
class NEGInst;
class CASTInst;
class ADDInst;
class SUBInst;
class MULInst;
class DIVInst;
class REMInst;
class SHLInst;
class USHRInst;
class ANDInst;
class ORInst;
class XORInst;
class CMPInst;
class CONSTInst;
class GETFIELDInst;
class PUTFIELDInst;
class PUTSTATICInst;
class GETSTATICInst;
class INCInst;
class AREFInst;
class ASTOREInst;
class ALOADInst;
class LOADInst;
class NEWInst;
class NEWARRAYInst;
class ANEWARRAYInst;
class MULTIANEWARRAYInst;
class CHECKCASTInst;
class INSTANCEOFInst;
class GOTOInst;
class BUILTINInst;
class INVOKEInst;
class INVOKEVIRTUALInst;
class INVOKESPECIALInst;
class INVOKESTATICInst;
class INVOKEINTERFACEInst;
class IFInst;
class IF_CMPInst;
class TABLESWITCHInst;
class LOOKUPSWITCHInst;
class RETURNInst;
class THROWInst;
class PHIInst;
class BeginInst;
class EndInst;
class SourceStateInst;
class ReplacementEntryInst;
class AssumptionInst;
class DeoptimizeInst;

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
