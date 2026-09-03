//===-- RiscvToyInstrInfo.h - RiscvToy instruction info --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCVTOY_RISCVTOYINSTRINFO_H
#define LLVM_LIB_TARGET_RISCVTOY_RISCVTOYINSTRINFO_H

#include "RiscvToyRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "RiscvToyGenInstrInfo.inc"

namespace llvm {

class RiscvToyInstrInfo : public RiscvToyGenInstrInfo {
  const RiscvToyRegisterInfo RI;

public:
  RiscvToyInstrInfo();

  const RiscvToyRegisterInfo &getRegisterInfo() const { return RI; }

  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                   const DebugLoc &DL, MCRegister DestReg, MCRegister SrcReg,
                   bool KillSrc) const override;
};

} // namespace llvm

#endif
