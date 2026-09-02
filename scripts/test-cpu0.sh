#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

if [[ ! -x "${BUILD_DIR}/bin/llc" ]]; then
  echo "llc is missing. Build first with scripts/build-cpu0.sh." >&2
  exit 1
fi

ninja -C "${BUILD_DIR}" check-llvm-codegen-cpu0
