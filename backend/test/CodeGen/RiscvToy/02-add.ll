; RUN: llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -filetype=asm -o - %s | FileCheck %s

define i32 @add(i32 %a, i32 %b) {
entry:
  %sum = add i32 %a, %b
  ret i32 %sum
}

; CHECK-LABEL: add:
; CHECK: add a0, a0, a1
; CHECK: ret
