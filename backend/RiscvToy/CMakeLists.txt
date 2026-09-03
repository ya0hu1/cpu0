add_llvm_component_group(RiscvToy)

set(LLVM_TARGET_DEFINITIONS RiscvToy.td)
tablegen(LLVM RiscvToyGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM RiscvToyGenCallingConv.inc -gen-callingconv)
tablegen(LLVM RiscvToyGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM RiscvToyGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM RiscvToyGenSubtargetInfo.inc -gen-subtarget)
tablegen(LLVM RiscvToyGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM RiscvToyGenMCCodeEmitter.inc -gen-emitter)

add_public_tablegen_target(RiscvToyCommonTableGen)

add_llvm_target(RiscvToyCodeGen
  RiscvToyAsmPrinter.cpp
  RiscvToyFrameLowering.cpp
  RiscvToyISelDAGToDAG.cpp
  RiscvToyISelLowering.cpp
  RiscvToyInstrInfo.cpp
  RiscvToyRegisterInfo.cpp
  RiscvToySubtarget.cpp
  RiscvToyTargetMachine.cpp

  LINK_COMPONENTS
  AsmPrinter
  CodeGen
  MC
  RiscvToyDesc
  RiscvToyInfo
  SelectionDAG
  Support
  Target

  DEPENDS
  RiscvToyCommonTableGen

  ADD_TO_COMPONENT
  RiscvToy
  )

add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
add_subdirectory(Disassembler)
