//===-- RiscvToySubtarget.cpp - RiscvToy subtarget implementation --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RiscvToySubtarget.h"
#include "RiscvToy.h"
#include "RiscvToyTargetMachine.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "riscvtoy-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "RiscvToyGenSubtargetInfo.inc"

void RiscvToySubtarget::anchor() {}

RiscvToySubtarget::RiscvToySubtarget(const Triple &TT, StringRef CPU,
                                     StringRef FS,
                                     const RiscvToyTargetMachine &TM)
    : RiscvToyGenSubtargetInfo(TT, CPU, /*TuneCPU=*/CPU, FS),
      InstrInfo(), FrameLowering(), TLInfo(TM, *this) {
  std::string CPUName = CPU.empty() ? "generic-rv32" : CPU.str();
  ParseSubtargetFeatures(CPUName, CPUName, FS);
}
