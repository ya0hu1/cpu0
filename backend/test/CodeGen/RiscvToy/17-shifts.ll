; RUN: llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -verify-machineinstrs -filetype=asm -o - %s | FileCheck %s

define i32 @slli(i32 %a) {
entry:
  %x = shl i32 %a, 4
  ret i32 %x
}

define i32 @srli(i32 %a) {
entry:
  %x = lshr i32 %a, 5
  ret i32 %x
}

define i32 @srai(i32 %a) {
entry:
  %x = ashr i32 %a, 6
  ret i32 %x
}

define i32 @var(i32 %a, i32 %n) {
entry:
  %x = shl i32 %a, %n
  %y = lshr i32 %x, %n
  %z = ashr i32 %y, %n
  ret i32 %z
}

; CHECK-LABEL: slli:
; CHECK: slli a0, a0, 4
; CHECK-NEXT: ret

; CHECK-LABEL: srli:
; CHECK: srli a0, a0, 5
; CHECK-NEXT: ret

; CHECK-LABEL: srai:
; CHECK: srai a0, a0, 6
; CHECK-NEXT: ret

; CHECK-LABEL: var:
; CHECK: sll a0, a0, a1
; CHECK-NEXT: srl a0, a0, a1
; CHECK-NEXT: sra a0, a0, a1
; CHECK-NEXT: ret
