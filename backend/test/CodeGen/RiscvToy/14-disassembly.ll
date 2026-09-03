; RUN: llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
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

; CHECK: bge a0, a1, 8
; CHECK: addi a0, a1, 0
; CHECK: ret
