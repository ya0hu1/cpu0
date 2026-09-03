; RUN: llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -filetype=asm -o - %s | FileCheck %s

define i32 @mix(i32 %a, i32 %b) {
entry:
  %s = sub i32 %a, %b
  %x = xor i32 %s, %b
  %r = add i32 %x, %a
  ret i32 %r
}

; CHECK-LABEL: mix:
; CHECK: sub a2, a0, a1
; CHECK: xor a1, a2, a1
; CHECK: add a0, a1, a0
; CHECK: ret

define i32 @add_const(i32 %a) {
entry:
  %r = add i32 %a, 7
  ret i32 %r
}

; CHECK-LABEL: add_const:
; CHECK: addi a0, a0, 7
; CHECK: ret
