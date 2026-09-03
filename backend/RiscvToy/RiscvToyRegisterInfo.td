//===-- RiscvToyRegisterInfo.td - RISC-V toy registers -----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file models RV32I integer registers only. FP registers are intentionally
// left for a later stage.
//
//===----------------------------------------------------------------------===//

def ABIRegAltName : RegAltNameIndex;

let Namespace = "RiscvToy" in {
class RiscvToyReg<bits<5> Enc, string n, list<string> alt = []>
    : Register<n> {
  let HWEncoding{4-0} = Enc;
  let AltNames = alt;
}
} // namespace RiscvToy

let RegAltNameIndices = [ABIRegAltName],
    Namespace = "RiscvToy" in {
  def X0  : RiscvToyReg<0,  "x0",  ["zero"]>, DwarfRegNum<[0]>;
  def X1  : RiscvToyReg<1,  "x1",  ["ra"]>,   DwarfRegNum<[1]>;
  def X2  : RiscvToyReg<2,  "x2",  ["sp"]>,   DwarfRegNum<[2]>;
  def X3  : RiscvToyReg<3,  "x3",  ["gp"]>,   DwarfRegNum<[3]>;
  def X4  : RiscvToyReg<4,  "x4",  ["tp"]>,   DwarfRegNum<[4]>;
  def X5  : RiscvToyReg<5,  "x5",  ["t0"]>,   DwarfRegNum<[5]>;
  def X6  : RiscvToyReg<6,  "x6",  ["t1"]>,   DwarfRegNum<[6]>;
  def X7  : RiscvToyReg<7,  "x7",  ["t2"]>,   DwarfRegNum<[7]>;
  def X8  : RiscvToyReg<8,  "x8",  ["s0", "fp"]>, DwarfRegNum<[8]>;
  def X9  : RiscvToyReg<9,  "x9",  ["s1"]>,   DwarfRegNum<[9]>;
  def X10 : RiscvToyReg<10, "x10", ["a0"]>,   DwarfRegNum<[10]>;
  def X11 : RiscvToyReg<11, "x11", ["a1"]>,   DwarfRegNum<[11]>;
  def X12 : RiscvToyReg<12, "x12", ["a2"]>,   DwarfRegNum<[12]>;
  def X13 : RiscvToyReg<13, "x13", ["a3"]>,   DwarfRegNum<[13]>;
  def X14 : RiscvToyReg<14, "x14", ["a4"]>,   DwarfRegNum<[14]>;
  def X15 : RiscvToyReg<15, "x15", ["a5"]>,   DwarfRegNum<[15]>;
  def X16 : RiscvToyReg<16, "x16", ["a6"]>,   DwarfRegNum<[16]>;
  def X17 : RiscvToyReg<17, "x17", ["a7"]>,   DwarfRegNum<[17]>;
  def X18 : RiscvToyReg<18, "x18", ["s2"]>,   DwarfRegNum<[18]>;
  def X19 : RiscvToyReg<19, "x19", ["s3"]>,   DwarfRegNum<[19]>;
  def X20 : RiscvToyReg<20, "x20", ["s4"]>,   DwarfRegNum<[20]>;
  def X21 : RiscvToyReg<21, "x21", ["s5"]>,   DwarfRegNum<[21]>;
  def X22 : RiscvToyReg<22, "x22", ["s6"]>,   DwarfRegNum<[22]>;
  def X23 : RiscvToyReg<23, "x23", ["s7"]>,   DwarfRegNum<[23]>;
  def X24 : RiscvToyReg<24, "x24", ["s8"]>,   DwarfRegNum<[24]>;
  def X25 : RiscvToyReg<25, "x25", ["s9"]>,   DwarfRegNum<[25]>;
  def X26 : RiscvToyReg<26, "x26", ["s10"]>,  DwarfRegNum<[26]>;
  def X27 : RiscvToyReg<27, "x27", ["s11"]>,  DwarfRegNum<[27]>;
  def X28 : RiscvToyReg<28, "x28", ["t3"]>,   DwarfRegNum<[28]>;
  def X29 : RiscvToyReg<29, "x29", ["t4"]>,   DwarfRegNum<[29]>;
  def X30 : RiscvToyReg<30, "x30", ["t5"]>,   DwarfRegNum<[30]>;
  def X31 : RiscvToyReg<31, "x31", ["t6"]>,   DwarfRegNum<[31]>;
}

// GPR is the normal allocatable class. The order intentionally puts
// caller-saved argument/temp registers first, then callee-saved registers,
// then the specials.
def GPR : RegisterClass<"RiscvToy", [i32], 32, (add
    (sequence "X%u", 10, 17),
    (sequence "X%u", 5, 7),
    (sequence "X%u", 28, 31),
    (sequence "X%u", 8, 9),
    (sequence "X%u", 18, 27)
  )>;

// X0 is not allocatable because reads return 0 and writes are discarded.
// Keep it available for patterns that explicitly emit X0.
def GPRX0 : RegisterClass<"RiscvToy", [i32], 32, (add X0)>;

// GPRNoX0 also removes SP/GP/TP, which CodeGen must manage separately.
def GPRNoX0 : RegisterClass<"RiscvToy", [i32], 32, (add
    (sequence "X%u", 10, 17),
    (sequence "X%u", 5, 7),
    (sequence "X%u", 28, 31),
    (sequence "X%u", 8, 9),
    (sequence "X%u", 18, 27),
    X1
  )>;

// SP is a singleton class used by frame/stack lowering.
def SP : RegisterClass<"RiscvToy", [i32], 32, (add X2)>;

// Callee-saved registers follow the standard RV32I ILP32 convention:
// ra, gp, tp, s0-s11.
def CSR_ILP32 : CalleeSavedRegs<(add X1, X3, X4, X8, X9,
                                 (sequence "X%u", 18, 27))>;
