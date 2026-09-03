//===-- RiscvToyCallingConv.td - RiscvToy calling convention --*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// RISC-V integer calling convention:
//   integer args      : X10-X17 (a0-a7)
//   integer return    : X10      (a0)
//   stack alignment   : 16 bytes
def CC_RiscvToy_ILP32 : CallingConv<[
  CCIfType<[i32], CCAssignToReg<[X10, X11, X12, X13, X14, X15, X16, X17]>>,
  CCIfType<[i32], CCAssignToStack<4, 4>>
]>;

def RetCC_RiscvToy_ILP32 : CallingConv<[
  CCIfType<[i32], CCAssignToReg<[X10]>>
]>;

// An empty callee-saved set is useful for inline asm and call lowering that
// must mark every register as clobbered.
def CSR_NoRegs : CalleeSavedRegs<(add)>;
