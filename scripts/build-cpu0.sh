#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE_DIR="${ROOT_DIR}/third_party/llvm-project/llvm"
BUILD_DIR="${ROOT_DIR}/build"

if [[ ! -f "${SOURCE_DIR}/CMakeLists.txt" ]]; then
  echo "LLVM source is missing. Run scripts/setup-llvm.sh first." >&2
  exit 1
fi

if command -v ninja >/dev/null 2>&1; then
  GENERATOR="Ninja"
else
  GENERATOR="Unix Makefiles"
fi

cmake -G "${GENERATOR}" \
  -S "${SOURCE_DIR}" \
  -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_PROJECTS=clang \
  -DLLVM_OPTIMIZED_TABLEGEN=ON \
  -DLLVM_TARGETS_TO_BUILD=Cpu0

JOBS="${JOBS:-4}"
cmake --build "${BUILD_DIR}" \
  --parallel "${JOBS}" \
  --target llc opt llvm-as llvm-dis clang llvm-objdump
