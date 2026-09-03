//===-- RiscvToyInstrInfo.cpp - RiscvToy instruction info impl -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RiscvToyInstrInfo.h"
#include "RiscvToy.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"

#define GET_INSTRINFO_CTOR_DTOR
#include "RiscvToyGenInstrInfo.inc"

using namespace llvm;

RiscvToyInstrInfo::RiscvToyInstrInfo() : RiscvToyGenInstrInfo(), RI() {}

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
