//===-- RiscvToySubtarget.td - RiscvToy subtarget definitions -*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RiscvToy currently models only the base RV32I instruction set.
//
//===----------------------------------------------------------------------===//

def FeatureRV32I : SubtargetFeature<"rv32i", "HasRV32I", "true",
                                    "RV32I Base Instruction Set">;

class RiscvToyProc<string Name, list<SubtargetFeature> Features>
    : Processor<Name, NoItineraries, Features>;

def : RiscvToyProc<"generic-rv32", [FeatureRV32I]>;
