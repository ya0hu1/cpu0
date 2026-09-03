//===-- RiscvToyFrameLowering.cpp - RiscvToy frame lowering impl ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RiscvToyFrameLowering.h"
#include "RiscvToy.h"
#include "RiscvToySubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include <algorithm>

using namespace llvm;

static void adjustStackPointer(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator MBBI,
                               const DebugLoc &DL, uint64_t Amount,
                               bool IsPrologue) {
  if (Amount == 0)
    return;

  MachineFunction &MF = *MBB.getParent();
  const TargetInstrInfo &TII = *MF.getSubtarget().getInstrInfo();
  MachineInstr::MIFlag Flag =
      IsPrologue ? MachineInstr::FrameSetup : MachineInstr::FrameDestroy;

  // ADDI only has a signed 12-bit immediate. Splitting the adjustment into
  // 2047-byte pieces keeps the emitted code simple and supports larger frames
  // without adding LUI/AUIPC yet.
  uint64_t Remaining = Amount;
  while (Remaining != 0) {
    int64_t Chunk = std::min<uint64_t>(Remaining, 2047);
    BuildMI(MBB, MBBI, DL, TII.get(RiscvToy::RiscvToyADDI), RiscvToy::X2)
        .addReg(RiscvToy::X2)
        .addImm(IsPrologue ? -Chunk : Chunk)
        .setMIFlag(Flag);
    Remaining -= Chunk;
  }
}

void RiscvToyFrameLowering::emitPrologue(MachineFunction &MF,
                                         MachineBasicBlock &MBB) const {
  uint64_t StackSize = MF.getFrameInfo().getStackSize();
  if (StackSize == 0)
    return;

  adjustStackPointer(MBB, MBB.begin(), DebugLoc(), StackSize,
                     /*IsPrologue=*/true);
}

void RiscvToyFrameLowering::emitEpilogue(MachineFunction &MF,
                                         MachineBasicBlock &MBB) const {
  uint64_t StackSize = MF.getFrameInfo().getStackSize();
  if (StackSize == 0)
    return;

  MachineBasicBlock::iterator MBBI = MBB.getFirstTerminator();
  if (MBBI == MBB.end())
    MBBI = MBB.getLastNonDebugInstr();

  DebugLoc DL;
  if (MBBI != MBB.end())
    DL = MBBI->getDebugLoc();

  adjustStackPointer(MBB, MBBI, DL, StackSize, /*IsPrologue=*/false);
}
