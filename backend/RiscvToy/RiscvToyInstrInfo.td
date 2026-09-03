//===-- RiscvToyInstrInfo.td - RiscvToy instruction definitions -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the RV32I instruction subset used by the RiscvToy
// teaching backend.
//
//===----------------------------------------------------------------------===//

include "RiscvToyInstrFormats.td"

//===----------------------------------------------------------------------===//
// Operand and pattern definitions
//===----------------------------------------------------------------------===//

def simm12 : Operand<i32>;

def immSExt12 : ImmLeaf<i32, [{ return isInt<12>(Imm); }]>;

//===----------------------------------------------------------------------===//
// Target-specific SDNodes
//===----------------------------------------------------------------------===//

def RiscvToyRetFlag : SDNode<"RiscvToyISD::RET_FLAG", SDTNone,
                             [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

//===----------------------------------------------------------------------===//
// RV32I instructions
//===----------------------------------------------------------------------===//

let hasSideEffects = 0, mayLoad = 0, mayStore = 0 in {

let isAdd = 1, isCommutable = 1, isReMaterializable = 1 in
def RiscvToyADD : RiscvToyR<0b0000000, 0b000, 0b0110011,
                            (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                            "add", "$rd, $rs1, $rs2",
                            [(set GPR:$rd, (add GPR:$rs1, GPR:$rs2))]>;

let isReMaterializable = 1 in
def RiscvToySUB : RiscvToyR<0b0100000, 0b000, 0b0110011,
                            (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                            "sub", "$rd, $rs1, $rs2",
                            [(set GPR:$rd, (sub GPR:$rs1, GPR:$rs2))]>;

let isCommutable = 1, isReMaterializable = 1 in
def RiscvToyAND : RiscvToyR<0b0000000, 0b111, 0b0110011,
                            (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                            "and", "$rd, $rs1, $rs2",
                            [(set GPR:$rd, (and GPR:$rs1, GPR:$rs2))]>;

let isCommutable = 1, isReMaterializable = 1 in
def RiscvToyOR : RiscvToyR<0b0000000, 0b110, 0b0110011,
                           (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                           "or", "$rd, $rs1, $rs2",
                           [(set GPR:$rd, (or GPR:$rs1, GPR:$rs2))]>;

let isCommutable = 1, isReMaterializable = 1 in
def RiscvToyXOR : RiscvToyR<0b0000000, 0b100, 0b0110011,
                            (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                            "xor", "$rd, $rs1, $rs2",
                            [(set GPR:$rd, (xor GPR:$rs1, GPR:$rs2))]>;

let isReMaterializable = 1 in
def RiscvToyADDI : RiscvToyI<0b000, 0b0010011,
                             (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
                             "addi", "$rd, $rs1, $imm12",
                             [(set GPR:$rd,
                               (add GPR:$rs1, immSExt12:$imm12))]>;

def RiscvToyJALR : RiscvToyI<0b000, 0b1100111,
                             (outs GPRFull:$rd),
                             (ins GPRFull:$rs1, simm12:$imm12),
                             "jalr", "$rd, ${imm12}(${rs1})", []>;
} // hasSideEffects = 0, mayLoad = 0, mayStore = 0

let isReturn = 1, isBarrier = 1, isTerminator = 1, isCodeGenOnly = 1 in
def PseudoRET : RiscvToyPseudo<(outs), (ins), "ret", [(RiscvToyRetFlag)]>,
                PseudoInstExpansion<(RiscvToyJALR X0, X1, 0)>;
