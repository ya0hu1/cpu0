add_llvm_component_library(LLVMRiscvToyInfo
  RiscvToyTargetInfo.cpp

  LINK_COMPONENTS
  Support

  ADD_TO_COMPONENT
  RiscvToy
  )
