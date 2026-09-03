; RUN: llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -verify-machineinstrs -filetype=asm -o - %s | FileCheck %s
; RUN: llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -verify-machineinstrs -filetype=obj -o %t.o %s

declare i32 @callee(i32)

define i32 @indirect(i32 (i32)* %f, i32 %x) {
entry:
  %r = call i32 %f(i32 %x)
  ret i32 %r
}

; CHECK-LABEL: indirect:
; CHECK: addi sp, sp, -16
; CHECK-NEXT: sw ra, 12(sp)
; CHECK-NEXT: addi a2, a0, 0
; CHECK-NEXT: addi a0, a1, 0
; CHECK-NEXT: jalr ra, 0(a2)
; CHECK-NEXT: lw ra, 12(sp)
; CHECK-NEXT: addi sp, sp, 16
; CHECK-NEXT: ret
