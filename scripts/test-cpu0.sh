#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
TEST_DIR="${ROOT_DIR}/third_party/llvm-project/llvm/test/CodeGen/Cpu0"
TOY_TEST_DIR="${ROOT_DIR}/third_party/llvm-project/llvm/test/CodeGen/RiscvToy"

if [[ ! -x "${BUILD_DIR}/bin/llc" ]]; then
  echo "llc is missing. Build first with scripts/build-cpu0.sh." >&2
  exit 1
fi

if [[ ! -d "${TEST_DIR}" ]]; then
  echo "Cpu0 tests are missing. Run scripts/setup-llvm.sh first." >&2
  exit 1
fi

"${BUILD_DIR}/bin/llvm-lit" -sv "${TEST_DIR}" "${TOY_TEST_DIR}"
