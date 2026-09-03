//===-- RiscvToyTargetMachine.h - RiscvToy target machine ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCVTOY_RISCVTOYTARGETMACHINE_H
#define LLVM_LIB_TARGET_RISCVTOY_RISCVTOYTARGETMACHINE_H

#include "llvm/IR/DataLayout.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class RiscvToyTargetMachine : public LLVMTargetMachine {
public:
  RiscvToyTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                        StringRef FS, const TargetOptions &Options,
                        Optional<Reloc::Model> RM, Optional<CodeModel::Model> CM,
                        CodeGenOpt::Level OL, bool JIT);

  // Stage 1 intentionally has no register model, instruction table, or MC
  // layer, so report unsupported instead of entering CodeGen.
  bool addPassesToEmitFile(
      legacy::PassManagerBase &PM, raw_pwrite_stream &Out,
      raw_pwrite_stream *DwoOut, CodeGenFileType FileType,
      bool DisableVerify, MachineModuleInfoWrapperPass *MMIWP) override;
};

} // namespace llvm

#endif
