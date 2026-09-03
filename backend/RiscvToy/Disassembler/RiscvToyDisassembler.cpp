//===-- RiscvToyDisassembler.cpp - RiscvToy disassembler -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This is a small hand-written decoder for the RV32I subset currently used by
// RiscvToy. It keeps the teaching backend readable: one decode function maps an
// opcode family to an MCInst without relying on generated decoder tables.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/RiscvToyMCTargetDesc.h"
#include "TargetInfo/RiscvToyTargetInfo.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "riscvtoy-disassembler"

typedef MCDisassembler::DecodeStatus DecodeStatus;

namespace {

class RiscvToyDisassembler : public MCDisassembler {
public:
  RiscvToyDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx)
      : MCDisassembler(STI, Ctx) {}

  DecodeStatus getInstruction(MCInst &Instr, uint64_t &Size,
                              ArrayRef<uint8_t> Bytes, uint64_t Address,
                              raw_ostream &CStream) const override;
};

static DecodeStatus addRegister(MCInst &Inst, unsigned RegNo) {
  if (RegNo >= 32)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createReg(RiscvToy::X0 + RegNo));
  return MCDisassembler::Success;
}

static int64_t signExtend(uint64_t Value, unsigned Bits) {
  assert(Bits < 64 && "Only small signed fields need manual extension");
  if (Value & (1ULL << (Bits - 1)))
    return static_cast<int64_t>(Value | (~0ULL << Bits));
  return static_cast<int64_t>(Value);
}

static DecodeStatus decodeRType(MCInst &Inst, uint32_t Insn) {
  unsigned Rd = (Insn >> 7) & 0x1f;
  unsigned Rs1 = (Insn >> 15) & 0x1f;
  unsigned Rs2 = (Insn >> 20) & 0x1f;
  unsigned Funct3 = (Insn >> 12) & 0x7;

  switch (Funct3) {
  default:
    return MCDisassembler::Fail;
  case 0:
    Inst.setOpcode((Insn & (1U << 30))
                       ? RiscvToy::RiscvToySUB
                       : RiscvToy::RiscvToyADD);
    break;
  case 2:
    Inst.setOpcode(RiscvToy::RiscvToySLT);
    break;
  case 3:
    Inst.setOpcode(RiscvToy::RiscvToySLTU);
    break;
  case 4:
    Inst.setOpcode(RiscvToy::RiscvToyXOR);
    break;
  case 1:
    Inst.setOpcode(RiscvToy::RiscvToySLL);
    break;
  case 5:
    Inst.setOpcode((Insn & (1U << 30)) ? RiscvToy::RiscvToySRA
                                       : RiscvToy::RiscvToySRL);
    break;
  case 6:
    Inst.setOpcode(RiscvToy::RiscvToyOR);
    break;
  case 7:
    Inst.setOpcode(RiscvToy::RiscvToyAND);
    break;
  }

  if (addRegister(Inst, Rd) == MCDisassembler::Fail ||
      addRegister(Inst, Rs1) == MCDisassembler::Fail ||
      addRegister(Inst, Rs2) == MCDisassembler::Fail)
    return MCDisassembler::Fail;
  return MCDisassembler::Success;
}

static DecodeStatus decodeIType(MCInst &Inst, uint32_t Insn) {
  unsigned Rd = (Insn >> 7) & 0x1f;
  unsigned Rs1 = (Insn >> 15) & 0x1f;
  unsigned Funct3 = (Insn >> 12) & 0x7;
  int64_t Imm = signExtend(Insn >> 20, 12);

  switch (Funct3) {
  default:
    return MCDisassembler::Fail;
  case 0:
    Inst.setOpcode(RiscvToy::RiscvToyADDI);
    break;
  case 2:
    Inst.setOpcode(RiscvToy::RiscvToySLTI);
    break;
  case 3:
    Inst.setOpcode(RiscvToy::RiscvToySLTIU);
    break;
  case 4:
    Inst.setOpcode(RiscvToy::RiscvToyXORI);
    break;
  }

  if (addRegister(Inst, Rd) == MCDisassembler::Fail ||
      addRegister(Inst, Rs1) == MCDisassembler::Fail)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createImm(Imm));
  return MCDisassembler::Success;
}

static DecodeStatus decodeShiftImmediate(MCInst &Inst, uint32_t Insn) {
  unsigned Rd = (Insn >> 7) & 0x1f;
  unsigned Rs1 = (Insn >> 15) & 0x1f;
  unsigned Shamt = (Insn >> 20) & 0x1f;
  unsigned Funct3 = (Insn >> 12) & 0x7;
  bool IsArithmetic = Insn & (1U << 30);

  switch (Funct3) {
  default:
    return MCDisassembler::Fail;
  case 1:
    if (IsArithmetic)
      return MCDisassembler::Fail;
    Inst.setOpcode(RiscvToy::RiscvToySLLI);
    break;
  case 5:
    Inst.setOpcode(IsArithmetic ? RiscvToy::RiscvToySRAI
                                : RiscvToy::RiscvToySRLI);
    break;
  }

  if (addRegister(Inst, Rd) == MCDisassembler::Fail ||
      addRegister(Inst, Rs1) == MCDisassembler::Fail)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createImm(Shamt));
  return MCDisassembler::Success;
}

static DecodeStatus decodeLoad(MCInst &Inst, uint32_t Insn) {
  unsigned Rd = (Insn >> 7) & 0x1f;
  unsigned Rs1 = (Insn >> 15) & 0x1f;
  int64_t Imm = signExtend(Insn >> 20, 12);

  Inst.setOpcode(RiscvToy::RiscvToyLW);
  if (addRegister(Inst, Rd) == MCDisassembler::Fail ||
      addRegister(Inst, Rs1) == MCDisassembler::Fail)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createImm(Imm));
  return MCDisassembler::Success;
}

static DecodeStatus decodeLUI(MCInst &Inst, uint32_t Insn) {
  unsigned Rd = (Insn >> 7) & 0x1f;
  int64_t Imm = Insn >> 12;

  Inst.setOpcode(RiscvToy::RiscvToyLUI);
  if (addRegister(Inst, Rd) == MCDisassembler::Fail)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createImm(Imm));
  return MCDisassembler::Success;
}

static DecodeStatus decodeStore(MCInst &Inst, uint32_t Insn) {
  unsigned Rs1 = (Insn >> 15) & 0x1f;
  unsigned Rs2 = (Insn >> 20) & 0x1f;
  int64_t Imm =
      signExtend(((Insn >> 25) & 0x7f) << 5 | ((Insn >> 7) & 0x1f), 12);

  Inst.setOpcode(RiscvToy::RiscvToySW);
  if (addRegister(Inst, Rs2) == MCDisassembler::Fail ||
      addRegister(Inst, Rs1) == MCDisassembler::Fail)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createImm(Imm));
  return MCDisassembler::Success;
}

static DecodeStatus decodeJALR(MCInst &Inst, uint32_t Insn) {
  unsigned Rd = (Insn >> 7) & 0x1f;
  unsigned Rs1 = (Insn >> 15) & 0x1f;
  int64_t Imm = signExtend(Insn >> 20, 12);

  Inst.setOpcode(RiscvToy::RiscvToyJALR);
  if (addRegister(Inst, Rd) == MCDisassembler::Fail ||
      addRegister(Inst, Rs1) == MCDisassembler::Fail)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createImm(Imm));
  return MCDisassembler::Success;
}

static DecodeStatus decodeBranch(MCInst &Inst, uint32_t Insn) {
  unsigned Rs1 = (Insn >> 15) & 0x1f;
  unsigned Rs2 = (Insn >> 20) & 0x1f;
  unsigned Funct3 = (Insn >> 12) & 0x7;
  int64_t Imm = signExtend(
      ((Insn >> 31) & 0x1) << 12 | ((Insn >> 7) & 0x1) << 11 |
          ((Insn >> 25) & 0x3f) << 5 | ((Insn >> 8) & 0xf) << 1,
      13);

  switch (Funct3) {
  default:
    return MCDisassembler::Fail;
  case 0:
    Inst.setOpcode(RiscvToy::RiscvToyBEQ);
    break;
  case 1:
    Inst.setOpcode(RiscvToy::RiscvToyBNE);
    break;
  case 4:
    Inst.setOpcode(RiscvToy::RiscvToyBLT);
    break;
  case 5:
    Inst.setOpcode(RiscvToy::RiscvToyBGE);
    break;
  case 6:
    Inst.setOpcode(RiscvToy::RiscvToyBLTU);
    break;
  case 7:
    Inst.setOpcode(RiscvToy::RiscvToyBGEU);
    break;
  }

  if (addRegister(Inst, Rs1) == MCDisassembler::Fail ||
      addRegister(Inst, Rs2) == MCDisassembler::Fail)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createImm(Imm));
  return MCDisassembler::Success;
}

static DecodeStatus decodeJAL(MCInst &Inst, uint32_t Insn) {
  unsigned Rd = (Insn >> 7) & 0x1f;
  int64_t Imm = signExtend(
      ((Insn >> 31) & 0x1) << 20 | ((Insn >> 21) & 0x3ff) << 1 |
          ((Insn >> 20) & 0x1) << 11 | ((Insn >> 12) & 0xff) << 12,
      21);

  Inst.setOpcode(RiscvToy::RiscvToyJAL);
  if (addRegister(Inst, Rd) == MCDisassembler::Fail)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::createImm(Imm));
  return MCDisassembler::Success;
}

DecodeStatus RiscvToyDisassembler::getInstruction(MCInst &Instr, uint64_t &Size,
                                                  ArrayRef<uint8_t> Bytes,
                                                  uint64_t Address,
                                                  raw_ostream &CStream) const {
  if (Bytes.size() < 4)
    return MCDisassembler::Fail;

  uint32_t Insn = support::endian::read32le(Bytes.data());
  unsigned Opcode = Insn & 0x7f;
  DecodeStatus Status = MCDisassembler::Fail;

  switch (Opcode) {
  case 0x37: // LUI
    Status = decodeLUI(Instr, Insn);
    break;
  case 0x33: // OP: integer register-register
    Status = decodeRType(Instr, Insn);
    break;
  case 0x13: // OP-IMM: integer register-immediate
    Status = (((Insn >> 12) & 0x7) == 0x1 ||
              ((Insn >> 12) & 0x7) == 0x5)
                 ? decodeShiftImmediate(Instr, Insn)
                 : decodeIType(Instr, Insn);
    break;
  case 0x03: // LOAD
    Status = ((Insn >> 12) & 0x7) == 0x2
                 ? decodeLoad(Instr, Insn)
                 : MCDisassembler::Fail;
    break;
  case 0x23: // STORE
    Status = ((Insn >> 12) & 0x7) == 0x2
                 ? decodeStore(Instr, Insn)
                 : MCDisassembler::Fail;
    break;
  case 0x67: // JALR
    Status = ((Insn >> 12) & 0x7) == 0x0
                 ? decodeJALR(Instr, Insn)
                 : MCDisassembler::Fail;
    break;
  case 0x63: // BRANCH
    Status = decodeBranch(Instr, Insn);
    break;
  case 0x6f: // JAL
    Status = decodeJAL(Instr, Insn);
    break;
  default:
    break;
  }

  if (Status == MCDisassembler::Fail)
    return MCDisassembler::Fail;

  Size = 4;
  return MCDisassembler::Success;
}

} // namespace

static MCDisassembler *createRiscvToyDisassembler(const Target &T,
                                                  const MCSubtargetInfo &STI,
                                                  MCContext &Ctx) {
  return new RiscvToyDisassembler(STI, Ctx);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRiscvToyDisassembler() {
  TargetRegistry::RegisterMCDisassembler(getTheRiscvToyTarget(),
                                         createRiscvToyDisassembler);
}
