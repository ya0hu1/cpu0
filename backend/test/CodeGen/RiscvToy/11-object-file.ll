; RUN: llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -filetype=obj -o %t.o %s
; RUN: llvm-readobj --file-headers %t.o | FileCheck --check-prefix=HDR %s
; RUN: llvm-objdump -h %t.o | FileCheck --check-prefix=SECT %s
; RUN: llvm-objdump -s -j .text %t.o | FileCheck --check-prefix=TEXT %s

define i32 @max(i32 %a, i32 %b) {
entry:
  %c = icmp slt i32 %a, %b
  br i1 %c, label %less, label %greater
less:
  ret i32 %b
greater:
  ret i32 %a
}

; HDR: Machine: EM_RISCV (0xF3)
; SECT: .text
; TEXT: 0000 6354b500 13850500 67800000
