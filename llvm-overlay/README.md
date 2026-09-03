# third-party notices

The files under `llvm-overlay/llvm/` originate from
[Jonathan2251/lbd](https://github.com/Jonathan2251/lbd) at commit
`8e5c43ff1b21cd8dd4eb03d6d2ae5099b4eadd64`.

They are LLVM 12.0.1-derived files modified by LBD to support the Cpu0 backend.
The upstream LLVM source is distributed under the Apache License 2.0 with LLVM
exceptions. LBD does not include a separate license file in the current snapshot;
users should preserve LLVM's upstream licensing terms when redistributing these
files or built binaries.

The overlay currently covers:

```text
llvm/CMakeLists.txt
llvm/cmake/config-ix.cmake
llvm/include/llvm/ADT/Triple.h
llvm/include/llvm/BinaryFormat/ELF.h
llvm/include/llvm/BinaryFormat/ELFRelocs/Cpu0.def
llvm/include/llvm/IR/Intrinsics.td
llvm/include/llvm/IR/IntrinsicsCpu0.td
llvm/include/llvm/Object/ELFObjectFile.h
llvm/include/llvm/module.modulemap
llvm/lib/MC/MCSubtargetInfo.cpp
llvm/lib/Object/ELF.cpp
llvm/lib/Support/Triple.cpp
llvm/tools/elf2hex/CMakeLists.txt
llvm/tools/elf2hex/elf2hex.cpp
llvm/tools/elf2hex/elf2hex.h
llvm/tools/llvm-objdump/llvm-objdump.cpp
llvm/utils/gn/secondary/llvm/lib/Target/targets.gni
```

The first group adds Cpu0 architecture identifiers and ELF relocation
definitions. The second group adds one Cpu0 intrinsic. The remaining groups
register Cpu0 in LLVM build/target tooling. Without these files, `llc` and
`llvm-objdump` may still partially work, but object-file and assembler-related
features can fail or miss Cpu0 support.

This workspace also adds a small `RiscvToy` teaching target on top of LBD.
The corresponding repository-local changes are:

```text
llvm/CMakeLists.txt                  adds RiscvToy to LLVM_ALL_TARGETS
llvm/lib/Support/Triple.cpp          parses "riscvtoy" as Triple::riscv32
```
