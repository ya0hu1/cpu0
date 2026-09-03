//===-- RiscvToy.h - Top-level interface for RiscvToy ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCVTOY_RISCVTOY_H
#define LLVM_LIB_TARGET_RISCVTOY_RISCVTOY_H

#include "MCTargetDesc/RiscvToyMCTargetDesc.h"
#include "llvm/Support/CodeGen.h"

namespace llvm {

class FunctionPass;
class RiscvToyTargetMachine;

FunctionPass *createRiscvToyISelDag(RiscvToyTargetMachine &TM,
                                    CodeGenOpt::Level OptLevel);

} // namespace llvm

#endif
