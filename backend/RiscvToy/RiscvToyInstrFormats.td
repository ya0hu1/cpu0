//===-- RiscvToyInstrFormats.td - RiscvToy instruction formats -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file describes the 32-bit RV32I encoding layouts used by RiscvToy.
// Each format class is a small teaching slice of the RISC-V specification.
//
//===----------------------------------------------------------------------===//

class RiscvToyInst<dag outs, dag ins, string asmstr, list<dag> pattern>
    : Instruction {
  field bits<32> Inst;
  field bits<32> SoftFail = 0;

  let Size = 4;
  let Namespace = "RiscvToy";

  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = pattern;
}

class RiscvToyPseudo<dag outs, dag ins, string asmstr, list<dag> pattern>
    : RiscvToyInst<outs, ins, asmstr, pattern> {
  let isPseudo = 1;
  let isCodeGenOnly = 1;
}

// R-type: funct7 | rs2 | rs1 | funct3 | rd | opcode
class RiscvToyR<bits<7> funct7, bits<3> funct3, bits<7> opcode,
                dag outs, dag ins, string opcodestr, string argstr,
                list<dag> pattern>
    : RiscvToyInst<outs, ins, opcodestr # "\t" # argstr, pattern> {
  bits<5> rs2;
  bits<5> rs1;
  bits<5> rd;

  let Inst{31-25} = funct7;
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7} = rd;
  let Inst{6-0} = opcode;
}

// I-type: imm12 | rs1 | funct3 | rd | opcode
class RiscvToyI<bits<3> funct3, bits<7> opcode,
                dag outs, dag ins, string opcodestr, string argstr,
                list<dag> pattern>
    : RiscvToyInst<outs, ins, opcodestr # "\t" # argstr, pattern> {
  bits<12> imm12;
  bits<5> rs1;
  bits<5> rd;

  let Inst{31-20} = imm12;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7} = rd;
  let Inst{6-0} = opcode;
}

// S-type: imm12[11:5] | rs2 | rs1 | funct3 | imm12[4:0] | opcode
class RiscvToyS<bits<3> funct3, bits<7> opcode,
                dag outs, dag ins, string opcodestr, string argstr,
                list<dag> pattern>
    : RiscvToyInst<outs, ins, opcodestr # "\t" # argstr, pattern> {
  bits<12> imm12;
  bits<5> rs2;
  bits<5> rs1;

  let Inst{31-25} = imm12{11-5};
  let Inst{24-20} = rs2;
  let Inst{19-15} = rs1;
  let Inst{14-12} = funct3;
  let Inst{11-7} = imm12{4-0};
  let Inst{6-0} = opcode;
}
