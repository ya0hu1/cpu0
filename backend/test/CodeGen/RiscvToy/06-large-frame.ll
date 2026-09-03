; RUN: llc -O0 -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -verify-machineinstrs -filetype=asm -o - %s | FileCheck %s

define void @big(i32 %v) {
entry:
  %p = alloca [3000 x i8], align 16
  %p32 = bitcast [3000 x i8]* %p to i32*
  store i32 %v, i32* %p32, align 4
  ret void
}

; CHECK-LABEL: big:
; CHECK: addi sp, sp, -2047
; CHECK-NEXT: addi sp, sp, -961
; CHECK-NEXT: sw a0, 0(sp)
; CHECK-NEXT: addi sp, sp, 2047
; CHECK-NEXT: addi sp, sp, 961
; CHECK-NEXT: ret
