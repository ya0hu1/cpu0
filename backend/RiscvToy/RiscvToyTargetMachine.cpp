//===-- RiscvToyTargetMachine.cpp - RiscvToy target machine --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RiscvToyTargetMachine.h"
#include "TargetInfo/RiscvToyTargetInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"
using namespace llvm;

static Reloc::Model getEffectiveRelocModel(Optional<Reloc::Model> RM) {
  if (RM.hasValue())
    return *RM;
  return Reloc::Static;
}

RiscvToyTargetMachine::RiscvToyTargetMachine(
    const Target &T, const Triple &TT, StringRef CPU, StringRef FS,
    const TargetOptions &Options, Optional<Reloc::Model> RM,
    Optional<CodeModel::Model> CM, CodeGenOpt::Level OL, bool JIT)
    : LLVMTargetMachine(T, "e-m:e-p:32:32-i64:64-n32-S128", TT, CPU, FS,
                        Options, getEffectiveRelocModel(RM),
                        CM.hasValue() ? *CM : CodeModel::Small, OL) {}

bool RiscvToyTargetMachine::addPassesToEmitFile(
    legacy::PassManagerBase &PM, raw_pwrite_stream &Out,
    raw_pwrite_stream *DwoOut, CodeGenFileType FileType, bool DisableVerify,
    MachineModuleInfoWrapperPass *MMIWP) {
  return true;
}

extern "C" void LLVMInitializeRiscvToyTarget() {
  RegisterTargetMachine<RiscvToyTargetMachine> X(getTheRiscvToyTarget());
}

extern "C" void LLVMInitializeRiscvToyTargetMC() {}
