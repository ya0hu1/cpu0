; RUN: llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -verify-machineinstrs -filetype=asm -o - %s | FileCheck %s

define i32 @cmp_use(i32 %a, i32 %b) {
entry:
  %c = icmp slt i32 %a, %b
  %x = zext i1 %c to i32
  %r = add i32 %x, %a
  ret i32 %r
}

; CHECK-LABEL: cmp_use:
; CHECK: slt a1, a0, a1
; CHECK: add a0, a1, a0
; CHECK: ret
