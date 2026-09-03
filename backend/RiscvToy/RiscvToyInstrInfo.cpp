//===-- RiscvToyInstrInfo.cpp - RiscvToy instruction info impl -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RiscvToyInstrInfo.h"
#include "RiscvToy.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/Support/ErrorHandling.h"

#define GET_INSTRINFO_CTOR_DTOR
#include "RiscvToyGenInstrInfo.inc"

using namespace llvm;

RiscvToyInstrInfo::RiscvToyInstrInfo()
    : RiscvToyGenInstrInfo(RiscvToy::ADJCALLSTACKDOWN,
                           RiscvToy::ADJCALLSTACKUP),
      RI() {}

unsigned RiscvToyInstrInfo::isLoadFromStackSlot(const MachineInstr &MI,
                                                int &FrameIndex) const {
  if (MI.getOpcode() != RiscvToy::RiscvToyLW)
    return 0;
  if (MI.getOperand(1).isFI() && MI.getOperand(2).isImm() &&
      MI.getOperand(2).getImm() == 0) {
    FrameIndex = MI.getOperand(1).getIndex();
    return MI.getOperand(0).getReg();
  }
  return 0;
}

unsigned RiscvToyInstrInfo::isStoreToStackSlot(const MachineInstr &MI,
                                               int &FrameIndex) const {
  if (MI.getOpcode() != RiscvToy::RiscvToySW)
    return 0;
  if (MI.getOperand(1).isFI() && MI.getOperand(2).isImm() &&
      MI.getOperand(2).getImm() == 0) {
    FrameIndex = MI.getOperand(1).getIndex();
    return MI.getOperand(0).getReg();
  }
  return 0;
}

void RiscvToyInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                    MachineBasicBlock::iterator MI,
                                    const DebugLoc &DL, MCRegister DestReg,
                                    MCRegister SrcReg, bool KillSrc) const {
  if (!RiscvToy::GPRRegClass.contains(DestReg) ||
      !RiscvToy::GPRRegClass.contains(SrcReg))
    llvm_unreachable("Impossible reg-to-reg copy");

  BuildMI(MBB, MI, DL, get(RiscvToy::RiscvToyADDI), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc))
      .addImm(0);
}

void RiscvToyInstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
    bool IsKill, int FrameIndex, const TargetRegisterClass *RC,
    const TargetRegisterInfo *TRI) const {
  if (!RiscvToy::GPRFullRegClass.hasSubClassEq(RC))
    llvm_unreachable("Can only spill 32-bit integer registers");

  DebugLoc DL;
  if (MI != MBB.end())
    DL = MI->getDebugLoc();

  MachineFunction *MF = MBB.getParent();
  const MachineFrameInfo &MFI = MF->getFrameInfo();
  MachineMemOperand *MMO = MF->getMachineMemOperand(
      MachinePointerInfo::getFixedStack(*MF, FrameIndex),
      MachineMemOperand::MOStore, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlign(FrameIndex));

  BuildMI(MBB, MI, DL, get(RiscvToy::RiscvToySW))
      .addReg(SrcReg, getKillRegState(IsKill))
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addMemOperand(MMO);
}

void RiscvToyInstrInfo::loadRegFromStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register DestReg,
    int FrameIndex, const TargetRegisterClass *RC,
    const TargetRegisterInfo *TRI) const {
  if (!RiscvToy::GPRFullRegClass.hasSubClassEq(RC))
    llvm_unreachable("Can only load 32-bit integer registers");

  DebugLoc DL;
  if (MI != MBB.end())
    DL = MI->getDebugLoc();

  MachineFunction *MF = MBB.getParent();
  const MachineFrameInfo &MFI = MF->getFrameInfo();
  MachineMemOperand *MMO = MF->getMachineMemOperand(
      MachinePointerInfo::getFixedStack(*MF, FrameIndex),
      MachineMemOperand::MOLoad, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlign(FrameIndex));

  BuildMI(MBB, MI, DL, get(RiscvToy::RiscvToyLW), DestReg)
      .addFrameIndex(FrameIndex)
      .addImm(0)
      .addMemOperand(MMO);
}
