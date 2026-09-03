; RUN: llc -O0 -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -filetype=obj -o %t.o %s
; RUN: llvm-objdump -d --triple=riscvtoy-unknown-unknown %t.o \
; RUN:   | FileCheck %s

define i32 @max(i32 %a, i32 %b) {
entry:
  %c = icmp slt i32 %a, %b
  br i1 %c, label %less, label %greater
less:
  ret i32 %b
greater:
  ret i32 %a
}

; PseudoBR is encoded as jal x0, target and decoded back as jal zero, 4.
; CHECK: jal zero, 4
