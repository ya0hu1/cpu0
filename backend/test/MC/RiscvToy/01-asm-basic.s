# RUN: llvm-mc -triple=riscvtoy-unknown-unknown -filetype=obj -o %t.o %s
# RUN: llvm-readobj --file-headers %t.o | FileCheck --check-prefix=HDR %s
# RUN: llvm-objdump -d --triple=riscvtoy-unknown-unknown %t.o \
# RUN:   | FileCheck %s

.text
.globl add
add:
  add a0, a0, a1
  ret

# HDR: Machine: EM_RISCV (0xF3)
# CHECK: add a0, a0, a1
# CHECK: ret
