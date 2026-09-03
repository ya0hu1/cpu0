add_llvm_component_group(RiscvToy)

set(LLVM_TARGET_DEFINITIONS RiscvToy.td)
tablegen(LLVM RiscvToyGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM RiscvToyGenCallingConv.inc -gen-callingconv)

add_public_tablegen_target(RiscvToyCommonTableGen)

add_llvm_target(RiscvToyCodeGen
  RiscvToyTargetMachine.cpp

  LINK_COMPONENTS
  CodeGen
  Support
  Target

  DEPENDS
  RiscvToyCommonTableGen

  ADD_TO_COMPONENT
  RiscvToy
  )

add_subdirectory(TargetInfo)
