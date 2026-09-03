//===-- RiscvToySubtarget.h - RiscvToy subtarget interface ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCVTOY_RISCVTOYSUBTARGET_H
#define LLVM_LIB_TARGET_RISCVTOY_RISCVTOYSUBTARGET_H

#include "RiscvToyFrameLowering.h"
#include "RiscvToyISelLowering.h"
#include "RiscvToyInstrInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/DataLayout.h"

#define GET_SUBTARGETINFO_HEADER
#include "RiscvToyGenSubtargetInfo.inc"

namespace llvm {

class RiscvToyTargetMachine;

class RiscvToySubtarget : public RiscvToyGenSubtargetInfo {
  virtual void anchor();

  bool HasRV32I = true;

  RiscvToyInstrInfo InstrInfo;
  RiscvToyFrameLowering FrameLowering;
  RiscvToyTargetLowering TLInfo;

public:
  RiscvToySubtarget(const Triple &TT, StringRef CPU, StringRef FS,
                    const RiscvToyTargetMachine &TM);

  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  bool hasRV32I() const { return HasRV32I; }

  const RiscvToyInstrInfo *getInstrInfo() const override {
    return &InstrInfo;
  }

  const RiscvToyFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }

  const RiscvToyTargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }

  const RiscvToyRegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }
};

} // namespace llvm

#endif
