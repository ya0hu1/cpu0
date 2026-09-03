# RUN: llvm-mc -triple=riscvtoy-unknown-unknown -filetype=obj -o %t.o %s
# RUN: llvm-readobj -r --symbols %t.o | FileCheck %s
# RUN: llvm-objdump -d --triple=riscvtoy-unknown-unknown %t.o \
# RUN:   | FileCheck --check-prefix=DIS %s

.text
.globl caller
caller:
  addi sp, sp, -16
  sw ra, 12(sp)
  call callee
  lw ra, 12(sp)
  addi sp, sp, 16
  ret

# CHECK: R_RISCV_JAL callee 0x0
# CHECK: Name: caller
# DIS: addi sp, sp, -16
# DIS: sw ra, 12(sp)
# DIS: jal ra, 0
# DIS: ret
