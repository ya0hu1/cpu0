//===-- RiscvToyTargetInfo.h - RiscvToy target info header ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCVTOY_TARGETINFO_RISCVTOYTARGETINFO_H
#define LLVM_LIB_TARGET_RISCVTOY_TARGETINFO_RISCVTOYTARGETINFO_H

namespace llvm {

class Target;

Target &getTheRiscvToyTarget();

} // namespace llvm

#endif
