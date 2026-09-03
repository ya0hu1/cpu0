//===-- RiscvToyMCCodeEmitter.h - RiscvToy machine code emitter ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCVTOY_MCTARGETDESC_RISCVTOYMCCODEEMITTER_H
#define LLVM_LIB_TARGET_RISCVTOY_MCTARGETDESC_RISCVTOYMCCODEEMITTER_H

#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCFixup.h"

namespace llvm {

class MCContext;
class MCInstrInfo;
class MCOperand;
class MCSubtargetInfo;
class raw_ostream;

class RiscvToyMCCodeEmitter : public MCCodeEmitter {
  const MCInstrInfo &MCII;
  MCContext &Ctx;

public:
  RiscvToyMCCodeEmitter(const MCInstrInfo &MCII, MCContext &Ctx)
      : MCII(MCII), Ctx(Ctx) {}

  void encodeInstruction(const MCInst &MI, raw_ostream &OS,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;

  uint64_t getBinaryCodeForInstr(const MCInst &MI,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  unsigned getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

  unsigned getImmOpValue(const MCInst &MI, unsigned OpNo,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const;

  unsigned getBranchTargetOpValue(const MCInst &MI, unsigned OpNo,
                                  SmallVectorImpl<MCFixup> &Fixups,
                                  const MCSubtargetInfo &STI) const;

  void emitRawBinary(uint32_t Binary, raw_ostream &OS) const;

  void expandPseudoBR(const MCInst &MI, raw_ostream &OS,
                      SmallVectorImpl<MCFixup> &Fixups,
                      const MCSubtargetInfo &STI) const;

  void expandPseudoCALL(const MCInst &MI, raw_ostream &OS,
                        SmallVectorImpl<MCFixup> &Fixups,
                        const MCSubtargetInfo &STI) const;

  void expandPseudoRET(const MCInst &MI, raw_ostream &OS,
                       SmallVectorImpl<MCFixup> &Fixups,
                       const MCSubtargetInfo &STI) const;
};

} // namespace llvm

#endif
