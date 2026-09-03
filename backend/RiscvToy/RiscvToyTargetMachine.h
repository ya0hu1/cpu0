//===-- RiscvToyTargetMachine.h - RiscvToy target machine ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCVTOY_RISCVTOYTARGETMACHINE_H
#define LLVM_LIB_TARGET_RISCVTOY_RISCVTOYTARGETMACHINE_H

#include "RiscvToySubtarget.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class TargetLoweringObjectFile;

class RiscvToyTargetMachine : public LLVMTargetMachine {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  RiscvToySubtarget DefaultSubtarget;

public:
  RiscvToyTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                        StringRef FS, const TargetOptions &Options,
                        Optional<Reloc::Model> RM, Optional<CodeModel::Model> CM,
                        CodeGenOpt::Level OL, bool JIT);

  const RiscvToySubtarget *getSubtargetImpl() const {
    return &DefaultSubtarget;
  }

  const RiscvToySubtarget *getSubtargetImpl(const Function &F) const override {
    return &DefaultSubtarget;
  }

  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
};

} // namespace llvm

#endif
