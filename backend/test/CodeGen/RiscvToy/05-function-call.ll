; RUN: llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -verify-machineinstrs -filetype=asm -o - %s | FileCheck %s

declare i32 @callee(i32, i32)

define i32 @caller(i32 %a, i32 %b) {
entry:
  %r = call i32 @callee(i32 %a, i32 %b)
  ret i32 %r
}

; CHECK-LABEL: caller:
; CHECK: addi sp, sp, -16
; CHECK-NEXT: sw ra, 12(sp)
; CHECK-NEXT: call callee
; CHECK-NEXT: lw ra, 12(sp)
; CHECK-NEXT: addi sp, sp, 16
; CHECK-NEXT: ret
