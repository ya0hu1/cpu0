//===-- RiscvToyFixupKinds.h - RiscvToy fixup kinds ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCVTOY_MCTARGETDESC_RISCVTOYFIXUPKINDS_H
#define LLVM_LIB_TARGET_RISCVTOY_MCTARGETDESC_RISCVTOYFIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace RiscvToy {

enum Fixups {
  fixup_riscvtoy_branch = FirstTargetFixupKind,
  fixup_riscvtoy_jal,

  fixup_riscvtoy_invalid,
  NumTargetFixupKinds = fixup_riscvtoy_invalid - FirstTargetFixupKind
};

} // namespace RiscvToy
} // namespace llvm

#endif
