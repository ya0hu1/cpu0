; RUN: llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -filetype=obj -o %t.o %s
; RUN: llvm-readobj -r --symbols %t.o | FileCheck %s

declare i32 @callee(i32, i32)

define i32 @caller(i32 %a, i32 %b) {
entry:
  %r = call i32 @callee(i32 %a, i32 %b)
  ret i32 %r
}

; CHECK: R_RISCV_JAL callee 0x0
; CHECK: Name: callee
; CHECK: Binding: Global (0x1)
; CHECK: Name: caller
