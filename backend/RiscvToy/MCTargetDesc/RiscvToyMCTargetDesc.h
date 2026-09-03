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
#include <memory>

namespace llvm {

class MCAsmBackend;
class MCAsmInfo;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCInstPrinter;
class MCObjectTargetWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class MCTargetOptions;
class Target;
class Triple;

} // namespace llvm

namespace llvm {

MCCodeEmitter *createRiscvToyMCCodeEmitter(const MCInstrInfo &MCII,
                                           const MCRegisterInfo &MRI,
                                           MCContext &Ctx);

MCAsmBackend *createRiscvToyAsmBackend(const Target &T,
                                       const MCSubtargetInfo &STI,
                                       const MCRegisterInfo &MRI,
                                       const MCTargetOptions &Options);

std::unique_ptr<MCObjectTargetWriter>
createRiscvToyELFObjectWriter(uint8_t OSABI, bool Is64Bit);

} // namespace llvm

// Register and instruction enums are shared by CodeGen, MC, and the AsmWriter.
#define GET_REGINFO_ENUM
#include "RiscvToyGenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#include "RiscvToyGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "RiscvToyGenSubtargetInfo.inc"

#endif
