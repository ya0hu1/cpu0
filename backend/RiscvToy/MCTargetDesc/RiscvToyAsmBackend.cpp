//===-- RiscvToyAsmBackend.cpp - RiscvToy assembler backend --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RiscvToyAsmBackend.h"
#include "RiscvToyMCTargetDesc.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

const MCFixupKindInfo &RiscvToyAsmBackend::getFixupKindInfo(
    MCFixupKind Kind) const {
  const static MCFixupKindInfo Infos[] = {
      // name                         offset bits flags
      {"fixup_riscvtoy_branch", 0, 32, MCFixupKindInfo::FKF_IsPCRel},
      {"fixup_riscvtoy_jal", 0, 32, MCFixupKindInfo::FKF_IsPCRel},
  };
  static_assert(array_lengthof(Infos) == RiscvToy::NumTargetFixupKinds,
                "Fixup kind table is out of sync");

  if (Kind < FirstTargetFixupKind)
    return MCAsmBackend::getFixupKindInfo(Kind);

  assert(unsigned(Kind - FirstTargetFixupKind) < getNumFixupKinds() &&
         "Invalid kind");
  return Infos[Kind - FirstTargetFixupKind];
}

static uint64_t adjustFixupValue(const MCFixup &Fixup, uint64_t Value,
                                 MCContext &Ctx) {
  int64_t Offset = static_cast<int64_t>(Value);
    switch (Fixup.getTargetKind()) {
  default:
    llvm_unreachable("Unknown RiscvToy fixup kind");
  case FK_Data_1:
  case FK_Data_2:
  case FK_Data_4:
  case FK_Data_8:
    return Value;
  case RiscvToy::fixup_riscvtoy_branch:
    if (!isInt<13>(Offset))
      Ctx.reportError(Fixup.getLoc(), "branch target out of range");
    if (Offset & 0x1)
      Ctx.reportError(Fixup.getLoc(), "branch target must be 2-byte aligned");
    {
      unsigned Bit12 = (Offset >> 12) & 0x1;
      unsigned Bits10_5 = (Offset >> 5) & 0x3f;
      unsigned Bits4_1 = (Offset >> 1) & 0xf;
      unsigned Bit11 = (Offset >> 11) & 0x1;
      return (Bit12 << 31) | (Bits10_5 << 25) | (Bits4_1 << 8) |
             (Bit11 << 7);
    }
  case RiscvToy::fixup_riscvtoy_jal:
    if (!isInt<21>(Offset))
      Ctx.reportError(Fixup.getLoc(), "jump target out of range");
    if (Offset & 0x1)
      Ctx.reportError(Fixup.getLoc(), "jump target must be 2-byte aligned");
    {
      unsigned Bit20 = (Offset >> 20) & 0x1;
      unsigned Bits10_1 = (Offset >> 1) & 0x3ff;
      unsigned Bit11 = (Offset >> 11) & 0x1;
      unsigned Bits19_12 = (Offset >> 12) & 0xff;
      return (Bit20 << 31) | (Bits10_1 << 21) | (Bit11 << 20) |
             (Bits19_12 << 12);
    }
  }
}

void RiscvToyAsmBackend::applyFixup(
    const MCAssembler &Asm, const MCFixup &Fixup, const MCValue &Target,
    MutableArrayRef<char> Data, uint64_t Value, bool IsResolved,
    const MCSubtargetInfo *STI) const {
  MCFixupKind Kind = Fixup.getKind();
  if (Kind >= FirstLiteralRelocationKind)
    return;

  MCContext &Ctx = Asm.getContext();
  const MCFixupKindInfo &Info = getFixupKindInfo(Kind);
  if (!Value)
    return;

  Value = adjustFixupValue(Fixup, Value, Ctx);
  Value <<= Info.TargetOffset;

  unsigned Offset = Fixup.getOffset();
  unsigned NumBytes = alignTo(Info.TargetSize + Info.TargetOffset, 8) / 8;
  assert(Offset + NumBytes <= Data.size() && "Invalid fixup offset");

  for (unsigned I = 0; I != NumBytes; ++I)
    Data[Offset + I] |= static_cast<uint8_t>((Value >> (I * 8)) & 0xff);
}

bool RiscvToyAsmBackend::writeNopData(raw_ostream &OS,
                                      uint64_t Count) const {
  if (Count % 4 != 0)
    return false;

  const char Nop[4] = {0x13, 0x00, 0x00, 0x00}; // addi x0, x0, 0
  for (uint64_t I = 0; I != Count; I += 4)
    OS.write(Nop, 4);
  return true;
}

std::unique_ptr<MCObjectTargetWriter>
RiscvToyAsmBackend::createObjectTargetWriter() const {
  return createRiscvToyELFObjectWriter(OSABI, /*Is64Bit=*/false);
}

MCAsmBackend *llvm::createRiscvToyAsmBackend(
    const Target &T, const MCSubtargetInfo &STI, const MCRegisterInfo &MRI,
    const MCTargetOptions &Options) {
  uint8_t OSABI = MCELFObjectTargetWriter::getOSABI(STI.getTargetTriple().getOS());
  return new RiscvToyAsmBackend(OSABI);
}
