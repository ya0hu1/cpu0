//===-- RiscvToyMCAsmInfo.h - RiscvToy asm info -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCVTOY_MCTARGETDESC_RISCVTOYMCASMINFO_H
#define LLVM_LIB_TARGET_RISCVTOY_MCTARGETDESC_RISCVTOYMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {

class Triple;

class RiscvToyMCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit RiscvToyMCAsmInfo(const Triple &TT);
};

} // namespace llvm

#endif
