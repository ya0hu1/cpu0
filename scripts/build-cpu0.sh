#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE_DIR="${ROOT_DIR}/third_party/llvm-project/llvm"
BUILD_DIR="${ROOT_DIR}/build"

if [[ ! -f "${SOURCE_DIR}/CMakeLists.txt" ]]; then
  echo "LLVM source is missing. Run scripts/setup-llvm.sh first." >&2
  exit 1
fi

if ! command -v ninja >/dev/null 2>&1; then
  echo "ninja is required. Install it, for example: sudo apt install ninja-build" >&2
  exit 1
fi

cmake -G Ninja \
  -S "${SOURCE_DIR}" \
  -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_PROJECTS=clang \
  -DLLVM_OPTIMIZED_TABLEGEN=ON \
  -DLLVM_TARGETS_TO_BUILD=Cpu0 \
  -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=Cpu0

ninja -C "${BUILD_DIR}" llc opt llvm-as llvm-dis clang
