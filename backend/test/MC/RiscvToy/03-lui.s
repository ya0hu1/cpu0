# RUN: llvm-mc -triple=riscvtoy-unknown-unknown -filetype=obj -o %t.o %s
# RUN: llvm-objdump -d --triple=riscvtoy-unknown-unknown %t.o \
# RUN:   | FileCheck %s

.text
.globl pos
pos:
  lui a0, 1
  addi a0, a0, 904
  ret

# CHECK: lui a0, 1
# CHECK-NEXT: addi a0, a0, 904
# CHECK-NEXT: ret
