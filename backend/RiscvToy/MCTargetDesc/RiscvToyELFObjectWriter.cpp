//===-- RiscvToyELFObjectWriter.cpp - RiscvToy ELF writer -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RiscvToyFixupKinds.h"
#include "RiscvToyMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {

class RiscvToyELFObjectWriter : public MCELFObjectTargetWriter {
public:
  RiscvToyELFObjectWriter(uint8_t OSABI)
      : MCELFObjectTargetWriter(/*Is64Bit=*/false, OSABI, ELF::EM_RISCV,
                                /*HasRelocationAddend=*/true) {}

protected:
  unsigned getRelocType(MCContext &Ctx, const MCValue &Target,
                        const MCFixup &Fixup,
                        bool IsPCRel) const override {
    switch (Fixup.getTargetKind()) {
    default:
      Ctx.reportError(Fixup.getLoc(), "Unsupported RiscvToy relocation");
      return ELF::R_RISCV_NONE;
    case FK_Data_4:
      return ELF::R_RISCV_32;
    case FK_Data_8:
      return ELF::R_RISCV_64;
    case RiscvToy::fixup_riscvtoy_branch:
      return ELF::R_RISCV_BRANCH;
    case RiscvToy::fixup_riscvtoy_jal:
      return ELF::R_RISCV_JAL;
    }
  }
};

} // namespace

std::unique_ptr<MCObjectTargetWriter>
llvm::createRiscvToyELFObjectWriter(uint8_t OSABI, bool Is64Bit) {
  assert(!Is64Bit && "RiscvToy is a 32-bit teaching target");
  return std::make_unique<RiscvToyELFObjectWriter>(OSABI);
}
