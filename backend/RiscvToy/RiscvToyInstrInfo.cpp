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

static void parseCondBranch(MachineInstr &MI, MachineBasicBlock *&Target,
                            SmallVectorImpl<MachineOperand> &Cond) {
  assert(MI.getDesc().isConditionalBranch() && "Unknown conditional branch");
  Target = MI.getOperand(2).getMBB();
  Cond.push_back(MachineOperand::CreateImm(MI.getOpcode()));
  Cond.push_back(MI.getOperand(0));
  Cond.push_back(MI.getOperand(1));
}

static unsigned getOppositeBranchOpcode(unsigned Opcode) {
  switch (Opcode) {
  default:
    llvm_unreachable("Unrecognized conditional branch");
  case RiscvToy::RiscvToyBEQ:
    return RiscvToy::RiscvToyBNE;
  case RiscvToy::RiscvToyBNE:
    return RiscvToy::RiscvToyBEQ;
  case RiscvToy::RiscvToyBLT:
    return RiscvToy::RiscvToyBGE;
  case RiscvToy::RiscvToyBGE:
    return RiscvToy::RiscvToyBLT;
  case RiscvToy::RiscvToyBLTU:
    return RiscvToy::RiscvToyBGEU;
  case RiscvToy::RiscvToyBGEU:
    return RiscvToy::RiscvToyBLTU;
  }
}

bool RiscvToyInstrInfo::analyzeBranch(
    MachineBasicBlock &MBB, MachineBasicBlock *&TBB, MachineBasicBlock *&FBB,
    SmallVectorImpl<MachineOperand> &Cond, bool AllowModify) const {
  TBB = FBB = nullptr;
  Cond.clear();

  MachineBasicBlock::iterator I = MBB.getLastNonDebugInstr();
  if (I == MBB.end() || !isUnpredicatedTerminator(*I))
    return false;

  int NumTerminators = 0;
  for (auto J = I.getReverse(); J != MBB.rend() && isUnpredicatedTerminator(*J);
       ++J) {
    NumTerminators++;
  }

  if (NumTerminators > 2)
    return true;

  // A single unconditional branch.
  if (NumTerminators == 1 && I->getDesc().isUnconditionalBranch()) {
    TBB = getBranchDestBlock(*I);
    return false;
  }

  // A single conditional branch.
  if (NumTerminators == 1 && I->getDesc().isConditionalBranch()) {
    parseCondBranch(*I, TBB, Cond);
    return false;
  }

  // A conditional branch followed by an unconditional branch.
  MachineBasicBlock::iterator Prev = std::prev(I);
  if (NumTerminators == 2 && Prev->getDesc().isConditionalBranch() &&
      I->getDesc().isUnconditionalBranch()) {
    parseCondBranch(*Prev, TBB, Cond);
    FBB = getBranchDestBlock(*I);
    return false;
  }

  return true;
}

unsigned RiscvToyInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                         int *BytesRemoved) const {
  if (BytesRemoved)
    *BytesRemoved = 0;

  MachineBasicBlock::iterator I = MBB.getLastNonDebugInstr();
  if (I == MBB.end() || !I->isBranch())
    return 0;

  I->eraseFromParent();
  I = MBB.end();

  unsigned Removed = 1;
  if (I == MBB.begin())
    return Removed;

  --I;
  if (!I->isBranch())
    return Removed;

  I->eraseFromParent();
  return ++Removed;
}

unsigned RiscvToyInstrInfo::insertBranch(
    MachineBasicBlock &MBB, MachineBasicBlock *TBB, MachineBasicBlock *FBB,
    ArrayRef<MachineOperand> Cond, const DebugLoc &DL,
    int *BytesAdded) const {
  if (BytesAdded)
    *BytesAdded = 0;

  assert(TBB && "insertBranch must not be told to insert a fallthrough");

  if (Cond.empty()) {
    BuildMI(&MBB, DL, get(RiscvToy::PseudoBR)).addMBB(TBB);
    return 1;
  }

  assert(Cond.size() == 3 && "RiscvToy branch conditions have 3 operands");
  unsigned Opcode = Cond[0].getImm();
  BuildMI(&MBB, DL, get(Opcode))
      .add(Cond[1])
      .add(Cond[2])
      .addMBB(TBB);

  if (!FBB)
    return 1;

  BuildMI(&MBB, DL, get(RiscvToy::PseudoBR)).addMBB(FBB);
  return 2;
}

bool RiscvToyInstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  assert(Cond.size() == 3 && "Invalid branch condition");
  Cond[0].setImm(getOppositeBranchOpcode(Cond[0].getImm()));
  return false;
}

MachineBasicBlock *RiscvToyInstrInfo::getBranchDestBlock(
    const MachineInstr &MI) const {
  assert(MI.getDesc().isBranch() && "Unexpected opcode");
  unsigned NumOps = MI.getNumExplicitOperands();
  return MI.getOperand(NumOps - 1).getMBB();
}
