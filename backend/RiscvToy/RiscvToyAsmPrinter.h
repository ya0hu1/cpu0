//===-- RiscvToyAsmPrinter.h - RiscvToy assembly printer -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCVTOY_RISCVTOYASMPRINTER_H
#define LLVM_LIB_TARGET_RISCVTOY_RISCVTOYASMPRINTER_H

#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/MC/MCStreamer.h"

namespace llvm {

class RiscvToyAsmPrinter : public AsmPrinter {
public:
  explicit RiscvToyAsmPrinter(TargetMachine &TM,
                              std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)) {}

  StringRef getPassName() const override {
    return "RiscvToy Assembly Printer";
  }

  void emitInstruction(const MachineInstr *MI) override;
};

} // namespace llvm

#endif
