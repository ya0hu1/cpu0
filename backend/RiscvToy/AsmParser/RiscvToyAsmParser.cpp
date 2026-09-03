//===-- RiscvToyAsmParser.cpp - Parse RiscvToy assembly ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This is a small hand-written parser for the RiscvToy assembly subset. It is
// intentionally kept smaller than a generated AsmMatcher-based parser so the
// teaching backend can show the MC parser flow directly.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/RiscvToyMCTargetDesc.h"
#include "TargetInfo/RiscvToyTargetInfo.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

namespace {

class RiscvToyAsmParser;

class RiscvToyOperand : public MCParsedAsmOperand {
  enum KindTy {
    Token,
    Register,
    Immediate,
    Memory,
  };

  KindTy Kind;
  SMLoc StartLoc, EndLoc;

  struct TokenInfo {
    StringRef Data;
  };
  struct RegisterInfo {
    unsigned RegNo;
  };
  struct ImmediateInfo {
    const MCExpr *Value;
  };
  struct MemoryInfo {
    unsigned BaseReg;
    const MCExpr *Offset;
  };

  union {
    TokenInfo Tok;
    RegisterInfo Reg;
    ImmediateInfo Imm;
    MemoryInfo Mem;
  };

public:
  RiscvToyOperand(KindTy Kind, SMLoc StartLoc, SMLoc EndLoc)
      : Kind(Kind), StartLoc(StartLoc), EndLoc(EndLoc) {}

  static std::unique_ptr<RiscvToyOperand> CreateToken(StringRef Str,
                                                      SMLoc Loc) {
    auto Op = std::make_unique<RiscvToyOperand>(Token, Loc, Loc);
    Op->Tok.Data = Str;
    return Op;
  }

  static std::unique_ptr<RiscvToyOperand> CreateReg(unsigned RegNo, SMLoc Start,
                                                    SMLoc End) {
    auto Op = std::make_unique<RiscvToyOperand>(Register, Start, End);
    Op->Reg.RegNo = RegNo;
    return Op;
  }

  static std::unique_ptr<RiscvToyOperand> CreateImm(const MCExpr *Value,
                                                    SMLoc Start, SMLoc End) {
    auto Op = std::make_unique<RiscvToyOperand>(Immediate, Start, End);
    Op->Imm.Value = Value;
    return Op;
  }

  static std::unique_ptr<RiscvToyOperand> CreateMem(unsigned BaseReg,
                                                    const MCExpr *Offset,
                                                    SMLoc Start, SMLoc End) {
    auto Op = std::make_unique<RiscvToyOperand>(Memory, Start, End);
    Op->Mem.BaseReg = BaseReg;
    Op->Mem.Offset = Offset;
    return Op;
  }

  bool isToken() const override { return Kind == Token; }
  bool isReg() const override { return Kind == Register; }
  bool isImm() const override { return Kind == Immediate; }
  bool isMem() const override { return Kind == Memory; }

  unsigned getReg() const override {
    assert(Kind == Register && "Not a register operand");
    return Reg.RegNo;
  }

  StringRef getToken() const {
    assert(Kind == Token && "Not a token operand");
    return Tok.Data;
  }

  const MCExpr *getImm() const {
    assert(Kind == Immediate && "Not an immediate operand");
    return Imm.Value;
  }

  unsigned getMemBase() const {
    assert(Kind == Memory && "Not a memory operand");
    return Mem.BaseReg;
  }

  const MCExpr *getMemOffset() const {
    assert(Kind == Memory && "Not a memory operand");
    return Mem.Offset;
  }

  SMLoc getStartLoc() const override { return StartLoc; }
  SMLoc getEndLoc() const override { return EndLoc; }

  void print(raw_ostream &OS) const override {
    switch (Kind) {
    case Token:
      OS << Tok.Data;
      break;
    case Register:
      OS << "Register<" << Reg.RegNo << ">";
      break;
    case Immediate:
      OS << "Immediate<";
      OS << *Imm.Value;
      OS << ">";
      break;
    case Memory:
      OS << "Memory<" << Mem.BaseReg << ", " << *Mem.Offset << ">";
      break;
    }
  }
};

class RiscvToyAsmParser : public MCTargetAsmParser {
  MCAsmParser &Parser;

  bool parseMemoryOperand(OperandVector &Operands);
  bool parseOperand(OperandVector &Operands, StringRef Mnemonic);
  bool matchRegisterName(StringRef Name, unsigned &RegNo);

  static RiscvToyOperand &getOperand(OperandVector &Operands, unsigned Index);
  static MCOperand getMCOperand(const MCExpr *Expr);
  bool requireImmediateRange(const MCExpr *Expr, SMLoc Loc, int64_t Lower,
                             int64_t Upper);
  bool requireBranchRange(const MCExpr *Expr, SMLoc Loc, bool IsJAL);

public:
  RiscvToyAsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
                    const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, STI, MII), Parser(Parser) {}

  bool ParseRegister(unsigned &RegNo, SMLoc &StartLoc,
                     SMLoc &EndLoc) override;
  OperandMatchResultTy tryParseRegister(unsigned &RegNo, SMLoc &StartLoc,
                                        SMLoc &EndLoc) override;
  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;
  bool ParseDirective(AsmToken DirectiveID) override;
  bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;
  void convertToMapAndConstraints(unsigned Kind,
                                  const OperandVector &Operands) override {}

  MCAsmParser &getParser() const { return Parser; }
  MCAsmLexer &getLexer() const { return Parser.getLexer(); }
};

} // namespace

RiscvToyOperand &RiscvToyAsmParser::getOperand(OperandVector &Operands,
                                               unsigned Index) {
  return static_cast<RiscvToyOperand &>(*Operands[Index]);
}

MCOperand RiscvToyAsmParser::getMCOperand(const MCExpr *Expr) {
  if (const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Expr))
    return MCOperand::createImm(CE->getValue());
  return MCOperand::createExpr(Expr);
}

bool RiscvToyAsmParser::matchRegisterName(StringRef Name, unsigned &RegNo) {
  StringRef Lower = Name.lower();
  int Value = StringSwitch<int>(Lower)
                  .Case("x0", 0).Case("zero", 0)
                  .Case("x1", 1).Case("ra", 1)
                  .Case("x2", 2).Case("sp", 2)
                  .Case("x3", 3).Case("gp", 3)
                  .Case("x4", 4).Case("tp", 4)
                  .Case("x5", 5).Case("t0", 5)
                  .Case("x6", 6).Case("t1", 6)
                  .Case("x7", 7).Case("t2", 7)
                  .Case("x8", 8).Case("s0", 8).Case("fp", 8)
                  .Case("x9", 9).Case("s1", 9)
                  .Case("x10", 10).Case("a0", 10)
                  .Case("x11", 11).Case("a1", 11)
                  .Case("x12", 12).Case("a2", 12)
                  .Case("x13", 13).Case("a3", 13)
                  .Case("x14", 14).Case("a4", 14)
                  .Case("x15", 15).Case("a5", 15)
                  .Case("x16", 16).Case("a6", 16)
                  .Case("x17", 17).Case("a7", 17)
                  .Case("x18", 18).Case("s2", 18)
                  .Case("x19", 19).Case("s3", 19)
                  .Case("x20", 20).Case("s4", 20)
                  .Case("x21", 21).Case("s5", 21)
                  .Case("x22", 22).Case("s6", 22)
                  .Case("x23", 23).Case("s7", 23)
                  .Case("x24", 24).Case("s8", 24)
                  .Case("x25", 25).Case("s9", 25)
                  .Case("x26", 26).Case("s10", 26)
                  .Case("x27", 27).Case("s11", 27)
                  .Case("x28", 28).Case("t3", 28)
                  .Case("x29", 29).Case("t4", 29)
                  .Case("x30", 30).Case("t5", 30)
                  .Case("x31", 31).Case("t6", 31)
                  .Default(-1);
  if (Value < 0)
    return false;
  RegNo = static_cast<unsigned>(Value);
  return true;
}

bool RiscvToyAsmParser::ParseRegister(unsigned &RegNo, SMLoc &StartLoc,
                                      SMLoc &EndLoc) {
  StartLoc = getParser().getTok().getLoc();
  if (getLexer().is(AsmToken::Identifier)) {
    unsigned Value;
    if (matchRegisterName(getLexer().getTok().getString(), Value)) {
      RegNo = RiscvToy::X0 + Value;
      EndLoc = getLexer().getTok().getEndLoc();
      return false;
    }
  }
  return true;
}

OperandMatchResultTy RiscvToyAsmParser::tryParseRegister(unsigned &RegNo,
                                                         SMLoc &StartLoc,
                                                         SMLoc &EndLoc) {
  StartLoc = getParser().getTok().getLoc();
  if (!getLexer().is(AsmToken::Identifier))
    return MatchOperand_NoMatch;

  StringRef Name = getLexer().getTok().getString();
  unsigned Value;
  if (!matchRegisterName(Name, Value))
    return MatchOperand_NoMatch;

  RegNo = RiscvToy::X0 + Value;
  EndLoc = getLexer().getTok().getEndLoc();
  getParser().Lex();
  return MatchOperand_Success;
}

bool RiscvToyAsmParser::parseMemoryOperand(OperandVector &Operands) {
  SMLoc Start = getParser().getTok().getLoc();
  const MCExpr *Offset = nullptr;

  if (getLexer().isNot(AsmToken::LParen)) {
    if (getParser().parseExpression(Offset))
      return Error(Start, "expected memory offset");
  }

  if (!getLexer().is(AsmToken::LParen))
    return Error(Start, "expected '(' in memory operand");
  getParser().Lex();

  SMLoc RegStart = getParser().getTok().getLoc();
  if (getLexer().is(AsmToken::Dollar))
    getParser().Lex();
  if (!getLexer().is(AsmToken::Identifier))
    return Error(RegStart, "expected base register");

  unsigned Encoding;
  if (!matchRegisterName(getLexer().getTok().getString(), Encoding))
    return Error(RegStart, "unknown base register");
  getParser().Lex();

  if (!getLexer().is(AsmToken::RParen))
    return Error(getParser().getTok().getLoc(), "expected ')' in memory operand");
  SMLoc End = getParser().getTok().getEndLoc();
  getParser().Lex();

  if (!Offset)
    Offset = MCConstantExpr::create(0, getContext());
  Operands.push_back(RiscvToyOperand::CreateMem(RiscvToy::X0 + Encoding, Offset,
                                                Start, End));
  return false;
}

bool RiscvToyAsmParser::parseOperand(OperandVector &Operands,
                                     StringRef Mnemonic) {
  if ((Mnemonic == "lw" || Mnemonic == "sw" || Mnemonic == "jalr") &&
      Operands.size() > 1)
    return parseMemoryOperand(Operands);

  if (getLexer().is(AsmToken::Identifier) ||
      getLexer().is(AsmToken::Dollar)) {
    unsigned Encoding;
    SMLoc Start = getParser().getTok().getLoc();
    StringRef Name = getLexer().getTok().getString();
    if (getLexer().is(AsmToken::Dollar)) {
      getParser().Lex();
      if (!getLexer().is(AsmToken::Identifier))
        return Error(Start, "expected register");
      Name = getLexer().getTok().getString();
    }
    if (matchRegisterName(Name, Encoding)) {
      if (getLexer().is(AsmToken::Dollar))
        getParser().Lex();
      SMLoc End = getLexer().getTok().getEndLoc();
      getParser().Lex();
      Operands.push_back(RiscvToyOperand::CreateReg(RiscvToy::X0 + Encoding,
                                                    Start, End));
      return false;
    }
  }

  const MCExpr *Value;
  SMLoc Start = getParser().getTok().getLoc();
  if (getParser().parseExpression(Value))
    return Error(Start, "expected expression");
  SMLoc End = SMLoc::getFromPointer(getParser().getTok().getLoc().getPointer());
  Operands.push_back(RiscvToyOperand::CreateImm(Value, Start, End));
  return false;
}

bool RiscvToyAsmParser::ParseInstruction(ParseInstructionInfo &Info,
                                         StringRef Name, SMLoc NameLoc,
                                         OperandVector &Operands) {
  Operands.push_back(RiscvToyOperand::CreateToken(Name, NameLoc));

  if (getLexer().is(AsmToken::EndOfStatement)) {
    getParser().Lex();
    return false;
  }

  if (parseOperand(Operands, Name))
    return true;

  while (getLexer().is(AsmToken::Comma)) {
    getParser().Lex();
    if (parseOperand(Operands, Name))
      return true;
  }

  if (!getLexer().is(AsmToken::EndOfStatement))
    return Error(getParser().getTok().getLoc(), "unexpected token in operand");

  getParser().Lex();
  return false;
}

bool RiscvToyAsmParser::ParseDirective(AsmToken DirectiveID) {
  return true;
}

bool RiscvToyAsmParser::requireImmediateRange(const MCExpr *Expr, SMLoc Loc,
                                              int64_t Lower, int64_t Upper) {
  const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Expr);
  if (!CE)
    return Error(Loc, "symbolic immediate is not supported");
  int64_t Value = CE->getValue();
  if (Value < Lower || Value > Upper)
    return Error(Loc, "immediate out of range");
  return false;
}

bool RiscvToyAsmParser::requireBranchRange(const MCExpr *Expr, SMLoc Loc,
                                           bool IsJAL) {
  const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Expr);
  if (!CE)
    return false;
  int64_t Value = CE->getValue();
  if (Value & 0x1)
    return Error(Loc, "branch target must be 2-byte aligned");
  if (IsJAL && (Value < -1048576 || Value > 1048574))
    return Error(Loc, "jump target out of range");
  if (!IsJAL && (Value < -4096 || Value > 4094))
    return Error(Loc, "branch target out of range");
  return false;
}

bool RiscvToyAsmParser::MatchAndEmitInstruction(
    SMLoc IDLoc, unsigned &Opcode, OperandVector &Operands, MCStreamer &Out,
    uint64_t &ErrorInfo, bool MatchingInlineAsm) {
  if (Operands.empty())
    return Error(IDLoc, "missing mnemonic");

  RiscvToyOperand &MnemonicOp = getOperand(Operands, 0);
  if (!MnemonicOp.isToken())
    return Error(IDLoc, "expected mnemonic");
  StringRef Mnemonic = MnemonicOp.getToken();

  auto fail = [&](const char *Message) {
    return Error(IDLoc, Message);
  };

  auto emit = [&](MCInst &Inst) {
    Inst.setLoc(IDLoc);
    Out.emitInstruction(Inst, getSTI());
    return false;
  };

  auto reg = [&](unsigned Index, unsigned &Value) {
    if (Index >= Operands.size() || !getOperand(Operands, Index).isReg())
      return true;
    Value = getOperand(Operands, Index).getReg();
    return false;
  };

  auto imm = [&](unsigned Index, const MCExpr *&Value) {
    if (Index >= Operands.size() || !getOperand(Operands, Index).isImm())
      return true;
    Value = getOperand(Operands, Index).getImm();
    return false;
  };

  auto mem = [&](unsigned Index, unsigned &BaseReg, const MCExpr *&Offset) {
    if (Index >= Operands.size() || !getOperand(Operands, Index).isMem())
      return true;
    BaseReg = getOperand(Operands, Index).getMemBase();
    Offset = getOperand(Operands, Index).getMemOffset();
    return false;
  };

  MCInst Inst;
  unsigned Rd, Rs1, Rs2;
  const MCExpr *ImmExpr;

  if (Mnemonic == "ret") {
    Inst.setOpcode(RiscvToy::PseudoRET);
    return emit(Inst);
  }

  if (Mnemonic == "j" || Mnemonic == "jal") {
    if (Mnemonic == "j") {
      if (imm(1, ImmExpr))
        return fail("branch target expected");
      if (requireBranchRange(ImmExpr, IDLoc, /*IsJAL=*/true))
        return true;
      Inst.setOpcode(RiscvToy::PseudoBR);
      Inst.addOperand(getMCOperand(ImmExpr));
    } else {
      if (reg(1, Rd) || imm(2, ImmExpr))
        return fail("jal needs destination register and target");
      if (requireBranchRange(ImmExpr, IDLoc, /*IsJAL=*/true))
        return true;
      Inst.setOpcode(RiscvToy::RiscvToyJAL);
      Inst.addOperand(MCOperand::createReg(Rd));
      Inst.addOperand(getMCOperand(ImmExpr));
    }
    return emit(Inst);
  }

  if (Mnemonic == "call") {
    if (imm(1, ImmExpr))
      return fail("call target expected");
    Inst.setOpcode(RiscvToy::PseudoCALL);
    Inst.addOperand(getMCOperand(ImmExpr));
    return emit(Inst);
  }

  if (Mnemonic == "lui") {
    if (reg(1, Rd) || imm(2, ImmExpr))
      return fail("register and 20-bit immediate expected");
    if (requireImmediateRange(ImmExpr, IDLoc, 0, 0xfffff))
      return true;
    Inst.setOpcode(RiscvToy::RiscvToyLUI);
    Inst.addOperand(MCOperand::createReg(Rd));
    Inst.addOperand(getMCOperand(ImmExpr));
    return emit(Inst);
  }

  if (Mnemonic == "add" || Mnemonic == "sub" || Mnemonic == "and" ||
      Mnemonic == "or" || Mnemonic == "xor" || Mnemonic == "slt" ||
      Mnemonic == "sltu") {
    if (reg(1, Rd) || reg(2, Rs1) || reg(3, Rs2))
      return fail("three register operands expected");
    if (Mnemonic == "add")
      Inst.setOpcode(RiscvToy::RiscvToyADD);
    else if (Mnemonic == "sub")
      Inst.setOpcode(RiscvToy::RiscvToySUB);
    else if (Mnemonic == "and")
      Inst.setOpcode(RiscvToy::RiscvToyAND);
    else if (Mnemonic == "or")
      Inst.setOpcode(RiscvToy::RiscvToyOR);
    else if (Mnemonic == "xor")
      Inst.setOpcode(RiscvToy::RiscvToyXOR);
    else if (Mnemonic == "slt")
      Inst.setOpcode(RiscvToy::RiscvToySLT);
    else
      Inst.setOpcode(RiscvToy::RiscvToySLTU);
    Inst.addOperand(MCOperand::createReg(Rd));
    Inst.addOperand(MCOperand::createReg(Rs1));
    Inst.addOperand(MCOperand::createReg(Rs2));
    return emit(Inst);
  }

  if (Mnemonic == "addi" || Mnemonic == "slti" || Mnemonic == "sltiu" ||
      Mnemonic == "xori") {
    if (reg(1, Rd) || reg(2, Rs1) || imm(3, ImmExpr))
      return fail("register, register, immediate expected");
    if (requireImmediateRange(ImmExpr, IDLoc, -2048, 2047))
      return true;
    if (Mnemonic == "addi")
      Inst.setOpcode(RiscvToy::RiscvToyADDI);
    else if (Mnemonic == "slti")
      Inst.setOpcode(RiscvToy::RiscvToySLTI);
    else if (Mnemonic == "sltiu")
      Inst.setOpcode(RiscvToy::RiscvToySLTIU);
    else
      Inst.setOpcode(RiscvToy::RiscvToyXORI);
    Inst.addOperand(MCOperand::createReg(Rd));
    Inst.addOperand(MCOperand::createReg(Rs1));
    Inst.addOperand(getMCOperand(ImmExpr));
    return emit(Inst);
  }

  if (Mnemonic == "lw" || Mnemonic == "sw") {
    const MCExpr *Offset;
    unsigned BaseReg;
    if (reg(1, Rd))
      return fail("register operand expected");
    if (mem(2, BaseReg, Offset))
      return fail("memory operand expected");
    if (requireImmediateRange(Offset, IDLoc, -2048, 2047))
      return true;
    if (Mnemonic == "lw") {
      Inst.setOpcode(RiscvToy::RiscvToyLW);
      Inst.addOperand(MCOperand::createReg(Rd));
      Inst.addOperand(MCOperand::createReg(BaseReg));
      Inst.addOperand(getMCOperand(Offset));
    } else {
      Inst.setOpcode(RiscvToy::RiscvToySW);
      Inst.addOperand(MCOperand::createReg(Rd));
      Inst.addOperand(MCOperand::createReg(BaseReg));
      Inst.addOperand(getMCOperand(Offset));
    }
    return emit(Inst);
  }

  if (Mnemonic == "beq" || Mnemonic == "bne" || Mnemonic == "blt" ||
      Mnemonic == "bge" || Mnemonic == "bltu" || Mnemonic == "bgeu") {
    if (reg(1, Rs1) || reg(2, Rs2) || imm(3, ImmExpr))
      return fail("branch operands expected");
    if (requireBranchRange(ImmExpr, IDLoc, /*IsJAL=*/false))
      return true;
    if (Mnemonic == "beq")
      Inst.setOpcode(RiscvToy::RiscvToyBEQ);
    else if (Mnemonic == "bne")
      Inst.setOpcode(RiscvToy::RiscvToyBNE);
    else if (Mnemonic == "blt")
      Inst.setOpcode(RiscvToy::RiscvToyBLT);
    else if (Mnemonic == "bge")
      Inst.setOpcode(RiscvToy::RiscvToyBGE);
    else if (Mnemonic == "bltu")
      Inst.setOpcode(RiscvToy::RiscvToyBLTU);
    else
      Inst.setOpcode(RiscvToy::RiscvToyBGEU);
    Inst.addOperand(MCOperand::createReg(Rs1));
    Inst.addOperand(MCOperand::createReg(Rs2));
    Inst.addOperand(getMCOperand(ImmExpr));
    return emit(Inst);
  }

  if (Mnemonic == "jalr") {
    const MCExpr *Offset;
    unsigned BaseReg;
    if (reg(1, Rd))
      return fail("jalr destination register expected");
    if (mem(2, BaseReg, Offset))
      return fail("jalr memory operand expected");
    if (requireImmediateRange(Offset, IDLoc, -2048, 2047))
      return true;
    Inst.setOpcode(RiscvToy::RiscvToyJALR);
    Inst.addOperand(MCOperand::createReg(Rd));
    Inst.addOperand(MCOperand::createReg(BaseReg));
    Inst.addOperand(getMCOperand(Offset));
    return emit(Inst);
  }

  return Error(IDLoc, "unknown instruction mnemonic");
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeRiscvToyAsmParser() {
  RegisterMCAsmParser<RiscvToyAsmParser> X(getTheRiscvToyTarget());
}
