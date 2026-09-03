//===-- RiscvToyAsmPrinter.cpp - RiscvToy assembly printer impl ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RiscvToyAsmPrinter.h"
#include "MCTargetDesc/RiscvToyInstPrinter.h"
#include "RiscvToy.h"
#include "TargetInfo/RiscvToyTargetInfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

static MCOperand lowerMachineOperand(const MachineOperand &MO) {
  switch (MO.getType()) {
  case MachineOperand::MO_Register:
    return MCOperand::createReg(MO.getReg());
  case MachineOperand::MO_Immediate:
    return MCOperand::createImm(MO.getImm());
  default:
    report_fatal_error("RiscvToy does not support this operand yet");
  }
}

void RiscvToyAsmPrinter::emitInstruction(const MachineInstr *MI) {
  MCInst Inst;

  if (MI->getOpcode() == RiscvToy::PseudoRET) {
    Inst.setOpcode(RiscvToy::RiscvToyJALR);
    Inst.addOperand(MCOperand::createReg(RiscvToy::X0));
    Inst.addOperand(MCOperand::createReg(RiscvToy::X1));
    Inst.addOperand(MCOperand::createImm(0));
  } else {
    Inst.setOpcode(MI->getOpcode());
    for (const MachineOperand &MO : MI->explicit_operands())
      Inst.addOperand(lowerMachineOperand(MO));
  }

  EmitToStreamer(*OutStreamer, Inst);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRiscvToyAsmPrinter() {
  RegisterAsmPrinter<RiscvToyAsmPrinter> X(getTheRiscvToyTarget());
}
