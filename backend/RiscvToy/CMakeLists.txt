add_llvm_component_group(RiscvToy)

add_llvm_target(RiscvToyCodeGen
  RiscvToyTargetMachine.cpp

  LINK_COMPONENTS
  CodeGen
  Support
  Target

  ADD_TO_COMPONENT
  RiscvToy
  )

add_subdirectory(TargetInfo)
