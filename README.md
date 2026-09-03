# cpu0 LLVM Backend Workspace

This repository contains a self-contained LLVM Cpu0 backend workspace based on
[Jonathan2251/lbd](https://github.com/Jonathan2251/lbd), with Chinese study notes in `for_cpu0/`.

The goal is to keep the backend source reviewable in this repository, fetch a
fixed LLVM 12.x source tree for building, and then extend the project with a
step-by-step RISC-V backend.

## Quick start

```bash
./scripts/setup-llvm.sh
./scripts/build-cpu0.sh
./scripts/test-cpu0.sh
```

See [`for_cpu0/01-overview.md`](for_cpu0/01-overview.md) for the Chinese
explanation of the LLVM backend layout and
[`for_cpu0/02-build.md`](for_cpu0/02-build.md) for build details.
The overlay files are explained in
[`for_cpu0/04-llvm-overlay.md`](for_cpu0/04-llvm-overlay.md).

A minimal committed example is under `examples/cpu0/`:

```bash
build/bin/llc -march=cpu0 examples/cpu0/add.ll -o -
```

## Layout

```text
backend/Cpu0/             Cpu0 backend from Jonathan2251/lbd
backend/test/CodeGen/Cpu0 Regression tests copied from LBD
llvm-overlay/llvm/        Files that must replace the corresponding LLVM files
for_cpu0/                 Chinese study notes
examples/cpu0/            Small runnable Cpu0 examples
scripts/                  Setup/build/test helpers
third_party/llvm-project  LLVM source created by setup-llvm.sh (not committed)
build/                    CMake build tree (not committed)
```
