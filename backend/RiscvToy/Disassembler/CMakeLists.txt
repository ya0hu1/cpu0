add_llvm_component_library(LLVMRiscvToyDisassembler
  RiscvToyDisassembler.cpp

  LINK_COMPONENTS
  MCDisassembler
  RiscvToyInfo
  Support

  ADD_TO_COMPONENT
  RiscvToy
  )
