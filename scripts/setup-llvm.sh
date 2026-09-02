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
  current="$(git -C "${LLVM_DIR}" rev-parse HEAD)"
  if [[ "${current}" != "${LLVM_COMMIT}" ]]; then
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
cp -v "${ROOT_DIR}/llvm-overlay/llvm/lib/MC/MCSubtargetInfo.cpp" \
      "${LLVM_DIR}/llvm/lib/MC/MCSubtargetInfo.cpp"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/lib/Object/ELF.cpp" \
      "${LLVM_DIR}/llvm/lib/Object/ELF.cpp"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/lib/Support/Triple.cpp" \
      "${LLVM_DIR}/llvm/lib/Support/Triple.cpp"
cp -v "${ROOT_DIR}/llvm-overlay/llvm/tools/llvm-objdump/llvm-objdump.cpp" \
      "${LLVM_DIR}/llvm/tools/llvm-objdump/llvm-objdump.cpp"

log "Installing Cpu0 into LLVM"
rm -rf "${LLVM_DIR}/llvm/lib/Target/Cpu0"
mkdir -p "${LLVM_DIR}/llvm/lib/Target/Cpu0"
cp -a "${ROOT_DIR}/backend/Cpu0/." "${LLVM_DIR}/llvm/lib/Target/Cpu0/"

rm -rf "${LLVM_DIR}/llvm/test/CodeGen/Cpu0"
mkdir -p "${LLVM_DIR}/llvm/test/CodeGen/Cpu0"
cp -a "${ROOT_DIR}/backend/test/CodeGen/Cpu0/." \
      "${LLVM_DIR}/llvm/test/CodeGen/Cpu0/"

log "LLVM source is ready at ${LLVM_DIR}"
