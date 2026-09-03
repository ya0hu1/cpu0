//===-- RiscvToyAsmBackend.h - RiscvToy assembler backend ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCVTOY_MCTARGETDESC_RISCVTOYASMBACKEND_H
#define LLVM_LIB_TARGET_RISCVTOY_MCTARGETDESC_RISCVTOYASMBACKEND_H

#include "RiscvToyFixupKinds.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"

namespace llvm {

class MCAsmLayout;
class MCObjectTargetWriter;
class MCRelaxableFragment;

class RiscvToyAsmBackend : public MCAsmBackend {
  uint8_t OSABI;

public:
  RiscvToyAsmBackend(uint8_t OSABI)
      : MCAsmBackend(support::little), OSABI(OSABI) {}

  unsigned getNumFixupKinds() const override {
    return RiscvToy::NumTargetFixupKinds;
  }

  const MCFixupKindInfo &getFixupKindInfo(MCFixupKind Kind) const override;

  void applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                  const MCValue &Target, MutableArrayRef<char> Data,
                  uint64_t Value, bool IsResolved,
                  const MCSubtargetInfo *STI) const override;

  bool writeNopData(raw_ostream &OS, uint64_t Count) const override;

  bool fixupNeedsRelaxation(const MCFixup &Fixup, uint64_t Value,
                            const MCRelaxableFragment *DF,
                            const MCAsmLayout &Layout) const override {
    return false;
  }

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override;
};

} // namespace llvm

#endif
