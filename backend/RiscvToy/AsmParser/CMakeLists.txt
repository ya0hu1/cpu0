add_llvm_component_library(LLVMRiscvToyAsmParser
  RiscvToyAsmParser.cpp

  LINK_COMPONENTS
  MC
  MCParser
  RiscvToyDesc
  RiscvToyInfo
  Support

  ADD_TO_COMPONENT
  RiscvToy
  )
