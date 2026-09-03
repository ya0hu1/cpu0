#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
THIRD_PARTY_DIR="${ROOT_DIR}/third_party"
LLVM_DIR="${THIRD_PARTY_DIR}/llvm-project"
LLVM_COMMIT="e8a397203c67adbeae04763ce25c6a5ae76af52c"

log() {
  printf '\n[cpu0-setup] %s\n' "$*"
}

mkdir -p "${THIRD_PARTY_DIR}"

if [[ ! -d "${LLVM_DIR}/.git" ]]; then
  log "Cloning the required LLVM 12.x source into third_party/llvm-project"
  git init -q "${LLVM_DIR}"
  git -C "${LLVM_DIR}" remote add origin https://github.com/llvm/llvm-project.git
  git -C "${LLVM_DIR}" fetch --depth 1 origin "${LLVM_COMMIT}"
  git -C "${LLVM_DIR}" checkout -q FETCH_HEAD
else
  if ! current="$(git -C "${LLVM_DIR}" rev-parse --verify HEAD 2>/dev/null)" ||
     [[ "${current}" != "${LLVM_COMMIT}" ]]; then
    log "Checking out required LLVM commit ${LLVM_COMMIT}"
    git -C "${LLVM_DIR}" fetch --depth 1 origin "${LLVM_COMMIT}"
    git -C "${LLVM_DIR}" checkout -q FETCH_HEAD
  fi
fi

log "Applying the small set of LLVM files required by Cpu0"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/CMakeLists.txt" \
      "${LLVM_DIR}/llvm/CMakeLists.txt"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/cmake/config-ix.cmake" \
      "${LLVM_DIR}/llvm/cmake/config-ix.cmake"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/include/module.modulemap" \
      "${LLVM_DIR}/llvm/include/llvm/module.modulemap"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/include/llvm/ADT/Triple.h" \
      "${LLVM_DIR}/llvm/include/llvm/ADT/Triple.h"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/include/llvm/BinaryFormat/ELF.h" \
      "${LLVM_DIR}/llvm/include/llvm/BinaryFormat/ELF.h"
mkdir -p "${LLVM_DIR}/llvm/include/llvm/BinaryFormat/ELFRelocs"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/include/llvm/BinaryFormat/ELFRelocs/Cpu0.def" \
      "${LLVM_DIR}/llvm/include/llvm/BinaryFormat/ELFRelocs/Cpu0.def"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/include/llvm/IR/Intrinsics.td" \
      "${LLVM_DIR}/llvm/include/llvm/IR/Intrinsics.td"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/include/llvm/IR/IntrinsicsCpu0.td" \
      "${LLVM_DIR}/llvm/include/llvm/IR/IntrinsicsCpu0.td"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/include/llvm/Object/ELFObjectFile.h" \
      "${LLVM_DIR}/llvm/include/llvm/Object/ELFObjectFile.h"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/lib/MC/MCSubtargetInfo.cpp" \
      "${LLVM_DIR}/llvm/lib/MC/MCSubtargetInfo.cpp"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/lib/Object/ELF.cpp" \
      "${LLVM_DIR}/llvm/lib/Object/ELF.cpp"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/lib/Support/Triple.cpp" \
      "${LLVM_DIR}/llvm/lib/Support/Triple.cpp"
mkdir -p "${LLVM_DIR}/llvm/tools/elf2hex"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/tools/elf2hex/CMakeLists.txt" \
      "${LLVM_DIR}/llvm/tools/elf2hex/CMakeLists.txt"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/tools/elf2hex/elf2hex.cpp" \
      "${LLVM_DIR}/llvm/tools/elf2hex/elf2hex.cpp"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/tools/elf2hex/elf2hex.h" \
      "${LLVM_DIR}/llvm/tools/elf2hex/elf2hex.h"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/tools/llvm-objdump/llvm-objdump.cpp" \
      "${LLVM_DIR}/llvm/tools/llvm-objdump/llvm-objdump.cpp"
mkdir -p "${LLVM_DIR}/llvm/utils/gn/secondary/llvm/lib/Target"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/utils/gn/secondary/llvm/lib/Target/targets.gni" \
      "${LLVM_DIR}/llvm/utils/gn/secondary/llvm/lib/Target/targets.gni"

log "Installing Cpu0 into LLVM"
rm -rf "${LLVM_DIR}/llvm/lib/Target/Cpu0"
mkdir -p "${LLVM_DIR}/llvm/lib/Target/Cpu0"
cp -a "${ROOT_DIR}/backend/Cpu0/." "${LLVM_DIR}/llvm/lib/Target/Cpu0/"

rm -rf "${LLVM_DIR}/llvm/test/CodeGen/Cpu0"
mkdir -p "${LLVM_DIR}/llvm/test/CodeGen/Cpu0"
cp -a "${ROOT_DIR}/backend/test/CodeGen/Cpu0/." \
      "${LLVM_DIR}/llvm/test/CodeGen/Cpu0/"

log "LLVM source is ready at ${LLVM_DIR}"
