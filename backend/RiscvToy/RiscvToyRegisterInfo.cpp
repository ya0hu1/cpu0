//===-- RiscvToyRegisterInfo.cpp - RiscvToy register info impl -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RiscvToyRegisterInfo.h"
#include "RiscvToy.h"
#include "RiscvToyFrameLowering.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/ErrorHandling.h"

#define GET_REGINFO_TARGET_DESC
#include "RiscvToyGenRegisterInfo.inc"

using namespace llvm;

RiscvToyRegisterInfo::RiscvToyRegisterInfo()
    : RiscvToyGenRegisterInfo(RiscvToy::X1) {}

const MCPhysReg *
RiscvToyRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR_ILP32_SaveList;
}

const uint32_t *
RiscvToyRegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                           CallingConv::ID) const {
  return CSR_ILP32_RegMask;
}

const uint32_t *RiscvToyRegisterInfo::getNoPreservedMask() const {
  return CSR_NoRegs_RegMask;
}

BitVector RiscvToyRegisterInfo::getReservedRegs(
    const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());

  markSuperRegs(Reserved, RiscvToy::X0); // zero
  markSuperRegs(Reserved, RiscvToy::X2); // sp
  markSuperRegs(Reserved, RiscvToy::X3); // gp
  markSuperRegs(Reserved, RiscvToy::X4); // tp

  return Reserved;
}

bool RiscvToyRegisterInfo::isConstantPhysReg(MCRegister PhysReg) const {
  return PhysReg == RiscvToy::X0;
}

const TargetRegisterClass *
RiscvToyRegisterInfo::getPointerRegClass(const MachineFunction &MF,
                                         unsigned Kind) const {
  return &RiscvToy::GPRRegClass;
}

void RiscvToyRegisterInfo::eliminateFrameIndex(
    MachineBasicBlock::iterator MI, int SPAdj, unsigned FIOperandNum,
    RegScavenger *RS) const {
  assert(SPAdj == 0 && "Unexpected non-zero SP adjustment");

  MachineInstr &Instr = *MI;
  MachineFunction &MF = *Instr.getParent()->getParent();
  const TargetFrameLowering *TFI = MF.getSubtarget().getFrameLowering();

  int FrameIndex = Instr.getOperand(FIOperandNum).getIndex();
  Register FrameReg;
  int Offset = TFI->getFrameIndexReference(MF, FrameIndex, FrameReg).getFixed();
  Offset += Instr.getOperand(FIOperandNum + 1).getImm();

  // Our load/store and addi instructions only encode a signed 12-bit
  // immediate. Keeping this check explicit makes the teaching limit visible
  // instead of silently emitting invalid code.
  if (!isInt<12>(Offset))
    report_fatal_error("RiscvToy stack offset exceeds the 12-bit encoding");

  Instr.getOperand(FIOperandNum).ChangeToRegister(FrameReg, false);
  Instr.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset);
}

Register RiscvToyRegisterInfo::getFrameRegister(
    const MachineFunction &MF) const {
  return RiscvToy::X2;
}
