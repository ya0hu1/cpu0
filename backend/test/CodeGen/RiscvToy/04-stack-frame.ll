; RUN: llc -O0 -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -verify-machineinstrs -filetype=asm -o - %s | FileCheck %s

define i32 @stack_frame(i32 %a, i32 %b) {
entry:
  %p = alloca i32, align 4
  store i32 %a, i32* %p, align 4
  %v = load i32, i32* %p, align 4
  %r = add i32 %v, %b
  ret i32 %r
}

; CHECK-LABEL: stack_frame:
; CHECK: addi sp, sp, -4
; CHECK-NEXT: sw a0, 0(sp)
; CHECK-NEXT: lw a0, 0(sp)
; CHECK-NEXT: add a0, a0, a1
; CHECK-NEXT: addi sp, sp, 4
; CHECK-NEXT: ret
