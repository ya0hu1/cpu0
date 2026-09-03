# RUN: llvm-mc -triple=riscvtoy-unknown-unknown -filetype=obj -o %t.o %s
# RUN: llvm-objdump -d --triple=riscvtoy-unknown-unknown %t.o \
# RUN:   | FileCheck %s

.text
.globl memops
memops:
  lb a0, 0(a1)
  lbu a0, 1(a1)
  lh a0, 2(a1)
  lhu a0, 4(a1)
  sb a0, 0(a1)
  sh a0, 2(a1)
  ret

# CHECK: lb a0, 0(a1)
# CHECK-NEXT: lbu a0, 1(a1)
# CHECK-NEXT: lh a0, 2(a1)
# CHECK-NEXT: lhu a0, 4(a1)
# CHECK-NEXT: sb a0, 0(a1)
# CHECK-NEXT: sh a0, 2(a1)
# CHECK-NEXT: ret
