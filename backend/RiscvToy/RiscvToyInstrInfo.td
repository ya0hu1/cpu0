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
def brtarget : Operand<OtherVT> {
  let PrintMethod = "printOperand";
}

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
def SDT_RiscvToySelectCC : SDTypeProfile<1, 5, [SDTCisSameAs<1, 2>,
                                                SDTCisSameAs<0, 4>,
                                                SDTCisSameAs<4, 5>]>;

def callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_RiscvToyCallSeqStart,
                           [SDNPHasChain, SDNPOutGlue]>;
def callseq_end : SDNode<"ISD::CALLSEQ_END", SDT_RiscvToyCallSeqEnd,
                         [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

def RiscvToyCallFlag : SDNode<"RiscvToyISD::CALL", SDT_RiscvToyCall,
                              [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue,
                               SDNPVariadic]>;
def RiscvToySelectCCFlag : SDNode<"RiscvToyISD::SELECT_CC",
                                  SDT_RiscvToySelectCC>;
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

def RiscvToySLTI : RiscvToyI<0b010, 0b0010011,
                             (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
                             "slti", "$rd, $rs1, $imm12", []>;

def RiscvToySLTIU : RiscvToyI<0b011, 0b0010011,
                              (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
                              "sltiu", "$rd, $rs1, $imm12", []>;

def RiscvToyXORI : RiscvToyI<0b100, 0b0010011,
                             (outs GPR:$rd), (ins GPR:$rs1, simm12:$imm12),
                             "xori", "$rd, $rs1, $imm12", []>;

def RiscvToySLT : RiscvToyR<0b0000000, 0b010, 0b0110011,
                            (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                            "slt", "$rd, $rs1, $rs2", []>;

def RiscvToySLTU : RiscvToyR<0b0000000, 0b011, 0b0110011,
                             (outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                             "sltu", "$rd, $rs1, $rs2", []>;

def RiscvToyJALR : RiscvToyI<0b000, 0b1100111,
                             (outs GPRFull:$rd),
                             (ins GPRFull:$rs1, simm12:$imm12),
                             "jalr", "$rd, ${imm12}(${rs1})", []>;

let isBranch = 1, isTerminator = 1 in {
def RiscvToyBEQ : RiscvToyB<0b000, 0b1100011,
                            (outs), (ins GPR:$rs1, GPR:$rs2,
                                     brtarget:$BrDst),
                            "beq", "$rs1, $rs2, $BrDst", []>;
def RiscvToyBNE : RiscvToyB<0b001, 0b1100011,
                            (outs), (ins GPR:$rs1, GPR:$rs2,
                                     brtarget:$BrDst),
                            "bne", "$rs1, $rs2, $BrDst", []>;
def RiscvToyBLT : RiscvToyB<0b100, 0b1100011,
                            (outs), (ins GPR:$rs1, GPR:$rs2,
                                     brtarget:$BrDst),
                            "blt", "$rs1, $rs2, $BrDst", []>;
def RiscvToyBGE : RiscvToyB<0b101, 0b1100011,
                            (outs), (ins GPR:$rs1, GPR:$rs2,
                                     brtarget:$BrDst),
                            "bge", "$rs1, $rs2, $BrDst", []>;
def RiscvToyBLTU : RiscvToyB<0b110, 0b1100011,
                             (outs), (ins GPR:$rs1, GPR:$rs2,
                                      brtarget:$BrDst),
                             "bltu", "$rs1, $rs2, $BrDst", []>;
def RiscvToyBGEU : RiscvToyB<0b111, 0b1100011,
                             (outs), (ins GPR:$rs1, GPR:$rs2,
                                      brtarget:$BrDst),
                             "bgeu", "$rs1, $rs2, $BrDst", []>;
} // isBranch = 1, isTerminator = 1
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

let isBranch = 1, isBarrier = 1, isTerminator = 1, isCodeGenOnly = 1 in
def PseudoBR : RiscvToyPseudo<(outs), (ins brtarget:$BrDst), "j\t$BrDst",
                              [(br bb:$BrDst)]>;

// Setcc values are materialized as 0/1 in GPR. The patterns below cover the
// comparisons that have a single RISC-V instruction; the rest are rewritten
// with operand swaps and xor(x, 1) for logical negation.
def : Pat<(seteq GPR:$rs1, 0), (RiscvToySLTIU GPR:$rs1, 1)>;
def : Pat<(seteq GPR:$rs1, GPR:$rs2),
          (RiscvToySLTIU (RiscvToyXOR GPR:$rs1, GPR:$rs2), 1)>;
def : Pat<(setne GPR:$rs1, 0), (RiscvToySLTU X0, GPR:$rs1)>;
def : Pat<(setne GPR:$rs1, GPR:$rs2),
          (RiscvToySLTU X0, (RiscvToyXOR GPR:$rs1, GPR:$rs2))>;
def : Pat<(setlt GPR:$rs1, GPR:$rs2), (RiscvToySLT GPR:$rs1, GPR:$rs2)>;
def : Pat<(setult GPR:$rs1, GPR:$rs2), (RiscvToySLTU GPR:$rs1, GPR:$rs2)>;
def : Pat<(setgt GPR:$rs1, GPR:$rs2), (RiscvToySLT GPR:$rs2, GPR:$rs1)>;
def : Pat<(setugt GPR:$rs1, GPR:$rs2), (RiscvToySLTU GPR:$rs2, GPR:$rs1)>;
def : Pat<(setge GPR:$rs1, GPR:$rs2),
          (RiscvToyXORI (RiscvToySLT GPR:$rs1, GPR:$rs2), 1)>;
def : Pat<(setuge GPR:$rs1, GPR:$rs2),
          (RiscvToyXORI (RiscvToySLTU GPR:$rs1, GPR:$rs2), 1)>;
def : Pat<(setle GPR:$rs1, GPR:$rs2),
          (RiscvToyXORI (RiscvToySLT GPR:$rs2, GPR:$rs1), 1)>;
def : Pat<(setule GPR:$rs1, GPR:$rs2),
          (RiscvToyXORI (RiscvToySLTU GPR:$rs2, GPR:$rs1), 1)>;

// Materialize small signed constants as addi x0, imm. Values outside the
// signed 12-bit range will require LUI/AUIPC in a later MC/materialization
// stage.
def : Pat<(immSExt12:$imm), (RiscvToyADDI X0, immSExt12:$imm)>;

// Branch directly on an icmp result. Unsupported conditions are lowered by
// swapping rs1/rs2.
def : Pat<(brcond (i32 (seteq GPR:$rs1, GPR:$rs2)), bb:$BrDst),
          (RiscvToyBEQ GPR:$rs1, GPR:$rs2, bb:$BrDst)>;
def : Pat<(brcond (i32 (setne GPR:$rs1, GPR:$rs2)), bb:$BrDst),
          (RiscvToyBNE GPR:$rs1, GPR:$rs2, bb:$BrDst)>;
def : Pat<(brcond (i32 (setlt GPR:$rs1, GPR:$rs2)), bb:$BrDst),
          (RiscvToyBLT GPR:$rs1, GPR:$rs2, bb:$BrDst)>;
def : Pat<(brcond (i32 (setge GPR:$rs1, GPR:$rs2)), bb:$BrDst),
          (RiscvToyBGE GPR:$rs1, GPR:$rs2, bb:$BrDst)>;
def : Pat<(brcond (i32 (setult GPR:$rs1, GPR:$rs2)), bb:$BrDst),
          (RiscvToyBLTU GPR:$rs1, GPR:$rs2, bb:$BrDst)>;
def : Pat<(brcond (i32 (setuge GPR:$rs1, GPR:$rs2)), bb:$BrDst),
          (RiscvToyBGEU GPR:$rs1, GPR:$rs2, bb:$BrDst)>;
def : Pat<(brcond (i32 (setgt GPR:$rs1, GPR:$rs2)), bb:$BrDst),
          (RiscvToyBLT GPR:$rs2, GPR:$rs1, bb:$BrDst)>;
def : Pat<(brcond (i32 (setle GPR:$rs1, GPR:$rs2)), bb:$BrDst),
          (RiscvToyBGE GPR:$rs2, GPR:$rs1, bb:$BrDst)>;
def : Pat<(brcond (i32 (setugt GPR:$rs1, GPR:$rs2)), bb:$BrDst),
          (RiscvToyBLTU GPR:$rs2, GPR:$rs1, bb:$BrDst)>;
def : Pat<(brcond (i32 (setule GPR:$rs1, GPR:$rs2)), bb:$BrDst),
          (RiscvToyBGEU GPR:$rs2, GPR:$rs1, bb:$BrDst)>;

// A boolean may also be a phi/argument rather than an icmp result. RISC-V
// treats any non-zero i32 as true, so bne against x0 is the fallback.
def : Pat<(brcond GPR:$cond, bb:$BrDst),
          (RiscvToyBNE GPR:$cond, X0, bb:$BrDst)>;

let usesCustomInserter = 1, isCodeGenOnly = 1 in
def RiscvToySelectPseudo
    : RiscvToyPseudo<(outs GPR:$dst),
                     (ins GPR:$lhs, GPR:$rhs, i32imm:$cc, GPR:$truev,
                      GPR:$falsev),
                     "# Select $dst = $lhs $cc $rhs ? $truev : $falsev",
                     [(set GPR:$dst,
                       (RiscvToySelectCCFlag GPR:$lhs, GPR:$rhs,
                        (i32 imm:$cc), GPR:$truev, GPR:$falsev))]>;

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
