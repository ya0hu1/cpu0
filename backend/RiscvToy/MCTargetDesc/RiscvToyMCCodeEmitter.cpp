//===-- RiscvToyMCCodeEmitter.cpp - RiscvToy machine code emitter --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RiscvToyMCCodeEmitter.h"
#include "RiscvToyFixupKinds.h"
#include "RiscvToyMCTargetDesc.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

MCCodeEmitter *llvm::createRiscvToyMCCodeEmitter(
    const MCInstrInfo &MCII, const MCRegisterInfo &MRI, MCContext &Ctx) {
  return new RiscvToyMCCodeEmitter(MCII, Ctx);
}

void RiscvToyMCCodeEmitter::emitRawBinary(uint32_t Binary,
                                          raw_ostream &OS) const {
  support::endian::write(OS, Binary, support::little);
}

void RiscvToyMCCodeEmitter::expandPseudoBR(
    const MCInst &MI, raw_ostream &OS, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {
  assert(MI.getOpcode() == RiscvToy::PseudoBR && "Expected PseudoBR");
  MCInst JAL;
  JAL.setOpcode(RiscvToy::RiscvToyJAL);
  JAL.addOperand(MCOperand::createReg(RiscvToy::X0));
  JAL.addOperand(MI.getOperand(0));
  emitRawBinary(getBinaryCodeForInstr(JAL, Fixups, STI), OS);
}

void RiscvToyMCCodeEmitter::expandPseudoCALL(
    const MCInst &MI, raw_ostream &OS, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {
  assert(MI.getOpcode() == RiscvToy::PseudoCALL && "Expected PseudoCALL");
  MCInst JAL;
  JAL.setOpcode(RiscvToy::RiscvToyJAL);
  JAL.addOperand(MCOperand::createReg(RiscvToy::X1));
  JAL.addOperand(MI.getOperand(0));
  emitRawBinary(getBinaryCodeForInstr(JAL, Fixups, STI), OS);
}

void RiscvToyMCCodeEmitter::expandPseudoRET(
    const MCInst &MI, raw_ostream &OS, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {
  assert(MI.getOpcode() == RiscvToy::PseudoRET && "Expected PseudoRET");
  MCInst JALR;
  JALR.setOpcode(RiscvToy::RiscvToyJALR);
  JALR.addOperand(MCOperand::createReg(RiscvToy::X0));
  JALR.addOperand(MCOperand::createReg(RiscvToy::X1));
  JALR.addOperand(MCOperand::createImm(0));
  emitRawBinary(getBinaryCodeForInstr(JALR, Fixups, STI), OS);
}

void RiscvToyMCCodeEmitter::encodeInstruction(
    const MCInst &MI, raw_ostream &OS, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {
  switch (MI.getOpcode()) {
  case RiscvToy::PseudoBR:
    expandPseudoBR(MI, OS, Fixups, STI);
    return;
  case RiscvToy::PseudoCALL:
    expandPseudoCALL(MI, OS, Fixups, STI);
    return;
  case RiscvToy::PseudoRET:
    expandPseudoRET(MI, OS, Fixups, STI);
    return;
  default:
    break;
  }

  uint32_t Binary = getBinaryCodeForInstr(MI, Fixups, STI);
  emitRawBinary(Binary, OS);
}

unsigned RiscvToyMCCodeEmitter::getMachineOpValue(
    const MCInst &MI, const MCOperand &MO, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {
  if (MO.isReg())
    return Ctx.getRegisterInfo()->getEncodingValue(MO.getReg());
  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm());
  llvm_unreachable("Unhandled machine operand in code emitter");
}

unsigned RiscvToyMCCodeEmitter::getImmOpValue(
    const MCInst &MI, unsigned OpNo, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpNo);
  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm());
  llvm_unreachable("RiscvToy does not support symbolic immediates yet");
}

unsigned RiscvToyMCCodeEmitter::getBranchTargetOpValue(
    const MCInst &MI, unsigned OpNo, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpNo);
  if (MO.isImm())
    return static_cast<unsigned>(MO.getImm() >> 1);

  assert(MO.isExpr() && "Expected an expression for a branch target");
  MCFixupKind Kind = MI.getOpcode() == RiscvToy::RiscvToyJAL
                         ? MCFixupKind(RiscvToy::fixup_riscvtoy_jal)
                         : MCFixupKind(RiscvToy::fixup_riscvtoy_branch);
  Fixups.push_back(MCFixup::create(0, MO.getExpr(), Kind, MI.getLoc()));
  return 0;
}

#include "RiscvToyGenMCCodeEmitter.inc"
