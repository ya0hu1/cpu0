# RUN: llvm-mc -triple=riscvtoy-unknown-unknown -filetype=obj -o %t.o %s
# RUN: llvm-objdump -d --triple=riscvtoy-unknown-unknown %t.o \
# RUN:   | FileCheck %s

.text
.globl shifts
shifts:
  slli a0, a0, 4
  srli a0, a0, 5
  srai a0, a0, 6
  sll a1, a0, a1
  srl a1, a0, a1
  sra a1, a0, a1
  ret

# CHECK: slli a0, a0, 4
# CHECK-NEXT: srli a0, a0, 5
# CHECK-NEXT: srai a0, a0, 6
# CHECK-NEXT: sll a1, a0, a1
# CHECK-NEXT: srl a1, a0, a1
# CHECK-NEXT: sra a1, a0, a1
# CHECK-NEXT: ret
