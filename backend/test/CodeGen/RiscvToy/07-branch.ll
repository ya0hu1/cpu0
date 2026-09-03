; RUN: llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -verify-machineinstrs -filetype=asm -o - %s | FileCheck %s

define i32 @max(i32 %a, i32 %b) {
entry:
  %c = icmp slt i32 %a, %b
  br i1 %c, label %less, label %greater
less:
  ret i32 %b
greater:
  ret i32 %a
}

; CHECK-LABEL: max:
; CHECK: bge a0, a1, .LBB0_2
; CHECK: addi a0, a1, 0
; CHECK: ret
