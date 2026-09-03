//===-- RiscvToyISelDAGToDAG.cpp - RiscvToy DAG selector --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RiscvToy.h"
#include "RiscvToySubtarget.h"
#include "RiscvToyTargetMachine.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGISel.h"

using namespace llvm;

#define DEBUG_TYPE "riscvtoy-isel"

namespace {

class RiscvToyDAGToDAGISel : public SelectionDAGISel {
  const RiscvToySubtarget *Subtarget = nullptr;

public:
  explicit RiscvToyDAGToDAGISel(RiscvToyTargetMachine &TM,
                                CodeGenOpt::Level OptLevel)
      : SelectionDAGISel(TM, OptLevel) {}

  StringRef getPassName() const override {
    return "RiscvToy DAG->DAG Pattern Instruction Selection";
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    Subtarget = &MF.getSubtarget<RiscvToySubtarget>();
    return SelectionDAGISel::runOnMachineFunction(MF);
  }

private:
  bool SelectAddrFI(SDValue Addr, SDValue &Base);

#include "RiscvToyGenDAGISel.inc"

  void Select(SDNode *N) override { SelectCode(N); }
};

} // namespace

bool RiscvToyDAGToDAGISel::SelectAddrFI(SDValue Addr, SDValue &Base) {
  if (auto *FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
    Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i32);
    return true;
  }
  return false;
}

FunctionPass *llvm::createRiscvToyISelDag(RiscvToyTargetMachine &TM,
                                          CodeGenOpt::Level OptLevel) {
  return new RiscvToyDAGToDAGISel(TM, OptLevel);
}
