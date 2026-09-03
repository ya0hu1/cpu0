; RUN: llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -verify-machineinstrs -filetype=asm -o - %s | FileCheck %s

define i32 @sel(i32 %a, i32 %b) {
entry:
  %c = icmp slt i32 %a, %b
  %r = select i1 %c, i32 %a, i32 %b
  ret i32 %r
}

; CHECK-LABEL: sel:
; CHECK: blt a0, a1, .LBB0_2
; CHECK: addi a0, a1, 0
; CHECK: ret
