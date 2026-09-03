; RUN: llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -verify-machineinstrs -filetype=asm -o - %s | FileCheck %s

define i32 @sum(i32 %n, i32 %init) {
entry:
  br label %loop
loop:
  %i = phi i32 [0, %entry], [%next, %body]
  %acc = phi i32 [%init, %entry], [%sum, %body]
  %cmp = icmp slt i32 %i, %n
  br i1 %cmp, label %body, label %done
body:
  %sum = add i32 %acc, %i
  %next = add i32 %i, 1
  br label %loop
done:
  ret i32 %acc
}

; CHECK-LABEL: sum:
; CHECK: addi a2, zero, 0
; CHECK: bge a2, a0, .LBB0_3
; CHECK: add a1, a1, a2
; CHECK: addi a2, a2, 1
; CHECK: blt a2, a0, .LBB0_2
; CHECK: addi a0, a1, 0
; CHECK: ret
