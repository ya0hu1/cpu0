; RUN: llc -O0 -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -filetype=obj -o %t.o %s
; RUN: llvm-objdump -s -j .text %t.o | FileCheck %s

define i32 @max(i32 %a, i32 %b) {
entry:
  %c = icmp slt i32 %a, %b
  br i1 %c, label %less, label %greater
less:
  ret i32 %b
greater:
  ret i32 %a
}

; PseudoBR expands to jal x0, target. Its little-endian bytes are 6f 00 40 00.
; CHECK: 0010 6f004000
