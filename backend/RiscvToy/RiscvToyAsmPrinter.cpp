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
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

static MCOperand lowerSymbolOperand(const MachineOperand &MO,
                                    const AsmPrinter &AP) {
  const MCSymbol *Symbol;
  if (MO.getType() == MachineOperand::MO_GlobalAddress)
    Symbol = AP.getSymbol(MO.getGlobal());
  else if (MO.getType() == MachineOperand::MO_ExternalSymbol)
    Symbol = AP.GetExternalSymbolSymbol(MO.getSymbolName());
  else
    llvm_unreachable("Unknown symbolic operand");

  const MCExpr *Expr = MCSymbolRefExpr::create(Symbol, AP.OutContext);
  if (MO.getOffset())
    Expr = MCBinaryExpr::createAdd(
        Expr, MCConstantExpr::create(MO.getOffset(), AP.OutContext),
        AP.OutContext);
  return MCOperand::createExpr(Expr);
}

static MCOperand lowerMachineOperand(const MachineOperand &MO,
                                     const AsmPrinter &AP) {
  switch (MO.getType()) {
  case MachineOperand::MO_Register:
    return MCOperand::createReg(MO.getReg());
  case MachineOperand::MO_Immediate:
    return MCOperand::createImm(MO.getImm());
  case MachineOperand::MO_GlobalAddress:
  case MachineOperand::MO_ExternalSymbol:
    return lowerSymbolOperand(MO, AP);
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
      Inst.addOperand(lowerMachineOperand(MO, *this));
  }

  EmitToStreamer(*OutStreamer, Inst);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRiscvToyAsmPrinter() {
  RegisterAsmPrinter<RiscvToyAsmPrinter> X(getTheRiscvToyTarget());
}
