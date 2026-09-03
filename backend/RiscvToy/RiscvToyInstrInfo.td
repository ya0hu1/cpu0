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
def call_symbol : Operand<i32>;

def immSExt12 : ImmLeaf<i32, [{ return isInt<12>(Imm); }]>;

// A frame index is not a normal register/address in SelectionDAG, so memory
// patterns use this complex pattern and lower it to a TargetFrameIndex.
def AddrFI : ComplexPattern<i32, 1, "SelectAddrFI", [frameindex], []>;

//===----------------------------------------------------------------------===//
// Target-specific SDNodes
//===----------------------------------------------------------------------===//

def SDT_RiscvToyCallSeqStart : SDCallSeqStart<[SDTCisVT<0, i32>,
                                               SDTCisVT<1, i32>]>;
def SDT_RiscvToyCallSeqEnd : SDCallSeqEnd<[SDTCisVT<0, i32>,
                                           SDTCisVT<1, i32>]>;
def SDT_RiscvToyCall : SDTypeProfile<0, -1, [SDTCisVT<0, i32>]>;

def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_RiscvToyCallSeqStart,
                           [SDNPHasChain, SDNPOutGlue]>;
def callseq_end : SDNode<"ISD::CALLSEQ_END", SDT_RiscvToyCallSeqEnd,
                         [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

def RiscvToyCallFlag : SDNode<"RiscvToyISD::CALL", SDT_RiscvToyCall,
                              [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue,
                               SDNPVariadic]>;
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

// Loads and stores use the full GPR class (including ra and sp) because the
// generic spill machinery must be able to save/restore x1 and frame lowering
// replaces frame indices with sp.
let mayLoad = 1, mayStore = 0 in
def RiscvToyLW : RiscvToyI<0b010, 0b0000011,
                           (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
                           "lw", "$rd, ${imm12}(${rs1})", []>;

let mayLoad = 0, mayStore = 1 in
def RiscvToySW : RiscvToyS<0b010, 0b0100011,
                           (outs), (ins GPR:$rs2, GPR:$rs1, simm12:$imm12),
                           "sw", "$rs2, ${imm12}(${rs1})", []>;

// FrameIndex and register + 12-bit offset addressing.
def : Pat<(load AddrFI:$rs1), (RiscvToyLW AddrFI:$rs1, 0)>;
def : Pat<(load (add AddrFI:$rs1, immSExt12:$imm12)),
          (RiscvToyLW AddrFI:$rs1, immSExt12:$imm12)>;
def : Pat<(load GPR:$rs1), (RiscvToyLW GPR:$rs1, 0)>;
def : Pat<(load (add GPR:$rs1, immSExt12:$imm12)),
          (RiscvToyLW GPR:$rs1, immSExt12:$imm12)>;

def : Pat<(store GPR:$rs2, AddrFI:$rs1),
          (RiscvToySW GPR:$rs2, AddrFI:$rs1, 0)>;
def : Pat<(store GPR:$rs2, (add AddrFI:$rs1, immSExt12:$imm12)),
          (RiscvToySW GPR:$rs2, AddrFI:$rs1, immSExt12:$imm12)>;
def : Pat<(store GPR:$rs2, GPR:$rs1), (RiscvToySW GPR:$rs2, GPR:$rs1, 0)>;
def : Pat<(store GPR:$rs2, (add GPR:$rs1, immSExt12:$imm12)),
          (RiscvToySW GPR:$rs2, GPR:$rs1, immSExt12:$imm12)>;

// Keep the internal instruction real (jalr x0, 0(ra)), but print the usual
// assembly mnemonic "ret".
def : InstAlias<"ret", (RiscvToyJALR X0, X1, 0), 4>;

let isReturn = 1, isBarrier = 1, isTerminator = 1, isCodeGenOnly = 1 in
def PseudoRET : RiscvToyPseudo<(outs), (ins), "ret", [(RiscvToyRetFlag)]>,
                PseudoInstExpansion<(RiscvToyJALR X0, X1, 0)>;

let isCall = 1,
    Defs = [X1, X5, X6, X7, X10, X11, X12, X13, X14, X15, X16, X17,
            X28, X29, X30, X31],
    isCodeGenOnly = 1 in
def PseudoCALL : RiscvToyPseudo<(outs), (ins call_symbol:$dst),
                                "call\t$dst", []>;

def : Pat<(RiscvToyCallFlag tglobaladdr:$dst),
          (PseudoCALL tglobaladdr:$dst)>;
def : Pat<(RiscvToyCallFlag texternalsym:$dst),
          (PseudoCALL texternalsym:$dst)>;

let Defs = [X2], Uses = [X2] in {
def ADJCALLSTACKDOWN
    : RiscvToyPseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                     "#ADJCALLSTACKDOWN", [(callseq_start timm:$amt1,
                                                          timm:$amt2)]>;
def ADJCALLSTACKUP
    : RiscvToyPseudo<(outs), (ins i32imm:$amt1, i32imm:$amt2),
                     "#ADJCALLSTACKUP", [(callseq_end timm:$amt1,
                                                      timm:$amt2)]>;
} // Defs = [X2], Uses = [X2]
