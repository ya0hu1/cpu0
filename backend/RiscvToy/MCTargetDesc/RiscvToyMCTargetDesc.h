//===-- RiscvToyMCTargetDesc.h - RiscvToy target descriptions --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCVTOY_MCTARGETDESC_RISCVTOYMCTARGETDESC_H
#define LLVM_LIB_TARGET_RISCVTOY_MCTARGETDESC_RISCVTOYMCTARGETDESC_H

#include "llvm/Support/DataTypes.h"

namespace llvm {

class MCAsmInfo;
class MCInstrInfo;
class MCInstPrinter;
class MCRegisterInfo;
class MCSubtargetInfo;
class Target;
class Triple;

} // namespace llvm

// Register and instruction enums are shared by CodeGen, MC, and the AsmWriter.
#define GET_REGINFO_ENUM
#include "RiscvToyGenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#include "RiscvToyGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "RiscvToyGenSubtargetInfo.inc"

#endif
