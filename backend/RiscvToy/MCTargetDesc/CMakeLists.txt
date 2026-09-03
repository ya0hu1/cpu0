add_llvm_component_library(LLVMRiscvToyDesc
  RiscvToyAsmBackend.cpp
  RiscvToyELFObjectWriter.cpp
  RiscvToyInstPrinter.cpp
  RiscvToyMCAsmInfo.cpp
  RiscvToyMCCodeEmitter.cpp
  RiscvToyMCTargetDesc.cpp

  LINK_COMPONENTS
  MC
  RiscvToyInfo
  Support

  ADD_TO_COMPONENT
  RiscvToy
  )
