//===-- RiscvToyTargetInfo.cpp - RiscvToy target implementation -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/RiscvToyTargetInfo.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

Target &llvm::getTheRiscvToyTarget() {
  static Target TheRiscvToyTarget;
  return TheRiscvToyTarget;
}

extern "C" void LLVMInitializeRiscvToyTargetInfo() {
  RegisterTarget<Triple::riscv32> X(
      getTheRiscvToyTarget(), "riscvtoy", "Educational RV32 backend",
      "RiscvToy");
}
