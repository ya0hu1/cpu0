//===-- RiscvToyISelLowering.cpp - RiscvToy DAG lowering impl ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RiscvToyISelLowering.h"
#include "RiscvToySubtarget.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

RiscvToyTargetLowering::RiscvToyTargetLowering(const TargetMachine &TM,
                                               const RiscvToySubtarget &STI)
    : TargetLowering(TM), Subtarget(STI) {
  addRegisterClass(MVT::i32, &RiscvToy::GPRRegClass);
  computeRegisterProperties(Subtarget.getRegisterInfo());

  setStackPointerRegisterToSaveRestore(RiscvToy::X2);
  setBooleanContents(ZeroOrOneBooleanContent);

  setOperationAction(ISD::BR_CC, MVT::i32, Expand);
  setOperationAction(ISD::SELECT, MVT::i32, Expand);
  setOperationAction(ISD::SELECT_CC, MVT::i32, Expand);
  setOperationAction(ISD::BR_JT, MVT::Other, Expand);
}

const char *RiscvToyTargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case RiscvToyISD::CALL:
    return "RiscvToyISD::CALL";
  case RiscvToyISD::RET_FLAG:
    return "RiscvToyISD::RET_FLAG";
  default:
    return nullptr;
  }
}

SDValue RiscvToyTargetLowering::LowerOperation(SDValue Op,
                                               SelectionDAG &DAG) const {
  llvm_unreachable("RiscvToy custom lowering is not implemented yet");
}

#include "RiscvToyGenCallingConv.inc"

SDValue RiscvToyTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  if (CallConv != CallingConv::C && CallConv != CallingConv::Fast)
    report_fatal_error("Unsupported calling convention");

  MachineFunction &MF = DAG.getMachineFunction();
  MachineRegisterInfo &MRI = MF.getRegInfo();

  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins, CC_RiscvToy_ILP32);

  for (CCValAssign &VA : ArgLocs) {
    if (!VA.isRegLoc() || VA.getLocVT() != MVT::i32)
      report_fatal_error("RiscvToy stack arguments are not supported yet");

    Register VReg =
        MRI.createVirtualRegister(&RiscvToy::GPRRegClass);
    MRI.addLiveIn(VA.getLocReg(), VReg);
    SDValue ArgValue = DAG.getCopyFromReg(Chain, DL, VReg, VA.getLocVT());
    InVals.push_back(ArgValue);
  }

  return Chain;
}

SDValue RiscvToyTargetLowering::LowerCall(CallLoweringInfo &CLI,
                                          SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG = CLI.DAG;
  SmallVectorImpl<ISD::OutputArg> &Outs = CLI.Outs;
  SmallVectorImpl<SDValue> &OutVals = CLI.OutVals;
  SmallVectorImpl<ISD::InputArg> &Ins = CLI.Ins;
  SDValue Chain = CLI.Chain;
  SDValue Callee = CLI.Callee;
  bool &IsTailCall = CLI.IsTailCall;
  CallingConv::ID CallConv = CLI.CallConv;
  bool IsVarArg = CLI.IsVarArg;
  MachineFunction &MF = DAG.getMachineFunction();

  if (CallConv != CallingConv::C && CallConv != CallingConv::Fast)
    report_fatal_error("Unsupported calling convention");

  // RiscvToy does not implement tail calls yet. Lowering it as a regular call
  // keeps the stack frame and ra handling simple.
  IsTailCall = false;

  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());
  CCInfo.AnalyzeCallOperands(Outs, CC_RiscvToy_ILP32);

  if (CCInfo.getNextStackOffset() != 0 || IsVarArg)
    report_fatal_error("RiscvToy stack arguments and varargs are not supported");

  EVT PtrVT = getPointerTy(MF.getDataLayout());
  Chain = DAG.getCALLSEQ_START(Chain, 0, 0, CLI.DL);

  SDValue Glue;
  SmallVector<std::pair<unsigned, SDValue>, 8> RegsToPass;

  for (unsigned I = 0, E = ArgLocs.size(); I != E; ++I) {
    CCValAssign &VA = ArgLocs[I];
    SDValue Arg = OutVals[I];

    if (!VA.isRegLoc() || VA.getLocVT() != MVT::i32)
      report_fatal_error("RiscvToy only lowers i32 register arguments");

    if (VA.getLocInfo() == CCValAssign::SExt)
      Arg = DAG.getNode(ISD::SIGN_EXTEND, CLI.DL, VA.getLocVT(), Arg);
    else if (VA.getLocInfo() == CCValAssign::ZExt)
      Arg = DAG.getNode(ISD::ZERO_EXTEND, CLI.DL, VA.getLocVT(), Arg);
    else if (VA.getLocInfo() == CCValAssign::AExt)
      Arg = DAG.getNode(ISD::ANY_EXTEND, CLI.DL, VA.getLocVT(), Arg);

    RegsToPass.push_back(std::make_pair(VA.getLocReg(), Arg));
  }

  for (auto &Reg : RegsToPass) {
    Chain = DAG.getCopyToReg(Chain, CLI.DL, Reg.first, Reg.second, Glue);
    Glue = Chain.getValue(1);
  }

  if (GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee))
    Callee = DAG.getTargetGlobalAddress(G->getGlobal(), CLI.DL, PtrVT,
                                        G->getOffset());
  else if (ExternalSymbolSDNode *E = dyn_cast<ExternalSymbolSDNode>(Callee))
    Callee = DAG.getTargetExternalSymbol(E->getSymbol(), PtrVT);
  else
    report_fatal_error("RiscvToy indirect calls are not supported yet");

  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Chain);
  Ops.push_back(Callee);

  for (auto &Reg : RegsToPass)
    Ops.push_back(DAG.getRegister(Reg.first, Reg.second.getValueType()));

  if (Glue.getNode())
    Ops.push_back(Glue);

  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
  Chain = DAG.getNode(RiscvToyISD::CALL, CLI.DL, NodeTys, Ops);
  Glue = Chain.getValue(1);

  Chain = DAG.getCALLSEQ_END(
      Chain, DAG.getConstant(0, CLI.DL, PtrVT, true),
      DAG.getConstant(0, CLI.DL, PtrVT, true), Glue, CLI.DL);
  Glue = Chain.getValue(1);

  return LowerCallResult(Chain, Glue, CallConv, IsVarArg, Ins, CLI.DL, DAG,
                         InVals);
}

SDValue RiscvToyTargetLowering::LowerCallResult(
    SDValue Chain, SDValue InFlag, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  MachineFunction &MF = DAG.getMachineFunction();

  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, RVLocs, *DAG.getContext());
  CCInfo.AnalyzeCallResult(Ins, RetCC_RiscvToy_ILP32);

  for (CCValAssign &VA : RVLocs) {
    if (!VA.isRegLoc() || VA.getLocVT() != MVT::i32)
      report_fatal_error("RiscvToy only supports i32 register returns");

    SDValue Val = DAG.getCopyFromReg(Chain, DL, VA.getLocReg(),
                                     VA.getValVT(), InFlag);
    Chain = Val.getValue(1);
    InFlag = Val.getValue(2);
    InVals.push_back(Val);
  }

  return Chain;
}

bool RiscvToyTargetLowering::CanLowerReturn(
    CallingConv::ID CallConv, MachineFunction &MF, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    LLVMContext &Context) const {
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, RVLocs, Context);
  if (!CCInfo.CheckReturn(Outs, RetCC_RiscvToy_ILP32))
    return false;

  for (const CCValAssign &VA : RVLocs)
    if (!VA.isRegLoc())
      return false;

  return true;
}

SDValue RiscvToyTargetLowering::LowerReturn(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
    SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, RVLocs, *DAG.getContext());
  CCInfo.AnalyzeReturn(Outs, RetCC_RiscvToy_ILP32);

  SDValue Glue;
  SmallVector<SDValue, 4> RetOps(1, Chain);

  for (unsigned I = 0, E = RVLocs.size(); I != E; ++I) {
    CCValAssign &VA = RVLocs[I];
    assert(VA.isRegLoc() && "Can only return in registers");

    Chain = DAG.getCopyToReg(Chain, DL, VA.getLocReg(), OutVals[I], Glue);
    Glue = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  RetOps[0] = Chain;
  if (Glue.getNode())
    RetOps.push_back(Glue);

  return DAG.getNode(RiscvToyISD::RET_FLAG, DL, MVT::Other, RetOps);
}
