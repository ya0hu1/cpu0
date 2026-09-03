; RUN: llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -verify-machineinstrs -filetype=asm -o - %s | FileCheck %s

define i32 @positive() {
entry:
  ret i32 5000
}

define i32 @negative() {
entry:
  ret i32 -5000
}

define i32 @edge() {
entry:
  ret i32 -4096
}

; CHECK-LABEL: positive:
; CHECK: lui a0, 1
; CHECK-NEXT: addi a0, a0, 904
; CHECK-NEXT: ret

; CHECK-LABEL: negative:
; CHECK: lui a0, 1048575
; CHECK-NEXT: addi a0, a0, -904
; CHECK-NEXT: ret

; CHECK-LABEL: edge:
; CHECK: lui a0, 1048575
; CHECK-NEXT: addi a0, a0, 0
; CHECK-NEXT: ret
