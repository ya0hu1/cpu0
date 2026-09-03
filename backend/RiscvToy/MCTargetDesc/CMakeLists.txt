add_llvm_component_library(LLVMRiscvToyDesc
  RiscvToyInstPrinter.cpp
  RiscvToyMCAsmInfo.cpp
  RiscvToyMCTargetDesc.cpp

  LINK_COMPONENTS
  MC
  RiscvToyInfo
  Support

  ADD_TO_COMPONENT
  RiscvToy
  )
