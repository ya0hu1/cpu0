//===-- RiscvToyFrameLowering.cpp - RiscvToy frame lowering impl ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RiscvToyFrameLowering.h"

using namespace llvm;

void RiscvToyFrameLowering::emitPrologue(MachineFunction &MF,
                                         MachineBasicBlock &MBB) const {}

void RiscvToyFrameLowering::emitEpilogue(MachineFunction &MF,
                                         MachineBasicBlock &MBB) const {}
