//===-- RiscvToyMCTargetDesc.cpp - RiscvToy target descriptions ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RiscvToyMCTargetDesc.h"
#include "RiscvToyInstPrinter.h"
#include "RiscvToyMCAsmInfo.h"
#include "TargetInfo/RiscvToyTargetInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/TargetRegistry.h"

#define GET_INSTRINFO_MC_DESC
#include "RiscvToyGenInstrInfo.inc"

#define GET_REGINFO_MC_DESC
#include "RiscvToyGenRegisterInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "RiscvToyGenSubtargetInfo.inc"

using namespace llvm;

static MCInstrInfo *createRiscvToyMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitRiscvToyMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createRiscvToyMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitRiscvToyMCRegisterInfo(X, RiscvToy::X1);
  return X;
}

static MCSubtargetInfo *createRiscvToyMCSubtargetInfo(const Triple &TT,
                                                      StringRef CPU,
                                                      StringRef FS) {
  std::string CPUName = CPU.empty() ? "generic-rv32" : CPU.str();
  return createRiscvToyMCSubtargetInfoImpl(TT, CPUName, CPUName, FS);
}

static MCAsmInfo *createRiscvToyMCAsmInfo(const MCRegisterInfo &MRI,
                                          const Triple &TT,
                                          const MCTargetOptions &Options) {
  return new RiscvToyMCAsmInfo(TT);
}

static MCInstPrinter *createRiscvToyMCInstPrinter(
    const Triple &T, unsigned SyntaxVariant, const MCAsmInfo &MAI,
    const MCInstrInfo &MII, const MCRegisterInfo &MRI) {
  return new RiscvToyInstPrinter(MAI, MII, MRI);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRiscvToyTargetMC() {
  Target &T = getTheRiscvToyTarget();
  TargetRegistry::RegisterMCAsmInfo(T, createRiscvToyMCAsmInfo);
  TargetRegistry::RegisterMCInstrInfo(T, createRiscvToyMCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(T, createRiscvToyMCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(T, createRiscvToyMCSubtargetInfo);
  TargetRegistry::RegisterMCAsmBackend(T, createRiscvToyAsmBackend);
  TargetRegistry::RegisterMCCodeEmitter(T, createRiscvToyMCCodeEmitter);
  TargetRegistry::RegisterMCInstPrinter(T, createRiscvToyMCInstPrinter);
}
